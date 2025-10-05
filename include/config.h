#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <unordered_map>

class Config {
public:
    Config() = default;

    bool Load(const std::string& filename);

    // Database settings
    std::string GetDatabaseHost() const { return db_host_; }
    int GetDatabasePort() const { return db_port_; }
    std::string GetDatabaseName() const { return db_name_; }
    std::string GetDatabaseUser() const { return db_user_; }
    std::string GetDatabasePassword() const { return db_password_; }

    // Spider settings
    std::string GetStartUrl() const { return start_url_; }
    int GetMaxDepth() const { return max_depth_; }
    int GetThreadCount() const { return thread_count_; }
    int GetRequestTimeout() const { return request_timeout_; }
    std::string GetUserAgent() const { return user_agent_; }
    int GetDelayBetweenRequests() const { return delay_between_requests_; }

    // Server settings
    int GetServerPort() const { return server_port_; }
    int GetMaxResults() const { return max_results_; }
    std::string GetServerHost() const { return server_host_; }
    int GetServerThreads() const { return server_threads_; }

private:
    // Database
    std::string db_host_ = "localhost";
    int db_port_ = 5432;
    std::string db_name_ = "search_engine";
    std::string db_user_ = "postgres";
    std::string db_password_ = "admin";

    // Spider
    std::string start_url_ = "https://example.com";
    int max_depth_ = 1;
    int thread_count_ = 2;
    int request_timeout_ = 30;
    std::string user_agent_ = "SearchEngineBot/1.0";
    int delay_between_requests_ = 100;

    // Server
    int server_port_ = 8080;
    int max_results_ = 10;
    std::string server_host_ = "0.0.0.0";
    int server_threads_ = 4;
};

#endif // CONFIG_H