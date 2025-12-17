#include "Config.h"
#include "DatabaseManager.h"
#include "HttpServer.h"
#include "WindowsService.h"
#include "CloudflareTunnel.h"
#include "IndexMaintenance.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include <iostream>
#include <filesystem>

using namespace FoxBridge;

// Global instances
std::shared_ptr<DatabaseManager> g_db_manager;
std::shared_ptr<HttpServer> g_http_server;
std::shared_ptr<CloudflareTunnel> g_tunnel;
std::shared_ptr<IndexMaintenance> g_maintenance;
Config g_config;

void initializeLogging(const Config& config) {
    try {
        // Create logs directory
        std::filesystem::create_directories(config.log_path);
        
        // Rotating file sink
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            config.log_path + "\\foxbridge.log", 
            1024 * 1024 * 10,  // 10MB
            5                   // 5 files
        );
        
        // Console sink
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        
        // Create logger with both sinks
        std::vector<spdlog::sink_ptr> sinks = {file_sink, console_sink};
        auto logger = std::make_shared<spdlog::logger>("foxbridge", sinks.begin(), sinks.end());
        
        // Set as default logger
        spdlog::set_default_logger(logger);
        
        // Set log level
        if (config.log_level == "debug") {
            spdlog::set_level(spdlog::level::debug);
        } else if (config.log_level == "info") {
            spdlog::set_level(spdlog::level::info);
        } else if (config.log_level == "warn") {
            spdlog::set_level(spdlog::level::warn);
        } else if (config.log_level == "error") {
            spdlog::set_level(spdlog::level::err);
        }
        
        spdlog::info("Logging initialized: {}", config.log_path);
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize logging: " << e.what() << std::endl;
    }
}

void startApplication() {
    try {
        spdlog::info("=== FoxBridgeAgent Starting ===");
        spdlog::info("Version: 1.0.0");
        spdlog::info("Database Path: {}", g_config.database_path);
        spdlog::info("HTTP Port: {}", g_config.port);
        
        // 1. Initialize Database Manager
        spdlog::info("Initializing Database Manager...");
        g_db_manager = std::make_shared<DatabaseManager>(g_config.database_path);
        
        if (!g_db_manager->isConnected()) {
            throw std::runtime_error("Failed to connect to database");
        }
        
        // 2. Initialize HTTP Server
        spdlog::info("Initializing HTTP Server...");
        g_http_server = std::make_shared<HttpServer>(g_config, g_db_manager);
        g_http_server->start();
        
        // 3. Initialize Cloudflare Tunnel
        if (!g_config.cloudflare_token.empty()) {
            spdlog::info("Initializing Cloudflare Tunnel...");
            g_tunnel = std::make_shared<CloudflareTunnel>(g_config.cloudflare_token, 
                                                          g_config.port);
            if (g_tunnel->start()) {
                g_tunnel->enableWatchdog(30);  // Check every 30 seconds
            } else {
                spdlog::warn("Failed to start Cloudflare Tunnel");
            }
        } else {
            spdlog::info("Cloudflare Tunnel token not configured, skipping");
        }
        
        // 4. Initialize Index Maintenance
        spdlog::info("Initializing Index Maintenance...");
        g_maintenance = std::make_shared<IndexMaintenance>(g_db_manager, 
                                                           g_config.maintenance_window);
        g_maintenance->start();
        
        spdlog::info("=== FoxBridgeAgent Started Successfully ===");
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to start application: {}", e.what());
        throw;
    }
}

void stopApplication() {
    try {
        spdlog::info("=== FoxBridgeAgent Stopping ===");
        
        // Graceful shutdown in reverse order
        if (g_maintenance) {
            g_maintenance->stop();
            g_maintenance.reset();
        }
        
        if (g_tunnel) {
            g_tunnel->stop();
            g_tunnel.reset();
        }
        
        if (g_http_server) {
            g_http_server->stop();
            g_http_server.reset();
        }
        
        // Database manager will disconnect in destructor
        g_db_manager.reset();
        
        spdlog::info("=== FoxBridgeAgent Stopped ===");
        
    } catch (const std::exception& e) {
        spdlog::error("Error during shutdown: {}", e.what());
    }
}

int runAsConsole() {
    try {
        startApplication();
        
        std::cout << "\nFoxBridgeAgent is running. Press Ctrl+C to stop...\n" << std::endl;
        
        // Wait for Ctrl+C
        while (true) {
            Sleep(1000);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

int runAsService() {
    WindowsService service("FoxBridgeAgent", startApplication, stopApplication);
    service.run();
    return 0;
}

void printUsage() {
    std::cout << "FoxBridgeAgent - HTTP API Server for Visual FoxPro / ExpressD\n\n"
              << "Usage:\n"
              << "  FoxBridgeAgent.exe [options]\n\n"
              << "Options:\n"
              << "  --console              Run as console application (default for testing)\n"
              << "  --service              Run as Windows Service (default for production)\n"
              << "  --install              Install as Windows Service\n"
              << "  --uninstall            Uninstall Windows Service\n"
              << "  --start                Start Windows Service\n"
              << "  --stop                 Stop Windows Service\n"
              << "  --config <path>        Specify config file path (default: C:\\ProgramData\\FoxBridgeAgent\\config.json)\n"
              << "  --help                 Show this help message\n\n"
              << "Example:\n"
              << "  FoxBridgeAgent.exe --console\n"
              << "  FoxBridgeAgent.exe --install\n"
              << "  FoxBridgeAgent.exe --service\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    std::string config_path = "C:\\ProgramData\\FoxBridgeAgent\\config.json";
    std::string mode = "service";
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        } else if (arg == "--console") {
            mode = "console";
        } else if (arg == "--service") {
            mode = "service";
        } else if (arg == "--install") {
            char exe_path[MAX_PATH];
            GetModuleFileNameA(nullptr, exe_path, MAX_PATH);
            
            if (WindowsService::install("FoxBridgeAgent", 
                                       "FoxBridge Agent - ExpressD API Server",
                                       std::string(exe_path) + " --service")) {
                std::cout << "Service installed successfully" << std::endl;
                return 0;
            } else {
                std::cerr << "Failed to install service" << std::endl;
                return 1;
            }
        } else if (arg == "--uninstall") {
            if (WindowsService::uninstall("FoxBridgeAgent")) {
                std::cout << "Service uninstalled successfully" << std::endl;
                return 0;
            } else {
                std::cerr << "Failed to uninstall service" << std::endl;
                return 1;
            }
        } else if (arg == "--start") {
            if (WindowsService::start("FoxBridgeAgent")) {
                std::cout << "Service started successfully" << std::endl;
                return 0;
            } else {
                std::cerr << "Failed to start service" << std::endl;
                return 1;
            }
        } else if (arg == "--stop") {
            if (WindowsService::stop("FoxBridgeAgent")) {
                std::cout << "Service stopped successfully" << std::endl;
                return 0;
            } else {
                std::cerr << "Failed to stop service" << std::endl;
                return 1;
            }
        } else if (arg == "--config" && i + 1 < argc) {
            config_path = argv[++i];
        }
    }
    
    // Load configuration
    try {
        g_config = Config::load(config_path);
        g_config.validate();
    } catch (const std::exception& e) {
        std::cerr << "Configuration error: " << e.what() << std::endl;
        std::cerr << "Please check: " << config_path << std::endl;
        return 1;
    }
    
    // Initialize logging
    initializeLogging(g_config);
    
    // Run in selected mode
    if (mode == "console") {
        return runAsConsole();
    } else {
        return runAsService();
    }
}
