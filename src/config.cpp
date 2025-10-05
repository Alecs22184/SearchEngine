#include "config.h"
#include <fstream>
#include <sstream>
#include <iostream>

bool Config::Load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Cannot open config file: " << filename << std::endl;
        return false;
    }

    std::string line;
    std::string current_section;

    while (std::getline(file, line)) {
        // Remove comments
        size_t comment_pos = line.find(';');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }

        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        if (line.empty()) continue;

        // Section
        if (line[0] == '[' && line[line.size() - 1] == ']') {
            current_section = line.substr(1, line.size() - 2);
            continue;
        }

        // Key-value pair
        size_t equals_pos = line.find('=');
        if (equals_pos != std::string::npos) {
            std::string key = line.substr(0, equals_pos);
            std::string value = line.substr(equals_pos + 1);

            // Trim key and value
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            // Parse values based on section and key
            if (current_section == "database") {
                if (key == "host") db_host_ = value;
                else if (key == "port") db_port_ = std::stoi(value);
                else if (key == "dbname") db_name_ = value;
                else if (key == "user") db_user_ = value;
                else if (key == "password") db_password_ = value;
            }
            else if (current_section == "spider") {
                if (key == "start_url") start_url_ = value;
                else if (key == "max_depth") max_depth_ = std::stoi(value);
                else if (key == "thread_count") thread_count_ = std::stoi(value);
                else if (key == "request_timeout") request_timeout_ = std::stoi(value);
                else if (key == "user_agent") user_agent_ = value;
                else if (key == "delay_between_requests") delay_between_requests_ = std::stoi(value);
            }
            else if (current_section == "search_server") {
                if (key == "port") server_port_ = std::stoi(value);
                else if (key == "max_results") max_results_ = std::stoi(value);
                else if (key == "host") server_host_ = value;
                else if (key == "threads") server_threads_ = std::stoi(value);
            }
        }
    }

    file.close();
    std::cout << "Config loaded successfully from: " << filename << std::endl;

    // ¬ывод загруженных настроек дл€ отладки
    std::cout << "Database: " << db_host_ << ":" << db_port_ << "/" << db_name_ << std::endl;
    std::cout << "Spider start URL: " << start_url_ << " (depth: " << max_depth_ << ")" << std::endl;
    std::cout << "Server port: " << server_port_ << std::endl;

    return true;
}