//
// Created by Максим Залеский on 08.07.2024.
//

#ifndef SCREEN_MANAGER_BACK_API_HANDLER_H
#define SCREEN_MANAGER_BACK_API_HANDLER_H

#include "crow_all.h"
#include "database.h"

class ApiHandler {
public:
    ApiHandler();
    void run();

private:
    crow::SimpleApp app;
    Database database;
};

#endif //SCREEN_MANAGER_BACK_API_HANDLER_H
