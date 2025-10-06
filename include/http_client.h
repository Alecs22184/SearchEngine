#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <string>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/ssl.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;

class HttpClient {
public:
    struct HttpResponse {
        int status_code = 0;
        std::string content;
        std::string content_type;
    };

    HttpClient();
    ~HttpClient();

    HttpResponse DownloadPage(const std::string& url);
    bool IsValidUrl(const std::string& url);

    void SetTimeout(int timeout) { timeout_ = timeout; }
    void SetUserAgent(const std::string& user_agent) { user_agent_ = user_agent; }

private:
    std::string ResolveUrl(const std::string& url, std::string& host, std::string& port, std::string& target);

    net::io_context ioc_;
    ssl::context ssl_ctx_;
    int timeout_ = 30;
    std::string user_agent_ = "SearchEngineBot/1.0";
};

#endif // HTTP_CLIENT_H