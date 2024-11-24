#include "crow.h"
#include <iostream>
#include <functional>
#include <thread>

using ServerCallback = std::function<void(const std::string&)>;

void startServer(ServerCallback callback) {
    crow::SimpleApp app;
    #ifdef _DEBUG
        app.loglevel(crow::LogLevel::Debug);
    #else
        app.loglevel(crow::LogLevel::Error);
    #endif
    // localhost:5700/dota/
    CROW_ROUTE(app, "/dota/").methods(crow::HTTPMethod::POST)([&callback](const crow::request& req) {
        auto body = req.body;

        if (callback) {
            callback(body);
        }

        return crow::response(200, "Successfully");
        });

    // Async thread
    app.port(5700).multithreaded().run();
}
