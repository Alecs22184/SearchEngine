#include "http_client.h"
#include <iostream>
#include <regex>

HttpClient::HttpClient()
    : ssl_ctx_(ssl::context::tlsv12_client) {
    // ��������� SSL ���������
    ssl_ctx_.set_default_verify_paths();
    ssl_ctx_.set_verify_mode(ssl::verify_none); // ��� ������������ ��������� �������� ������������
}

HttpClient::~HttpClient() {
    // ������� �������� �����-������
    if (!ioc_.stopped()) {
        ioc_.stop();
    }
}

HttpClient::HttpResponse HttpClient::DownloadPage(const std::string& url) {
    HttpResponse response;

    try {
        std::string host, port, target;
        std::string resolved_url = ResolveUrl(url, host, port, target);

        if (resolved_url.empty()) {
            response.status_code = 400;
            std::cerr << "Invalid URL: " << url << std::endl;
            return response;
        }

        std::cout << "Downloading: " << url << " (host: " << host << ", port: " << port << ", target: " << target << ")" << std::endl;

        // ������� ��������
        tcp::resolver resolver(ioc_);

        // ��������� ����
        auto const results = resolver.resolve(host, port);

        // ���������� ��������
        bool use_ssl = (port == "443" || url.find("https://") == 0);

        if (use_ssl) {
            // SSL ����������
            beast::ssl_stream<beast::tcp_stream> stream(ioc_, ssl_ctx_);

            // ������������� SNI Hostname (����� ��� HTTPS)
            if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
                beast::error_code ec{ static_cast<int>(::ERR_get_error()), net::error::get_ssl_category() };
                throw beast::system_error{ ec };
            }

            // ������������� TCP ����������
            beast::get_lowest_layer(stream).connect(results);

            // ��������� SSL handshake
            stream.handshake(ssl::stream_base::client);

            // ������������� �������
            beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(timeout_));

            // ������� HTTP GET ������
            http::request<http::string_body> req{ http::verb::get, target, 11 };
            req.set(http::field::host, host);
            req.set(http::field::user_agent, user_agent_);
            req.set(http::field::accept, "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");

            // ���������� HTTP ������
            http::write(stream, req);

            // �������� �����
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // ������ �����
            http::read(stream, buffer, res);

            // ��������� ������ ���
            response.status_code = res.result_int();

            // ����������� ���� ������ � ������
            response.content = beast::buffers_to_string(res.body().data());

            // �������� Content-Type
            auto content_type = res.find(http::field::content_type);
            if (content_type != res.end()) {
                response.content_type = content_type->value();
            }

            // Gracefully close the SSL stream
            beast::error_code ec;
            stream.shutdown(ec);

            // SSL shutdown ����� ���������� ������, ��� ���������
            if (ec == net::error::eof || ec == ssl::error::stream_truncated) {
                ec = {};
            }
            if (ec && ec != beast::errc::not_connected) {
                std::cerr << "Error shutting down SSL stream: " << ec.message() << std::endl;
            }

        }
        else {
            // ������� HTTP ����������
            beast::tcp_stream stream(ioc_);

            // ������������� ����������
            stream.connect(results);

            // ������������� �������
            stream.expires_after(std::chrono::seconds(timeout_));

            // ������� HTTP GET ������
            http::request<http::string_body> req{ http::verb::get, target, 11 };
            req.set(http::field::host, host);
            req.set(http::field::user_agent, user_agent_);
            req.set(http::field::accept, "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");

            // ���������� HTTP ������
            http::write(stream, req);

            // �������� �����
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // ������ �����
            http::read(stream, buffer, res);

            // ��������� ������ ���
            response.status_code = res.result_int();

            // ����������� ���� ������ � ������
            response.content = beast::buffers_to_string(res.body().data());

            // �������� Content-Type
            auto content_type = res.find(http::field::content_type);
            if (content_type != res.end()) {
                response.content_type = content_type->value();
            }

            // Gracefully close the socket
            beast::error_code ec;
            stream.socket().shutdown(tcp::socket::shutdown_both, ec);

            if (ec && ec != beast::errc::not_connected) {
                std::cerr << "Error shutting down socket: " << ec.message() << std::endl;
            }
        }

        std::cout << "Successfully downloaded " << url << " (" << response.content.size() << " bytes, status: " << response.status_code << ")" << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "HTTP Client Error downloading " << url << ": " << e.what() << std::endl;
        response.status_code = 500;
    }

    return response;
}

bool HttpClient::IsValidUrl(const std::string& url) {
    try {
        std::regex url_regex(
            R"(^(https?://)?([\w-]+\.)+[\w-]+(/[\w\-./?%&=]*)?$)",
            std::regex::icase
        );
        return std::regex_match(url, url_regex);
    }
    catch (const std::exception& e) {
        std::cerr << "Error validating URL: " << e.what() << std::endl;
        return false;
    }
}

std::string HttpClient::ResolveUrl(const std::string& url, std::string& host, std::string& port, std::string& target) {
    try {
        std::regex url_regex(
            R"(^(https?)://([^:/]+)(:([0-9]+))?(/.*)?$)",
            std::regex::icase
        );

        std::smatch match;
        if (std::regex_match(url, match, url_regex)) {
            std::string protocol = match[1].str();
            host = match[2].str();
            std::string port_str = match[4].str();
            target = match[5].str();

            if (target.empty()) {
                target = "/";
            }

            if (port_str.empty()) {
                port = (protocol == "https") ? "443" : "80";
            }
            else {
                port = port_str;
            }

            return url;
        }

        // ��������� URL ��� ��������� (��������� http �� ���������)
        std::regex no_protocol_regex(
            R"(^([\w-]+\.)+[\w-]+(/[\w\-./?%&=]*)?$)",
            std::regex::icase
        );

        if (std::regex_match(url, no_protocol_regex)) {
            host = url;
            port = "80";
            target = "/";
            return "http://" + url;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error resolving URL: " << e.what() << std::endl;
    }

    return "";
}