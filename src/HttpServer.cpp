#include "HttpServer.h"
#include <spdlog/spdlog.h>
#include <regex>

namespace FoxBridge {

HttpServer::HttpServer(const Config& config, std::shared_ptr<DatabaseManager> db_manager)
    : config_(config)
    , db_manager_(db_manager)
    , running_(false) {
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    if (running_) {
        spdlog::warn("HTTP server already running");
        return;
    }
    
    running_ = true;
    server_thread_ = std::make_unique<std::thread>(&HttpServer::run, this);
    spdlog::info("HTTP server started on port {}", config_.port);
}

void HttpServer::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    if (server_thread_ && server_thread_->joinable()) {
        server_thread_->join();
    }
    spdlog::info("HTTP server stopped");
}

void HttpServer::run() {
    try {
        net::io_context ioc{1};
        tcp::acceptor acceptor{ioc, {net::ip::make_address("127.0.0.1"), 
                                     static_cast<unsigned short>(config_.port)}};
        
        while (running_) {
            tcp::socket socket{ioc};
            acceptor.accept(socket);
            
            beast::flat_buffer buffer;
            http::request<http::string_body> req;
            http::read(socket, buffer, req);
            
            http::response<http::string_body> res;
            res.version(req.version());
            res.keep_alive(false);
            
            handleRequest(req, res);
            
            http::write(socket, res);
        }
        
    } catch (const std::exception& e) {
        spdlog::error("HTTP server error: {}", e.what());
    }
}

bool HttpServer::authenticate(const http::request<http::string_body>& req) {
    auto it = req.find("X-API-Key");
    if (it == req.end()) {
        return false;
    }
    return it->value() == config_.api_key;
}

