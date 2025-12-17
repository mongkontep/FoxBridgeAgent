#pragma once

#include <string>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <nlohmann/json.hpp>
#include "DatabaseManager.h"
#include "Config.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace FoxBridge {

class HttpServer {
public:
    HttpServer(const Config& config, std::shared_ptr<DatabaseManager> db_manager);
    ~HttpServer();
    
    void start();
    void stop();
    bool isRunning() const { return running_; }
    
private:
    Config config_;
    std::shared_ptr<DatabaseManager> db_manager_;
    std::atomic<bool> running_;
    std::unique_ptr<std::thread> server_thread_;
    
    void run();
    void handleRequest(http::request<http::string_body>& req, 
                      http::response<http::string_body>& res);
    
    bool authenticate(const http::request<http::string_body>& req);
    nlohmann::json handleHealth();
    
    // DBF file operations
    nlohmann::json handleExportJSON(const std::string& filename, const std::string& docnum = "");
    nlohmann::json handleExportCSV(const std::string& filename, const std::string& docnum = "");
    nlohmann::json handleSearch(const std::string& filename, const std::string& queryParams);
    std::string handleViewHTML(const std::string& filename);
    nlohmann::json handleFindDocnum(const std::string& docnum);
    nlohmann::json handleDirectLookup(const std::string& docnum);
    
    // CRUD operations
    nlohmann::json handleAdd(const std::string& filename, const nlohmann::json& body);
    void sendCSVResponse(http::response<http::string_body>& res, const std::string& csv,
                        const std::string& filename);
    void sendHTMLResponse(http::response<http::string_body>& res, const std::string& html);
    
    std::string extractFilename(const std::string& path);
    std::string sanitizeFilename(const std::string& filename);
    std::map<std::string, std::string> parseQueryString(const std::string& queryString);
    nlohmann::json handleUpdate(const std::string& filename, const nlohmann::json& body);
    nlohmann::json handleDelete(const std::string& filename, const nlohmann::json& body);
    nlohmann::json handleUndelete(const std::string& filename, const nlohmann::json& body);
    nlohmann::json handlePack(const std::string& filename);
    
    // Maintenance
    nlohmann::json handleReindex(const std::string& filename);
    nlohmann::json handleIndexStatus(const std::string& filename);
    
    void sendJsonResponse(http::response<http::string_body>& res, int status, 
                         const nlohmann::json& json);
    void sendError(http::response<http::string_body>& res, int status, 
                  const std::string& message);
};

} // namespace FoxBridge
