#include "config.h"
#include "database.h"
#include "beast_http_server.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "=== Search Engine Server ===" << std::endl;

    // Загрузка конфигурации
    Config config;
    if (!config.Load("config.ini")) {
        std::cerr << "Failed to load config file" << std::endl;
        return 1;
    }

    // Подключение к базе данных
    Database db;
    if (!db.Connect(config.GetDatabaseHost(), config.GetDatabasePort(),
        config.GetDatabaseName(), config.GetDatabaseUser(),
        config.GetDatabasePassword())) {
        std::cerr << "Failed to connect to database" << std::endl;
        return 1;
    }

    std::cout << "Database connection established." << std::endl;
    std::cout << "Starting HTTP server on " << config.GetServerHost()
        << ":" << config.GetServerPort() << std::endl;

    try {
        // Запуск HTTP сервера
        BeastHttpServer server(config, db);
        server.Start();

        std::cout << "Server is running. Press Ctrl+C to stop." << std::endl;

        // Бесконечный цикл для поддержания работы сервера
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}