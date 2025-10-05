#ifndef THREADED_SPIDER_H
#define THREADED_SPIDER_H

#include "config.h"
#include "database.h"
#include "http_client.h"
#include "html_parser.h"
#include <string>
#include <queue>
#include <unordered_set>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

struct UrlTask {
    std::string url;
    int depth;
};

class ThreadedSpider {
public:
    ThreadedSpider(Config& config, Database& db);
    ~ThreadedSpider();

    void Start();
    void Stop();

    int GetProcessedCount() const { return processed_count_; }
    int GetErrorCount() const { return error_count_; }

private:
    void WorkerThread();
    void ProcessUrl(const std::string& url, int depth);
    bool AddUrlToQueue(const std::string& url, int depth);
    bool ShouldProcessUrl(const std::string& url);

    Config& config_;
    Database& db_;
    HttpClient http_client_;

    // Очередь URL для обработки
    std::queue<UrlTask> url_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;

    // Посещенные URL
    std::unordered_set<std::string> visited_urls_;
    std::mutex visited_mutex_;

    // Рабочие потоки
    std::vector<std::thread> workers_;

    // Статистика и управление
    std::atomic<bool> running_{ false };
    std::atomic<int> processed_count_{ 0 };
    std::atomic<int> error_count_{ 0 };
};

#endif // THREADED_SPIDER_H