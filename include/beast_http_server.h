#ifndef BEAST_HTTP_SERVER_H
#define BEAST_HTTP_SERVER_H

#include "config.h"
#include "database.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <atomic>
#include <vector>
#include <thread>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

// Глобальный флаг для обработки сигналов
extern std::atomic<bool> g_signal_received;

class BeastHttpServer {
public:
    BeastHttpServer(Config& config, Database& db);
    ~BeastHttpServer();

    void Start();
    void Stop();

private:
    void Run();
    void CreateWorkerThreads();
    void Listen();
    void HandleRequest(http::request<http::string_body>&& req, std::shared_ptr<tcp::socket> socket);
    void SendResponse(std::shared_ptr<tcp::socket> socket, http::response<http::string_body>&& response);

    std::vector<std::string> ParseSearchQuery(const std::string& query);

    std::string GenerateSearchPage(const std::string& query);
    std::string GenerateResultsPage(const std::vector<SearchResult>& results, const std::string& query);
    std::string GenerateErrorPage(const std::string& message);

    Config& config_;
    Database& db_;
    net::io_context ioc_;
    tcp::acceptor acceptor_;
    std::vector<std::thread> worker_threads_;
    std::atomic<bool> stopped_{ false };
};

#endif // BEAST_HTTP_SERVER_H