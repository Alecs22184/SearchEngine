#include "threaded_spider.h"
#include <iostream>
#include <chrono>
#include <thread>

ThreadedSpider::ThreadedSpider(Config& config, Database& db)
    : config_(config), db_(db) {
    http_client_.SetTimeout(config_.GetRequestTimeout());
    http_client_.SetUserAgent(config_.GetUserAgent());

    std::cout << "ThreadedSpider initialized with:" << std::endl;
    std::cout << "  Start URL: " << config_.GetStartUrl() << std::endl;
    std::cout << "  Max Depth: " << config_.GetMaxDepth() << std::endl;
    std::cout << "  Threads: " << config_.GetThreadCount() << std::endl;
    std::cout << "  Timeout: " << config_.GetRequestTimeout() << "s" << std::endl;
}

ThreadedSpider::~ThreadedSpider() {
    Stop();
}

void ThreadedSpider::Start() {
    if (running_) {
        std::cout << "Spider is already running!" << std::endl;
        return;
    }

    running_ = true;
    processed_count_ = 0;
    error_count_ = 0;

    std::cout << "=== Starting Threaded Spider ===" << std::endl;
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Start URL: " << config_.GetStartUrl() << std::endl;
    std::cout << "  Max Depth: " << config_.GetMaxDepth() << std::endl;
    std::cout << "  Thread Count: " << config_.GetThreadCount() << std::endl;
    std::cout << "  Request Timeout: " << config_.GetRequestTimeout() << "s" << std::endl;
    std::cout << "  User Agent: " << config_.GetUserAgent() << std::endl;
    std::cout << "  Delay between requests: " << config_.GetDelayBetweenRequests() << "ms" << std::endl;

    // Создаем рабочие потоки ПЕРВЫМИ
    for (int i = 0; i < config_.GetThreadCount(); ++i) {
        workers_.emplace_back(&ThreadedSpider::WorkerThread, this);
        std::cout << "Started worker thread " << i + 1 << std::endl;
    }

    // Затем добавляем начальный URL в очередь
    AddUrlToQueue(config_.GetStartUrl(), 0);

    std::cout << "Spider started with " << workers_.size() << " worker threads" << std::endl;

    // Ждем завершения всех потоков
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    running_ = false;

    std::cout << "=== Threaded Spider Finished ===" << std::endl;
    std::cout << "Statistics:" << std::endl;
    std::cout << "  URLs Processed: " << processed_count_ << std::endl;
    std::cout << "  Errors: " << error_count_ << std::endl;
    std::cout << "  URLs visited: " << visited_urls_.size() << std::endl;
}

void ThreadedSpider::Stop() {
    if (!running_) return;

    std::cout << "Stopping spider..." << std::endl;
    running_ = false;
    queue_cv_.notify_all();
}

void ThreadedSpider::WorkerThread() {
    std::cout << "Worker thread " << std::this_thread::get_id() << " started" << std::endl;

    while (running_) {
        UrlTask task;
        bool has_task = false;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);

            // Ждем задачи или сигнала остановки
            if (url_queue_.empty()) {
                queue_cv_.wait_for(lock, std::chrono::milliseconds(100), [this]() {
                    return !url_queue_.empty() || !running_;
                    });
            }

            if (!running_) {
                break;
            }

            if (!url_queue_.empty()) {
                task = url_queue_.front();
                url_queue_.pop();
                has_task = true;
            }
        }

        if (has_task) {
            ProcessUrl(task.url, task.depth);
        }
    }

    std::cout << "Worker thread " << std::this_thread::get_id() << " finished" << std::endl;
}

void ThreadedSpider::ProcessUrl(const std::string& url, int depth) {
    if (!running_) return;

    // Проверяем глубину
    if (depth > config_.GetMaxDepth()) {
        return;
    }

    // Проверяем и добавляем URL в посещенные
    {
        std::lock_guard<std::mutex> lock(visited_mutex_);
        if (visited_urls_.find(url) != visited_urls_.end()) {
            return;
        }
        visited_urls_.insert(url);
    }

    std::cout << "=== Processing URL: " << url << " (depth: " << depth << ") ===" << std::endl;

    // Загружаем страницу
    auto response = http_client_.DownloadPage(url);

    std::cout << "Download completed - Status: " << response.status_code
        << ", Size: " << response.content.size() << " bytes" << std::endl;

    if (response.status_code != 200) {
        std::cout << "ERROR: Failed to download URL" << std::endl;
        error_count_++;
        return;
    }

    // Проверяем, что это HTML
    if (response.content_type.find("text/html") == std::string::npos) {
        std::cout << "Skipping non-HTML content" << std::endl;
        return;
    }

    // Извлекаем текст
    std::string clean_text = HtmlParser::ExtractText(response.content);
    std::cout << "Clean text size: " << clean_text.length() << " characters" << std::endl;

    // Извлекаем слова
    auto words = HtmlParser::ExtractWords(clean_text);
    std::cout << "Extracted " << words.size() << " words" << std::endl;

    // Получаем заголовок
    std::string title = HtmlParser::GetPageTitle(response.content);
    if (title.empty()) {
        title = url;
    }

    // Добавляем документ в базу
    int doc_id = db_.AddDocument(url, title, clean_text);
    if (doc_id == -1) {
        std::cout << "ERROR: Failed to add document to database" << std::endl;
        error_count_++;
        return;
    }

    std::cout << "Document added with ID: " << doc_id << std::endl;

    // Подсчитываем частоту слов
    std::map<std::string, int> word_freq;
    for (const auto& word : words) {
        if (word.length() >= 3 && word.length() <= 32) {
            word_freq[word]++;
        }
    }

    // Добавляем слова в базу
    int words_added = 0;
    for (const auto& [word, freq] : word_freq) {
        int word_id = db_.AddWord(word);
        if (word_id != -1) {
            db_.AddDocumentWord(doc_id, word_id, freq);
            words_added++;
        }
    }

    processed_count_++;
    std::cout << "Successfully indexed page. Words added: " << words_added << std::endl;

    // Извлекаем ссылки для следующего уровня
    if (depth < config_.GetMaxDepth()) {
        auto links = HtmlParser::ExtractLinks(response.content, url);
        std::cout << "Found " << links.size() << " links" << std::endl;

        for (const auto& link : links) {
            AddUrlToQueue(link, depth + 1);
        }
    }

    // Задержка между запросами
    if (config_.GetDelayBetweenRequests() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(config_.GetDelayBetweenRequests()));
    }
}

bool ThreadedSpider::AddUrlToQueue(const std::string& url, int depth) {
    if (!running_ || depth > config_.GetMaxDepth()) {
        return false;
    }

    // Проверяем валидность URL
    if (!http_client_.IsValidUrl(url)) {
        return false;
    }

    // Проверяем, не посещали ли уже этот URL
    {
        std::lock_guard<std::mutex> lock(visited_mutex_);
        if (visited_urls_.find(url) != visited_urls_.end()) {
            return false;
        }
    }

    // Добавляем в очередь
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        url_queue_.push({ url, depth });
    }

    queue_cv_.notify_one();
    return true;
}