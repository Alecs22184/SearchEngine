#include "config.h"
#include "database.h"
#include "threaded_spider.h"
#include <iostream>

int main() {
    std::cout << "=== Search Engine Spider ===" << std::endl;

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

    // Создание таблиц
    if (!db.CreateTables()) {
        std::cerr << "Failed to create tables" << std::endl;
        return 1;
    }

    std::cout << "Starting spider with configuration:" << std::endl;
    std::cout << "  Start URL: " << config.GetStartUrl() << std::endl;
    std::cout << "  Max Depth: " << config.GetMaxDepth() << std::endl;
    std::cout << "  Threads: " << config.GetThreadCount() << std::endl;
    std::cout << "  Timeout: " << config.GetRequestTimeout() << " seconds" << std::endl;

    // Запуск паука
    ThreadedSpider spider(config, db);
    spider.Start();

    // Выводим статистику
    std::cout << "\n=== Final Statistics ===" << std::endl;
    db.PrintStats();
    std::cout << "Processed URLs: " << spider.GetProcessedCount() << std::endl;
    std::cout << "Errors: " << spider.GetErrorCount() << std::endl;

    std::cout << "Spider completed successfully!" << std::endl;
    return 0;
}