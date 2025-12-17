#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <fstream>

namespace FoxBridge {

struct Config {
    std::string database_path;
    std::string api_key;
    int port = 8787;
    std::string cloudflare_token;
    std::string log_level = "info";
    std::string log_path = "C:\\ProgramData\\FoxBridgeAgent\\logs";
    std::string index_policy = "auto";  // auto | manual_only
    std::string maintenance_window = "02:00-04:00";
    int max_retry_attempts = 3;
    int connection_timeout = 30;
    
    static Config load(const std::string& config_path) {
        Config config;
        std::ifstream file(config_path);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open config file: " + config_path);
        }
        
        nlohmann::json j;
        file >> j;
        
        config.database_path = j.value("database_path", "");
        config.api_key = j.value("api_key", "");
        config.port = j.value("port", 8787);
        config.cloudflare_token = j.value("cloudflare_token", "");
        config.log_level = j.value("log_level", "info");
        config.log_path = j.value("log_path", "C:\\ProgramData\\FoxBridgeAgent\\logs");
        config.index_policy = j.value("index_policy", "auto");
        config.maintenance_window = j.value("maintenance_window", "02:00-04:00");
        config.max_retry_attempts = j.value("max_retry_attempts", 3);
        config.connection_timeout = j.value("connection_timeout", 30);
        
        return config;
    }
    
    void validate() const {
        if (database_path.empty()) {
            throw std::runtime_error("database_path is required");
        }
        if (api_key.empty()) {
            throw std::runtime_error("api_key is required");
        }
        if (port < 1024 || port > 65535) {
            throw std::runtime_error("port must be between 1024 and 65535");
        }
    }
};

} // namespace FoxBridge