void HttpServer::handleRequest(http::request<http::string_body>& req, 
                               http::response<http::string_body>& res) {
    
    std::string target = std::string(req.target());
    std::string method = std::string(req.method_string());
    
    spdlog::info("Request: {} {}", method, target);
    
    // Health check (no auth required)
    if (target == "/health" && method == "GET") {
        sendJsonResponse(res, 200, handleHealth());
        return;
    }
    
    // Auth required for all other endpoints
    if (!authenticate(req)) {
        sendError(res, 401, "Unauthorized: Invalid or missing X-API-Key");
        return;
    }
    
    // Parse routes using regex
    std::regex json_all_pattern(R"(^/api/dbf/json/([^/]+\.dbf)$)");
    std::regex json_docnum_pattern(R"(^/api/dbf/json/([^/]+\.dbf)/([^/]+)$)");
    std::regex csv_all_pattern(R"(^/api/dbf/csv/([^/]+\.dbf)$)");
    std::regex csv_docnum_pattern(R"(^/api/dbf/csv/([^/]+\.dbf)/([^/]+)$)");
    std::regex search_pattern(R"(^/api/dbf/search/([^/]+\.dbf))");
    std::regex view_pattern(R"(^/view/([^/]+\.dbf)$)");
    std::regex docnum_pattern(R"(^/docnum/([^/]+)$)");
    std::regex direct_lookup_pattern(R"(^/([A-Z]{2}\d{7})$)");  // e.g., HP0000001
    
    std::regex add_pattern(R"(^/api/dbf/add/([^/]+\.dbf)$)");
    std::regex update_pattern(R"(^/api/dbf/update/([^/]+\.dbf)$)");
    std::regex delete_pattern(R"(^/api/dbf/delete/([^/]+\.dbf)$)");
    std::regex undelete_pattern(R"(^/api/dbf/undelete/([^/]+\.dbf)$)");
    std::regex pack_pattern(R"(^/api/dbf/pack/([^/]+\.dbf)$)");
    std::regex reindex_pattern(R"(^/api/dbf/maintenance/reindex/([^/]+\.dbf)$)");
    std::regex status_pattern(R"(^/api/dbf/maintenance/status/([^/]+\.dbf)$)");
    
    std::smatch matches;
    
    try {
        nlohmann::json body;
        if (!req.body().empty()) {
            body = nlohmann::json::parse(req.body());
        }
        
        // GET /api/dbf/json/filename.dbf - Export all as JSON
        if (std::regex_match(target, matches, json_all_pattern) && method == "GET") {
            std::string filename = matches[1];
            sendJsonResponse(res, 200, handleExportJSON(filename));
            return;
        }
        
        // GET /api/dbf/json/filename.dbf/HP0000001 - Export filtered JSON
        if (std::regex_match(target, matches, json_docnum_pattern) && method == "GET") {
            std::string filename = matches[1];
            std::string docnum = matches[2];
            sendJsonResponse(res, 200, handleExportJSON(filename, docnum));
            return;
        }
        
        // GET /api/dbf/csv/filename.dbf - Export all as CSV
        if (std::regex_match(target, matches, csv_all_pattern) && method == "GET") {
            std::string filename = matches[1];
            auto result = handleExportCSV(filename);
            if (result["status"] == "success") {
                sendCSVResponse(res, result["data"].dump(), filename);
            } else {
                sendJsonResponse(res, 500, result);
            }
            return;
        }
        
        // GET /api/dbf/csv/filename.dbf/HP0000001 - Export filtered CSV
        if (std::regex_match(target, matches, csv_docnum_pattern) && method == "GET") {
            std::string filename = matches[1];
            std::string docnum = matches[2];
            auto result = handleExportCSV(filename, docnum);
            if (result["status"] == "success") {
                sendCSVResponse(res, result["data"].dump(), filename);
            } else {
                sendJsonResponse(res, 500, result);
            }
            return;
        }
        
        // GET /api/dbf/search/filename.dbf?field=value - Search
        if (std::regex_match(target, matches, search_pattern) && method == "GET") {
            std::string filename = matches[1];
            size_t query_pos = target.find('?');
            std::string query_params = (query_pos != std::string::npos) ? 
                                       target.substr(query_pos + 1) : "";
            sendJsonResponse(res, 200, handleSearch(filename, query_params));
            return;
        }
        
        // GET /view/filename.dbf - HTML view
        if (std::regex_match(target, matches, view_pattern) && method == "GET") {
            std::string filename = matches[1];
            sendHTMLResponse(res, handleViewHTML(filename));
            return;
        }
        
        // GET /docnum/HP0000001 - Find by docnum
        if (std::regex_match(target, matches, docnum_pattern) && method == "GET") {
            std::string docnum = matches[1];
            sendJsonResponse(res, 200, handleFindDocnum(docnum));
            return;
        }
        
        // GET /HP0000001 - Direct lookup
        if (std::regex_match(target, matches, direct_lookup_pattern) && method == "GET") {
            std::string docnum = matches[1];
            sendJsonResponse(res, 200, handleDirectLookup(docnum));
            return;
        }
        
        // POST /api/dbf/add/filename.dbf - Add record
        if (std::regex_match(target, matches, add_pattern) && method == "POST") {
            std::string filename = matches[1];
            sendJsonResponse(res, 200, handleAdd(filename, body));
            return;
        }
        
        // POST /api/dbf/update/filename.dbf - Update record
        if (std::regex_match(target, matches, update_pattern) && method == "POST") {
            std::string filename = matches[1];
            sendJsonResponse(res, 200, handleUpdate(filename, body));
            return;
        }
        
        // POST /api/dbf/delete/filename.dbf - Delete record
        if (std::regex_match(target, matches, delete_pattern) && method == "POST") {
            std::string filename = matches[1];
            sendJsonResponse(res, 200, handleDelete(filename, body));
            return;
        }
        
        // POST /api/dbf/undelete/filename.dbf - Undelete record
        if (std::regex_match(target, matches, undelete_pattern) && method == "POST") {
            std::string filename = matches[1];
            sendJsonResponse(res, 200, handleUndelete(filename, body));
            return;
        }
        
        // POST /api/dbf/pack/filename.dbf - Pack file
        if (std::regex_match(target, matches, pack_pattern) && method == "POST") {
            std::string filename = matches[1];
            sendJsonResponse(res, 200, handlePack(filename));
            return;
        }
        
        // POST /api/dbf/maintenance/reindex/filename.dbf
        if (std::regex_match(target, matches, reindex_pattern) && method == "POST") {
            std::string filename = matches[1];
            sendJsonResponse(res, 200, handleReindex(filename));
            return;
        }
        
        // GET /api/dbf/maintenance/status/filename.dbf
        if (std::regex_match(target, matches, status_pattern) && method == "GET") {
            std::string filename = matches[1];
            sendJsonResponse(res, 200, handleIndexStatus(filename));
            return;
        }
        
        sendError(res, 404, "Not Found");
        
    } catch (const std::exception& e) {
        sendError(res, 500, std::string("Internal Server Error: ") + e.what());
    }
}

nlohmann::json HttpServer::handleHealth() {
    return {
        {"status", "success"},
        {"msg", "FoxBridgeAgent is running"},
        {"data", {
            {"version", "1.0.0"},
            {"database_connected", db_manager_->isConnected()},
            {"timestamp", std::time(nullptr)}
        }},
        {"index", "ok"},
        {"warnings", nlohmann::json::array()}
    };
}

nlohmann::json HttpServer::handleExportJSON(const std::string& filename, const std::string& docnum) {
    auto result = db_manager_->exportJSON(filename, docnum);
    
    return {
        {"status", result.success ? "success" : "error"},
        {"msg", result.message},
        {"data", result.data},
        {"index", result.index_status == IndexStatus::OK ? "ok" : 
                 result.index_status == IndexStatus::PENDING ? "pending" : "failed"},
        {"warnings", result.warnings}
    };
}

