// Created by Максим Залеский on 08.07.2024.

#ifndef SCREEN_MANAGER_BACK_DATABASE_H
#define SCREEN_MANAGER_BACK_DATABASE_H

#include <sqlite3.h>
#include <string>
#include <vector>
#include <iostream>
#include <any>

class Database {
public:
    Database();
    ~Database();

    void open(const std::string &path);
    void close();

    using ParamType = std::any;
    bool execute(const std::string &sql, const std::vector<ParamType> &params);
    bool execute(const std::string &sql);

    std::vector<std::vector<std::any>> executeSelect(const std::string &sql, const std::vector<ParamType> &params);

    void beginTransaction();
    bool commit();
    void rollback();

private:
    sqlite3 *db;
    std::string path;
    bool inTransaction;

    static bool bindParameters(sqlite3_stmt *stmt, const std::vector<ParamType> &params);
    static void finalizeStatement(sqlite3_stmt *stmt);
};

#endif //SCREEN_MANAGER_BACK_DATABASE_H
