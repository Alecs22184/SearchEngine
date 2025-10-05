#include "beast_http_server.h"
#include "html_parser.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <csignal>

// Глобальный флаг для обработки сигналов
std::atomic<bool> g_signal_received{ false };

void signal_handler(int signal) {
    std::cout << "Received signal " << signal << ", shutting down gracefully..." << std::endl;
    g_signal_received = true;
}

BeastHttpServer::BeastHttpServer(Config& config, Database& db)
    : config_(config), db_(db), ioc_(), acceptor_(ioc_) {
}

BeastHttpServer::~BeastHttpServer() {
    Stop();
}

void BeastHttpServer::Start() {
    try {
        // Устанавливаем обработчики сигналов
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        auto const address = net::ip::make_address(config_.GetServerHost());
        auto const port = static_cast<unsigned short>(config_.GetServerPort());

        tcp::endpoint endpoint{ address, port };

        // Открываем acceptor
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(net::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen();

        std::cout << "HTTP Server started on " << config_.GetServerHost() << ":" << config_.GetServerPort() << std::endl;

        CreateWorkerThreads();
        Listen();

        // Запускаем обработку в текущем потоке
        Run();

    }
    catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
    }
}

void BeastHttpServer::Stop() {
    stopped_ = true;
    if (acceptor_.is_open()) {
        acceptor_.close();
    }
    ioc_.stop();
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void BeastHttpServer::Run() {
    std::cout << "Server is running. Press Ctrl+C to stop." << std::endl;

    // Основной цикл с проверкой сигналов
    while (!stopped_ && !g_signal_received) {
        ioc_.run_for(std::chrono::milliseconds(100));
    }

    std::cout << "Server stopped gracefully" << std::endl;
}

void BeastHttpServer::CreateWorkerThreads() {
    int thread_count = config_.GetServerThreads();
    worker_threads_.reserve(thread_count);
    for (int i = 0; i < thread_count; ++i) {
        worker_threads_.emplace_back([this]() {
            ioc_.run();
            });
    }
}

void BeastHttpServer::Listen() {
    if (stopped_ || g_signal_received) return;

    auto socket = std::make_shared<tcp::socket>(ioc_);

    acceptor_.async_accept(*socket, [this, socket](beast::error_code ec) {
        if (!ec) {
            // Обрабатываем запрос в отдельном потоке
            std::thread([this, socket]() {
                try {
                    beast::flat_buffer buffer;
                    http::request<http::string_body> req;

                    // Читаем запрос
                    http::read(*socket, buffer, req);

                    // Обрабатываем запрос
                    HandleRequest(std::move(req), socket);
                }
                catch (const std::exception& e) {
                    std::cerr << "Request handling error: " << e.what() << std::endl;
                    try {
                        http::response<http::string_body> res{ http::status::internal_server_error, 11 };
                        res.set(http::field::server, "SearchEngine/1.0");
                        res.set(http::field::content_type, "text/html");
                        res.body() = GenerateErrorPage("Internal server error");
                        res.prepare_payload();
                        http::write(*socket, res);
                    }
                    catch (...) {
                        // Игнорируем ошибки при отправке ответа об ошибке
                    }
                }
                }).detach();
        }
        else {
            if (ec != beast::errc::operation_canceled) {
                std::cerr << "Accept error: " << ec.message() << std::endl;
            }
        }

        // Продолжаем принимать соединения если не остановлено
        if (!stopped_ && !g_signal_received) {
            Listen();
        }
        });
}


//void BeastHttpServer::Listen() {
//    if (stopped_ || g_signal_received) return;
//
//    auto socket = std::make_shared<tcp::socket>(ioc_);
//
//    acceptor_.async_accept(*socket, [this, socket](beast::error_code ec) {
//        if (!ec) {
//            // Обрабатываем запрос с таймаутами
//            std::thread([this, socket]() {
//                try {
//                    // Устанавливаем таймауты
//                    socket->set_option(net::socket_base::receive_timeout(std::chrono::seconds(10)));
//                    socket->set_option(net::socket_base::send_timeout(std::chrono::seconds(10)));
//
//                    beast::flat_buffer buffer;
//                    http::request<http::string_body> req;
//
//                    // Читаем запрос с таймаутом
//                    http::read(*socket, buffer, req);
//
//                    // Обрабатываем запрос
//                    HandleRequest(std::move(req), socket);
//                }
//                catch (const beast::system_error& se) {
//                    if (se.code() != beast::error::timeout) {
//                        std::cerr << "Request handling error: " << se.what() << std::endl;
//                    }
//                }
//                catch (const std::exception& e) {
//                    std::cerr << "Request handling error: " << e.what() << std::endl;
//                }
//                }).detach();
//        }
//
//        // Продолжаем принимать соединения
//        if (!stopped_ && !g_signal_received) {
//            Listen();
//        }
//        });
//}

std::vector<std::string> BeastHttpServer::ParseSearchQuery(const std::string& query) {
    std::vector<std::string> words;
    std::stringstream ss(query);
    std::string word;

    while (ss >> word) {
        // Очищаем слово от знаков препинания и приводим к нижнему регистру
        std::string clean_word;
        for (char c : word) {
            if (std::isalnum(c)) {
                clean_word += std::tolower(c);
            }
        }

        // Добавляем только валидные слова
        if (clean_word.length() >= 3 && clean_word.length() <= 32) {
            words.push_back(clean_word);
        }
    }

    return words;
}

void BeastHttpServer::HandleRequest(http::request<http::string_body>&& req,
    std::shared_ptr<tcp::socket> socket) {
    http::response<http::string_body> res;

    try {
        if (req.method() == http::verb::get) {
            // Обработка GET запроса (форма поиска)
            if (req.target() == "/" || req.target() == "/search") {
                std::string query;

                // Извлекаем параметры запроса из URL
                size_t query_pos = req.target().find('?');
                if (query_pos != std::string::npos) {
                    std::string query_str = req.target().substr(query_pos + 1);
                    size_t q_pos = query_str.find("q=");
                    if (q_pos != std::string::npos) {
                        query = query_str.substr(q_pos + 2);
                        size_t amp_pos = query.find('&');
                        if (amp_pos != std::string::npos) {
                            query = query.substr(0, amp_pos);
                        }

                        // URL decode (простая версия)
                        std::string decoded_query;
                        for (size_t i = 0; i < query.length(); ++i) {
                            if (query[i] == '%' && i + 2 < query.length()) {
                                int hex_value;
                                std::stringstream hex_stream;
                                hex_stream << std::hex << query.substr(i + 1, 2);
                                hex_stream >> hex_value;
                                decoded_query += static_cast<char>(static_cast<unsigned char>(hex_value));
                                i += 2;
                            }
                            else {
                                decoded_query += query[i];
                            }
                        }
                        query = decoded_query;
                    }
                }

                res = { http::status::ok, req.version() };
                res.set(http::field::server, "SearchEngine/1.0");
                res.set(http::field::content_type, "text/html");
                res.body() = GenerateSearchPage(query);
                res.prepare_payload();
            }
            else {
                // 404 Not Found
                res = { http::status::not_found, req.version() };
                res.set(http::field::server, "SearchEngine/1.0");
                res.set(http::field::content_type, "text/html");
                res.body() = GenerateErrorPage("Page not found");
                res.prepare_payload();
            }
        }
        else if (req.method() == http::verb::post && req.target() == "/search") {
            // Обработка POST запроса (поиск)
            std::string query;

            // Парсим тело запроса
            std::string body = req.body();
            size_t pos = body.find("q=");
            if (pos != std::string::npos) {
                query = body.substr(pos + 2);
                // Декодируем URL-encoded строку (упрощенно)
                std::replace(query.begin(), query.end(), '+', ' ');
                // Обрезаем до первого &
                size_t amp_pos = query.find('&');
                if (amp_pos != std::string::npos) {
                    query = query.substr(0, amp_pos);
                }

                // URL decode (простая версия)
                std::string decoded_query;
                for (size_t i = 0; i < query.length(); ++i) {
                    if (query[i] == '%' && i + 2 < query.length()) {
                        int hex_value;
                        std::stringstream hex_stream;
                        hex_stream << std::hex << query.substr(i + 1, 2);
                        hex_stream >> hex_value;
                        decoded_query += static_cast<char>(static_cast<unsigned char>(hex_value));
                        i += 2;
                    }
                    else {
                        decoded_query += query[i];
                    }
                }
                query = decoded_query;
            }

            // Выполняем поиск - используем нашу функцию парсинга
            auto words = ParseSearchQuery(query);
            auto results = db_.SearchDocuments(words, config_.GetMaxResults());

            res = { http::status::ok, req.version() };
            res.set(http::field::server, "SearchEngine/1.0");
            res.set(http::field::content_type, "text/html");
            res.body() = GenerateResultsPage(results, query);
            res.prepare_payload();
        }
        else {
            // 404 Not Found
            res = { http::status::not_found, req.version() };
            res.set(http::field::server, "SearchEngine/1.0");
            res.set(http::field::content_type, "text/html");
            res.body() = GenerateErrorPage("Page not found");
            res.prepare_payload();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error handling request: " << e.what() << std::endl;
        res = { http::status::internal_server_error, req.version() };
        res.set(http::field::server, "SearchEngine/1.0");
        res.set(http::field::content_type, "text/html");
        res.body() = GenerateErrorPage("Internal server error");
        res.prepare_payload();
    }

    // Отправляем ответ
    SendResponse(socket, std::move(res));
}

void BeastHttpServer::SendResponse(std::shared_ptr<tcp::socket> socket,
    http::response<http::string_body>&& response) {
    try {
        http::write(*socket, response);

        // Закрываем соединение
        beast::error_code ec;
        socket->shutdown(tcp::socket::shutdown_send, ec);

        if (ec && ec != beast::errc::not_connected) {
            std::cerr << "Error shutting down socket: " << ec.message() << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error sending response: " << e.what() << std::endl;
    }
}

std::string BeastHttpServer::GenerateSearchPage(const std::string& query) {
    std::stringstream html;
    html << "<!DOCTYPE html>"
        << "<html>"
        << "<head>"
        << "<title>Search Engine</title>"
        << "<meta charset='UTF-8'>"
        << "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
        << "<style>"
        << "body { font-family: Arial, sans-serif; margin: 40px; background-color: #f5f5f5; }"
        << ".container { max-width: 800px; margin: 0 auto; background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }"
        << ".search-box { text-align: center; margin-bottom: 30px; }"
        << "h1 { color: #4285f4; margin-bottom: 30px; }"
        << "input[type=text] { width: 70%; padding: 12px; font-size: 16px; border: 1px solid #ddd; border-radius: 24px; outline: none; }"
        << "input[type=text]:focus { border-color: #4285f4; box-shadow: 0 0 5px rgba(66, 133, 244, 0.3); }"
        << "input[type=submit] { padding: 12px 24px; font-size: 16px; background-color: #4285f4; color: white; border: none; border-radius: 24px; cursor: pointer; margin-left: 10px; }"
        << "input[type=submit]:hover { background-color: #3367d6; }"
        << ".footer { text-align: center; margin-top: 40px; color: #666; font-size: 14px; }"
        << "</style>"
        << "</head>"
        << "<body>"
        << "<div class='container'>"
        << "<div class='search-box'>"
        << "<h1>Search Engine</h1>"
        << "<form method='post' action='/search'>"
        << "<input type='text' name='q' value='" << query << "' placeholder='Enter your search query...'>"
        << "<input type='submit' value='Search'>"
        << "</form>"
        << "</div>"
        << "<div class='footer'>"
        << "Built with C++, Boost.Beast and PostgreSQL"
        << "</div>"
        << "</div>"
        << "</body>"
        << "</html>";
    return html.str();
}

std::string BeastHttpServer::GenerateResultsPage(const std::vector<SearchResult>& results, const std::string& query) {
    std::stringstream html;
    html << "<!DOCTYPE html>"
        << "<html>"
        << "<head>"
        << "<title>Search Results for \"" << query << "\"</title>"
        << "<meta charset='UTF-8'>"
        << "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
        << "<style>"
        << "body { font-family: Arial, sans-serif; margin: 0; padding: 0; background-color: #f5f5f5; }"
        << ".header { background: white; padding: 20px; border-bottom: 1px solid #e0e0e0; }"
        << ".container { max-width: 800px; margin: 0 auto; }"
        << ".search-box { display: flex; align-items: center; }"
        << "h1 { color: #4285f4; margin: 0; margin-right: 30px; font-size: 24px; }"
        << "input[type=text] { flex: 1; padding: 12px; font-size: 16px; border: 1px solid #ddd; border-radius: 24px; outline: none; }"
        << "input[type=text]:focus { border-color: #4285f4; }"
        << "input[type=submit] { padding: 12px 24px; font-size: 16px; background-color: #4285f4; color: white; border: none; border-radius: 24px; cursor: pointer; margin-left: 10px; }"
        << ".results { background: white; margin: 20px auto; max-width: 800px; padding: 20px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }"
        << ".result { margin-bottom: 25px; padding-bottom: 20px; border-bottom: 1px solid #f0f0f0; }"
        << ".result:last-child { border-bottom: none; margin-bottom: 0; }"
        << ".result-title { font-size: 18px; color: #1a0dab; text-decoration: none; font-weight: normal; margin: 0 0 5px 0; display: block; }"
        << ".result-title:hover { text-decoration: underline; }"
        << ".result-url { color: #006621; font-size: 14px; margin: 0 0 8px 0; }"
        << ".result-snippet { color: #545454; font-size: 14px; line-height: 1.4; margin: 0; }"
        << ".result-relevance { color: #70757a; font-size: 12px; margin-top: 5px; }"
        << ".no-results { text-align: center; padding: 40px; color: #70757a; }"
        << ".results-count { color: #70757a; font-size: 14px; margin-bottom: 20px; }"
        << "</style>"
        << "</head>"
        << "<body>"
        << "<div class='header'>"
        << "<div class='container'>"
        << "<div class='search-box'>"
        << "<h1>Search Engine</h1>"
        << "<form method='post' action='/search' style='display: flex; flex: 1;'>"
        << "<input type='text' name='q' value='" << query << "'>"
        << "<input type='submit' value='Search'>"
        << "</form>"
        << "</div>"
        << "</div>"
        << "</div>"
        << "<div class='results'>"
        << "<div class='results-count'>Found " << results.size() << " results for \"" << query << "\"</div>";

    if (results.empty()) {
        html << "<div class='no-results'>"
            << "<h3>No results found</h3>"
            << "<p>Try different keywords or check your spelling.</p>"
            << "</div>";
    }
    else {
        for (const auto& result : results) {
            html << "<div class='result'>"
                << "<a class='result-title' href='" << result.url << "' target='_blank'>"
                << (result.title.empty() ? result.url : result.title)
                << "</a>"
                << "<div class='result-url'>" << result.url << "</div>"
                << "<div class='result-snippet'>" << result.snippet << "</div>"
                << "<div class='result-relevance'>Relevance score: " << result.relevance << "</div>"
                << "</div>";
        }
    }

    html << "</div>"
        << "</body>"
        << "</html>";
    return html.str();
}

std::string BeastHttpServer::GenerateErrorPage(const std::string& message) {
    std::stringstream html;
    html << "<!DOCTYPE html>"
        << "<html>"
        << "<head>"
        << "<title>Error</title>"
        << "<meta charset='UTF-8'>"
        << "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
        << "<style>"
        << "body { font-family: Arial, sans-serif; margin: 40px; background-color: #f5f5f5; }"
        << ".container { max-width: 600px; margin: 0 auto; background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); text-align: center; }"
        << ".error { color: #d93025; font-size: 18px; margin-bottom: 20px; }"
        << "a { color: #4285f4; text-decoration: none; }"
        << "a:hover { text-decoration: underline; }"
        << "</style>"
        << "</head>"
        << "<body>"
        << "<div class='container'>"
        << "<h1>Error</h1>"
        << "<div class='error'>" << message << "</div>"
        << "<p><a href='/'>Back to search</a></p>"
        << "</div>"
        << "</body>"
        << "</html>";
    return html.str();
}