nlohmann::json HttpServer::handleExportCSV(const std::string& filename, const std::string& docnum) {
    auto result = db_manager_->exportCSV(filename, docnum);
    
    return {
        {"status", result.success ? "success" : "error"},
        {"msg", result.message},
        {"data", result.data},
        {"index", "ok"},
        {"warnings", result.warnings}
    };
}

nlohmann::json HttpServer::handleSearch(const std::string& filename, const std::string& queryParams) {
    auto params = parseQueryString(queryParams);
    int limit = 100;
    
    if (params.find("limit") != params.end()) {
        limit = std::stoi(params["limit"]);
        params.erase("limit");
    }
    
    auto result = db_manager_->search(filename, params, limit);
    
    return {
        {"status", result.success ? "success" : "error"},
        {"msg", result.message},
        {"data", result.data},
        {"index", "ok"},
        {"warnings", result.warnings}
    };
}

std::string HttpServer::handleViewHTML(const std::string& filename) {
    auto result = db_manager_->getAllRecords(filename, 500);
    
    std::ostringstream html;
    html << "<!DOCTYPE html>\n<html>\n<head>\n"
         << "<meta charset='UTF-8'>\n"
         << "<title>" << filename << "</title>\n"
         << "<style>\n"
         << "body { font-family: Arial, sans-serif; margin: 20px; }\n"
         << "table { border-collapse: collapse; width: 100%; }\n"
         << "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n"
         << "th { background-color: #4CAF50; color: white; }\n"
         << "tr:nth-child(even) { background-color: #f2f2f2; }\n"
         << "</style>\n</head>\n<body>\n";
    
    if (result.success && result.data.is_array() && !result.data.empty()) {
        html << "<h1>" << filename << " - " << result.data.size() << " records</h1>\n";
        html << "<table>\n<tr>";
        
        // Header row
        for (auto& [key, value] : result.data[0].items()) {
            html << "<th>" << key << "</th>";
        }
        html << "</tr>\n";
        
        // Data rows
        for (auto& record : result.data) {
            html << "<tr>";
            for (auto& [key, value] : record.items()) {
                html << "<td>";
                if (value.is_string()) {
                    html << value.get<std::string>();
                } else if (value.is_null()) {
                    html << "";
                } else {
                    html << value.dump();
                }
                html << "</td>";
            }
            html << "</tr>\n";
        }
        
        html << "</table>\n";
    } else {
        html << "<h1>Error</h1>\n";
        html << "<p>" << result.message << "</p>\n";
    }
    
    html << "</body>\n</html>";
    return html.str();
}

nlohmann::json HttpServer::handleFindDocnum(const std::string& docnum) {
    auto result = db_manager_->findByDocnum(docnum);
    nlohmann::json json_result = {
        {"status", result.success ? "success" : "error"},
        {"data", result.data}
    };
    if (!result.message.empty()) {
        json_result["message"] = result.message;
    }
    return json_result;
}

void HttpServer::sendCSVResponse(http::response<http::string_body>& res, const std::string& csv,
                                const std::string& filename) {
    res.result(200);
    res.set(http::field::content_type, "text/csv; charset=utf-8");
    res.set(http::field::content_disposition, "attachment; filename=\"" + filename + ".csv\"");
    res.body() = csv;
    res.prepare_payload();
}

void HttpServer::sendHTMLResponse(http::response<http::string_body>& res, const std::string& html) {
    res.result(200);
    res.set(http::field::content_type, "text/html; charset=utf-8");
    res.body() = html;
    res.prepare_payload();
}

void HttpServer::sendError(http::response<http::string_body>& res, int status, 
                          const std::string& message) {
    nlohmann::json error_json = {
        {"status", "error"},
        {"msg", message},
        {"data", nullptr},
        {"index", "ok"},
        {"warnings", nlohmann::json::array()}
    };
    sendJsonResponse(res, status, error_json);
}

std::string HttpServer::extractFilename(const std::string& path) {
    size_t last_slash = path.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        return path.substr(last_slash + 1);
    }
    return path;
}

std::string HttpServer::sanitizeFilename(const std::string& filename) {
    std::string sanitized = filename;
    
    // Remove dangerous characters but keep .dbf extension
    sanitized.erase(std::remove_if(sanitized.begin(), sanitized.end(),
                    [](char c) {
                        return c == '/' || c == '\\' || c == ';' || c == '|' || 
                               c == '<' || c == '>' || c == '"' || c == '*' || c == '?';
                    }), sanitized.end());
    
    // Ensure it ends with .dbf
    if (sanitized.length() < 4 || sanitized.substr(sanitized.length() - 4) != ".dbf") {
        throw std::runtime_error("Invalid DBF filename");
    }
    
    return sanitized;
}

