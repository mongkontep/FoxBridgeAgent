#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace FoxBridge {

enum class IndexStatus {
    OK,
    PENDING,
    FAILED
};

struct QueryResult {
    bool success;
    std::string message;
    nlohmann::json data;
    IndexStatus index_status;
    std::vector<std::string> warnings;
};

class DatabaseManager {
public:
    explicit DatabaseManager(const std::string& db_folder_path);
    ~DatabaseManager();
    
    // Disable copy
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
    
    // DBF file operations
    QueryResult exportJSON(const std::string& filename, const std::string& docnum = "");
    QueryResult exportCSV(const std::string& filename, const std::string& docnum = "");
    QueryResult search(const std::string& filename, const std::map<std::string, std::string>& filters, int limit = 100);
    QueryResult getAllRecords(const std::string& filename, int limit = 1000);
    QueryResult findByDocnum(const std::string& docnum);
    
    // CRUD operations
    QueryResult add(const std::string& filename, const nlohmann::json& record);
    QueryResult update(const std::string& filename, const nlohmann::json& where, const nlohmann::json& updates);
    QueryResult deleteRecords(const std::string& filename, const nlohmann::json& where);
    QueryResult undeleteRecords(const std::string& filename, const nlohmann::json& where);
    QueryResult pack(const std::string& filename);
    
    // Index maintenance
    QueryResult getIndexStatus(const std::string& filename);
    QueryResult reindex(const std::string& filename);
    
    bool isConnected() const { return connected_; }
    
private:
    std::string db_folder_path_;
    bool executeSQLCount(const std::string& sql, int& count);
    std::string sanitizeFilename(const std::string& filename);
    std::string getTableNameFromFile(const std::string& filename);
    std::string buildConnectionString();
    std::string buildWhereClause(const nlohmann::json& where);
    std::string jsonToCSV(const nlohmann::json& data);
    IndexStatus checkIndexHealth(const std::string& filename);
    void logError(SQLHANDLE handle, SQLSMALLINT type);
    bool fileExists(const std::string& filename);
    std::vector<std::string> listDBFFiles(
    bool connect();
    void disconnect();
    bool executeSQL(const std::string& sql, nlohmann::json& result);
    std::string sanitizeTableName(const std::string& table);
    std::string buildConnectionString();
    IndexStatus checkIndexHealth(const std::string& table);
    void logError(SQLHANDLE handle, SQLSMALLINT type);
};

} // namespace FoxBridge
