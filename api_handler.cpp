// Created by Максим Залеский on 08.07.2024.

#include "api_handler.h"
#include "database.h"
#include "crow_all.h"
#include "cstdlib"
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

void init_db(Database& db) {
    bool drop = false;
    const char* isDrop = std::getenv("DROP");
    if (isDrop && strcmp(isDrop, "true") == 0) {
        drop = true;
    }

    db.beginTransaction();

    if (drop) {
        db.execute("DROP TABLE IF EXISTS users;");
        db.execute("DROP TABLE IF EXISTS screens;");
    }

    db.execute("CREATE TABLE IF NOT EXISTS users ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "login TEXT UNIQUE, "
               "password_hash TEXT"
               ");");

    db.execute("CREATE TABLE IF NOT EXISTS screens ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "name TEXT UNIQUE, "
               "run TEXT, "
               "path TEXT"
               ");");

    db.commit();
}

ApiHandler::ApiHandler() {
    database.open("/tmp/screen_manager.db");

    init_db(database);

    CROW_ROUTE(app, "/register").methods("POST"_method)
            ([this](const crow::request &req) {
                auto x = crow::json::load(req.body);
                if (!x) {
                    return crow::response(400, "Invalid JSON");
                }

                std::string login = x["login"].s();
                std::string password_hash = x["password_hash"].s();
                std::string secret = x["secret"].s();
                const char* real_secret = std::getenv("SECRET");

                if (!real_secret || secret != real_secret) {
                    return crow::response(403, "Forbidden");
                }

                if (login.empty() || password_hash.empty()) {
                    return crow::response(400, "Login and password_hash cannot be empty");
                }

                try {
                    database.beginTransaction();
                    bool success = database.execute("INSERT INTO users (login, password_hash) VALUES (?, ?);", {login, password_hash});
                    if (!success) {
                        database.rollback();
                        return crow::response(500, "Failed to insert user");
                    }
                    database.commit();
                    return crow::response(200, "User registered successfully");
                } catch (const std::exception &e) {
                    database.rollback();
                    return crow::response(500, "Database error: " + std::string(e.what()));
                }
            });

    CROW_ROUTE(app, "/add").methods("POST"_method)
            ([this](const crow::request &req) {
                auto x = crow::json::load(req.body);
                if (!x) {
                    return crow::response(400, "Invalid JSON");
                }

                std::string name = x["name"].s();
                std::string run = x["run"].s();
                std::string path = x["path"].s();
                std::string login = x["login"].s();
                std::string password_hash = x["password_hash"].s();

                // Check if user exists
                std::vector<std::vector<std::any>> result = database.executeSelect("SELECT id FROM users WHERE login = ? AND password_hash = ?;", {login, password_hash});
                if (result.empty()) {
                    return crow::response(403, "Forbidden");
                }

                if (name.empty() || path.empty()) {
                    return crow::response(400, "Name and path cannot be empty");
                }

                try {
                    database.beginTransaction();
                    bool success = database.execute("INSERT INTO screens (name, run, path) VALUES (?, ?, ?);", {name, run, path});
                    if (!success) {
                        database.rollback();
                        return crow::response(500, "Failed to insert screen");
                    }
                    database.commit();

                    // Create the directory if it doesn't exist
                    if (mkdir(path.c_str(), 0777) && errno != EEXIST) {
                        database.rollback();
                        return crow::response(500, "Failed to create directory");
                    }

                    // Write run to run.sh
                    std::ofstream run_file(path + "/run.sh");
                    if (!run_file) {
                        return crow::response(500, "Failed to create run.sh file");
                    }
                    run_file << run;
                    run_file.close();

                    // Make run.sh executable
                    if (chmod((path + "/run.sh").c_str(), 0777) != 0) {
                        return crow::response(500, "Failed to set execute permissions on run.sh");
                    }

                    return crow::response(200, "Screen added successfully");
                } catch (const std::exception &e) {
                    database.rollback();
                    return crow::response(500, "Database error: " + std::string(e.what()));
                }
            });

    CROW_ROUTE(app, "/kill").methods("POST"_method)
            ([this](const crow::request &req) {
                auto x = crow::json::load(req.body);
                if (!x) {
                    return crow::response(400, "Invalid JSON");
                }

                std::string name = x["name"].s();
                std::string login = x["login"].s();
                std::string password_hash = x["password_hash"].s();

                // Check if user exists
                std::vector<std::vector<std::any>> result = database.executeSelect("SELECT id FROM users WHERE login = ? AND password_hash = ?;", {login, password_hash});
                if (result.empty()) {
                    return crow::response(403, "Forbidden");
                }

                if (name.empty()) {
                    return crow::response(400, "Name cannot be empty");
                }

                try {
                    database.beginTransaction();
                    bool success = database.execute("DELETE FROM screens WHERE name = ?;", {name});
                    if (!success) {
                        database.rollback();
                        return crow::response(500, "Failed to delete screen");
                    }
                    database.commit();

                    std::string system_request = "screen -ls | grep " + name + " | awk '{print $1}' | xargs -I {} screen -S {} -X quit";
                    system(system_request.c_str());

                    return crow::response(200, "Screen killed successfully");
                } catch (const std::exception &e) {
                    database.rollback();
                    return crow::response(500, "Database error: " + std::string(e.what()));
                }
            });
}

void ApiHandler::run() {
    app.port(9923).multithreaded().run();
}