std::map<std::string, std::string> HttpServer::parseQueryString(const std::string& query) {
    std::map<std::string, std::string> params;
    
    if (query.empty()) {
        return params;
    }
    
    std::istringstream iss(query);
    std::string pair;
    
    while (std::getline(iss, pair, '&')) {
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = pair.substr(0, eq_pos);
            std::string value = pair.substr(eq_pos + 1);
            params[key] = value;
        }
    }
    
    return params;
}

nlohmann::json HttpServer::handleDirectLookup(const std::string& docnum) {
    return handleFindDocnum(docnum);
}

nlohmann::json HttpServer::handleAdd(const std::string& filename, const nlohmann::json& body) {
    auto result = db_manager_->add(filename, body);
    
    return {
        {"status", result.success ? "success" : "error"},
        {"msg", result.message},
        {"data", result.data},
        {"index", result.index_status == IndexStatus::OK ? "ok" : 
                 result.index_status == IndexStatus::PENDING ? "pending" : "failed"},
        {"warnings", result.warnings}
    };
}

nlohmann::json HttpServer::handleUpdate(const std::string& filename, const nlohmann::json& body) {
    if (!body.contains("where") || !body.contains("update")) {
        return {
            {"status", "error"},
            {"msg", "Missing 'where' or 'update' in request body"},
            {"data", nullptr},
            {"index", "ok"},
            {"warnings", nlohmann::json::array()}
        };
    }
    
    auto result = db_manager_->update(filename, body["where"], body["update"]);
    
    return {
        {"status", result.success ? "success" : "error"},
        {"msg", result.message},
        {"data", result.data},
        {"index", result.index_status == IndexStatus::OK ? "ok" : 
                 result.index_status == IndexStatus::PENDING ? "pending" : "failed"},
        {"warnings", result.warnings}
    };
}

nlohmann::json HttpServer::handleDelete(const std::string& filename, const nlohmann::json& body) {
    if (!body.contains("where")) {
        return {
            {"status", "error"},
            {"msg", "Missing 'where' clause in request body"},
            {"data", nullptr},
            {"index", "ok"},
            {"warnings", nlohmann::json::array()}
        };
    }
    
    auto result = db_manager_->deleteRecords(filename, body["where"]);
    
    return {
        {"status", result.success ? "success" : "error"},
        {"msg", result.message},
        {"data", result.data},
        {"index", result.index_status == IndexStatus::OK ? "ok" : 
                 result.index_status == IndexStatus::PENDING ? "pending" : "failed"},
        {"warnings", result.warnings}
    };
}

nlohmann::json HttpServer::handleUndelete(const std::string& filename, const nlohmann::json& body) {
    if (!body.contains("where")) {
        return {
            {"status", "error"},
            {"msg", "Missing 'where' clause in request body"},
            {"data", nullptr},
            {"index", "ok"},
            {"warnings", nlohmann::json::array()}
        };
    }
    
    auto result = db_manager_->undeleteRecords(filename, body["where"]);
    
    return {
        {"status", result.success ? "success" : "error"},
        {"msg", result.message},
        {"data", result.data},
        {"index", "ok"},
        {"warnings", result.warnings}
    };
}

nlohmann::json HttpServer::handlePack(const std::string& filename) {
    auto result = db_manager_->pack(filename);
    
    return {
        {"status", result.success ? "success" : "error"},
        {"msg", result.message},
        {"data", result.data},
        {"index", result.index_status == IndexStatus::OK ? "ok" : 
                 result.index_status == IndexStatus::PENDING ? "pending" : "failed"},
        {"warnings", result.warnings}
    };
}

nlohmann::json HttpServer::handleReindex(const std::string& filename) {
    auto result = db_manager_->reindex(filename);
    
    return {
        {"status", result.success ? "success" : "error"},
        {"msg", result.message},
        {"data", result.data},
        {"index", result.index_status == IndexStatus::OK ? "ok" : 
                 result.index_status == IndexStatus::PENDING ? "pending" : "failed"},
        {"warnings", result.warnings}
    };
}

nlohmann::json HttpServer::handleIndexStatus(const std::string& filename) {
    auto result = db_manager_->getIndexStatus(filename);
    
    return {
        {"status", "success"},
        {"msg", result.message},
        {"data", result.data},
        {"index", result.index_status == IndexStatus::OK ? "ok" : 
                 result.index_status == IndexStatus::PENDING ? "pending" : "failed"},
        {"warnings", nlohmann::json::array()}
    };
}

void HttpServer::sendJsonResponse(http::response<http::string_body>& res, int status, 
                                  const nlohmann::json& json) {
    res.result(status);
    res.set(http::field::content_type, "application/json");
    res.body() = json.dump(2);
    res.prepare_payload();
}

} // namespace FoxBridge
