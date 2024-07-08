// Created by Максим Залеский on 08.07.2024.

#include "database.h"

Database::Database() : db(nullptr), inTransaction(false) {}

Database::~Database() {
    close();
}

void Database::open(const std::string &path_) {
    this->path = path_;
    int rc = sqlite3_open(path.c_str(), &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        db = nullptr;
    }
}

void Database::close() {
    if (db) {
        if (inTransaction) {
            commit();
        }
        sqlite3_close(db);
        db = nullptr;
    }
}

bool Database::execute(const std::string &sql, const std::vector<ParamType> &params) {
    if (!db) {
        std::cerr << "Database is not opened" << std::endl;
        return false;
    }

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    if (!bindParameters(stmt, params)) {
        finalizeStatement(stmt);
        return false;
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
        std::cerr << "Execution failed: " << sqlite3_errmsg(db) << std::endl;
        finalizeStatement(stmt);
        return false;
    }

    finalizeStatement(stmt);
    return true;
}

bool Database::execute(const std::string &sql) {
    return execute(sql, {});
}

std::vector<std::vector<std::any>> Database::executeSelect(const std::string &sql, const std::vector<ParamType> &params) {
    std::vector<std::vector<std::any>> results;

    if (!db) {
        std::cerr << "Database is not opened" << std::endl;
        return results;
    }

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return results;
    }

    if (!bindParameters(stmt, params)) {
        finalizeStatement(stmt);
        return results;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::vector<std::any> row;
        int colCount = sqlite3_column_count(stmt);

        for (int col = 0; col < colCount; ++col) {
            switch (sqlite3_column_type(stmt, col)) {
                case SQLITE_INTEGER:
                    row.push_back(sqlite3_column_int(stmt, col));
                    break;
                case SQLITE_FLOAT:
                    row.push_back(sqlite3_column_double(stmt, col));
                    break;
                case SQLITE_TEXT:
                    row.push_back(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, col))));
                    break;
                case SQLITE_NULL:
                    row.push_back(std::any());
                    break;
                default:
                    std::cerr << "Unsupported column type" << std::endl;
                    row.push_back(std::any());
                    break;
            }
        }
        results.push_back(row);
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "Execution failed: " << sqlite3_errmsg(db) << std::endl;
    }

    finalizeStatement(stmt);
    return results;
}

void Database::beginTransaction() {
    if (!inTransaction) {
        execute("BEGIN TRANSACTION;", {});
        inTransaction = true;
    }
}

bool Database::commit() {
    if (inTransaction) {
        if (!execute("COMMIT;", {})) {
            rollback();
            return false;
        }
        inTransaction = false;
    }
    return true;
}

void Database::rollback() {
    if (inTransaction) {
        execute("ROLLBACK;", {});
        inTransaction = false;
    }
}

bool Database::bindParameters(sqlite3_stmt *stmt, const std::vector<ParamType> &params) {
    for (size_t i = 0; i < params.size(); ++i) {
        const auto &param = params[i];
        if (param.type() == typeid(int)) {
            sqlite3_bind_int(stmt, static_cast<int>(i + 1), std::any_cast<int>(param));
        } else if (param.type() == typeid(double)) {
            sqlite3_bind_double(stmt, static_cast<int>(i + 1), std::any_cast<double>(param));
        } else if (param.type() == typeid(std::string)) {
            sqlite3_bind_text(stmt, static_cast<int>(i + 1), std::any_cast<std::string>(param).c_str(), -1, SQLITE_STATIC);
        } else {
            std::cerr << "Unsupported parameter type" << std::endl;
            return false;
        }
    }
    return true;
}

void Database::finalizeStatement(sqlite3_stmt *stmt) {
    sqlite3_finalize(stmt);
}
