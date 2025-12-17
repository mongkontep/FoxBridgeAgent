#include "DatabaseManager.h"
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace FoxBridge {

DatabaseManager::DatabaseManager(const std::string& db_folder_path)
    : db_folder_path_(db_folder_path)
    , henv_(SQL_NULL_HENV)
    , hdbc_(SQL_NULL_HDBC)
    , connected_(false) {
    
    if (!std::filesystem::exists(db_folder_path_)) {
        throw std::runtime_error("Database folder does not exist: " + db_folder_path_);
    }
    
    connect();
}

DatabaseManager::~DatabaseManager() {
    disconnect();
}

bool DatabaseManager::connect() {
    SQLRETURN ret;
    
    // Allocate environment handle
    ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv_);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        spdlog::error("Failed to allocate ODBC environment handle");
        return false;
    }
    
    // Set ODBC version
    ret = SQLSetEnvAttr(henv_, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        spdlog::error("Failed to set ODBC version");
        SQLFreeHandle(SQL_HANDLE_ENV, henv_);
        return false;
    }
    
    // Allocate connection handle
    ret = SQLAllocHandle(SQL_HANDLE_DBC, henv_, &hdbc_);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        spdlog::error("Failed to allocate ODBC connection handle");
        SQLFreeHandle(SQL_HANDLE_ENV, henv_);
        return false;
    }
    
    // Build connection string for VFP ODBC
    std::string conn_str = buildConnectionString();
    
    SQLWCHAR out_conn_str[1024];
    SQLSMALLINT out_conn_str_len;
    
    // Connect to database
    ret = SQLDriverConnectA(hdbc_, NULL, 
                           (SQLCHAR*)conn_str.c_str(), SQL_NTS,
                           (SQLCHAR*)out_conn_str, sizeof(out_conn_str),
                           &out_conn_str_len, SQL_DRIVER_NOPROMPT);
    
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        spdlog::error("Failed to connect to database");
        logError(hdbc_, SQL_HANDLE_DBC);
        SQLFreeHandle(SQL_HANDLE_DBC, hdbc_);
        SQLFreeHandle(SQL_HANDLE_ENV, henv_);
        return false;
    }
    
    connected_ = true;
    spdlog::info("Connected to VFP database at: {}", db_folder_path_);
    return true;
}

void DatabaseManager::disconnect() {
    if (hdbc_ != SQL_NULL_HDBC) {
        SQLDisconnect(hdbc_);
        SQLFreeHandle(SQL_HANDLE_DBC, hdbc_);
        hdbc_ = SQL_NULL_HDBC;
    }
    if (henv_ != SQL_NULL_HENV) {
        SQLFreeHandle(SQL_HANDLE_ENV, henv_);
        henv_ = SQL_NULL_HENV;
    }
    connected_ = false;
    spdlog::info("Disconnected from database");
}

std::string DatabaseManager::buildConnectionString() {
    // Visual FoxPro ODBC connection string
    // CRITICAL: Uses SourceType=DBF for multi-user shared access
    std::ostringstream oss;
    oss << "Driver={Microsoft Visual FoxPro Driver};"
        << "SourceType=DBF;"  // DBF mode - supports multi-user
        << "SourceDB=" << db_folder_path_ << ";"
        << "Exclusive=No;"    // CRITICAL: Never exclusive
        << "Collate=Machine;";
    
    return oss.str();
}

std::string DatabaseManager::sanitizeFilename(const std::string& filename) {
    // Prevent path traversal and SQL injection
    std::string sanitized = filename;
    
    // Remove dangerous characters but keep .dbf extension
    if (sanitized.find("..") != std::string::npos || 
        sanitized.find("/") != std::string::npos || 
        sanitized.find("\\") != std::string::npos) {
        throw std::runtime_error("Invalid filename - path traversal detected");
    }
    
    // Ensure it's a .dbf file
    if (sanitized.length() < 4 || sanitized.substr(sanitized.length() - 4) != ".dbf") {
        throw std::runtime_error("Invalid filename - must be .dbf file");
    }
    
    return sanitized;
}

bool DatabaseManager::fileExists(const std::string& filename) {
    std::filesystem::path dbf_path = std::filesystem::path(db_folder_path_) / filename;
    return std::filesystem::exists(dbf_path);
}

std::vector<std::string> DatabaseManager::listDBFFiles() {
    std::vector<std::string> files;
    
    for (const auto& entry : std::filesystem::directory_iterator(db_folder_path_)) {
        if (entry.path().extension() == ".DBF" || entry.path().extension() == ".dbf") {
            files.push_back(entry.path().filename().string());
        }
    }
    
    return files;
}

std::string DatabaseManager::buildWhereClause(const nlohmann::json& filters) {
    if (filters.is_null() || filters.empty()) {
        return "";
    }
    
    std::ostringstream where;
    bool first = true;
    
    for (auto& [key, value] : filters.items()) {
        if (!first) where << " AND ";
        where << key << " = ";
        
        if (value.is_string()) {
            where << "'" << value.get<std::string>() << "'";
        } else if (value.is_number()) {
            where << value.dump();
        } else if (value.is_boolean()) {
            where << (value.get<bool>() ? ".T." : ".F.");
        } else {
            where << "NULL";
        }
        
        first = false;
    }
    
    return where.str();
}

std::string DatabaseManager::jsonToCSV(const nlohmann::json& data) {
    if (!data.is_array() || data.empty()) {
        return "";
    }
    
    std::ostringstream csv;
    
    // Header row
    bool first = true;
    for (auto& [key, value] : data[0].items()) {
        if (!first) csv << ",";
        csv << "\"" << key << "\"";
        first = false;
    }
    csv << "\n";
    
    // Data rows
    for (auto& record : data) {
        first = true;
        for (auto& [key, value] : record.items()) {
            if (!first) csv << ",";
            
            if (value.is_string()) {
                std::string str = value.get<std::string>();
                // Escape quotes
                size_t pos = 0;
                while ((pos = str.find("\"", pos)) != std::string::npos) {
                    str.replace(pos, 1, "\"\"");
                    pos += 2;
                }
                csv << "\"" << str << "\"";
            } else if (value.is_null()) {
                csv << "\"\"";
            } else {
                csv << value.dump();
            }
            
            first = false;
        }
        csv << "\n";
    }
    
    return csv.str();
}

// New file-based operations

QueryResult DatabaseManager::exportJSON(const std::string& filename, const std::string& docnum) {
    QueryResult result;
    result.success = false;
    result.index_status = IndexStatus::OK;
    
    try {
        std::string safe_filename = sanitizeFilename(filename);
        
        if (!fileExists(safe_filename)) {
            result.message = "File not found: " + safe_filename;
            return result;
        }
        
        std::ostringstream sql;
        sql << "SELECT * FROM " << safe_filename;
        
        if (!docnum.empty()) {
            sql << " WHERE docnum = '" << docnum << "'";
        }
        
        if (executeSQL(sql.str(), result.data)) {
            result.success = true;
            result.message = docnum.empty() ? "All records exported" : "Record found";
        } else {
            result.message = "Failed to export records";
        }
        
    } catch (const std::exception& e) {
        result.message = std::string("Error: ") + e.what();
    }
    
    return result;
}

QueryResult DatabaseManager::exportCSV(const std::string& filename, const std::string& docnum) {
    QueryResult result = exportJSON(filename, docnum);
    
    if (result.success && result.data.is_array()) {
        std::string csv = jsonToCSV(result.data);
        result.data = csv;
    }
    
    return result;
}

QueryResult DatabaseManager::search(const std::string& filename, 
                                    const std::map<std::string, std::string>& filters, 
                                    int limit) {
    QueryResult result;
    result.success = false;
    result.index_status = IndexStatus::OK;
    
    try {
        std::string safe_filename = sanitizeFilename(filename);
        
        if (!fileExists(safe_filename)) {
            result.message = "File not found: " + safe_filename;
            return result;
        }
        
        std::ostringstream sql;
        sql << "SELECT TOP " << limit << " * FROM " << safe_filename;
        
        if (!filters.empty()) {
            sql << " WHERE ";
            bool first = true;
            for (auto& [key, value] : filters) {
                if (!first) sql << " AND ";
                sql << key << " LIKE '%" << value << "%'";
                first = false;
            }
        }
        
        if (executeSQL(sql.str(), result.data)) {
            result.success = true;
            result.message = "Search completed";
        } else {
            result.message = "Search failed";
        }
        
    } catch (const std::exception& e) {
        result.message = std::string("Error: ") + e.what();
    }
    
    return result;
}

QueryResult DatabaseManager::getAllRecords(const std::string& filename, int limit) {
    QueryResult result;
    result.success = false;
    result.index_status = IndexStatus::OK;
    
    try {
        std::string safe_filename = sanitizeFilename(filename);
        
        if (!fileExists(safe_filename)) {
            result.message = "File not found: " + safe_filename;
            return result;
        }
        
        std::ostringstream sql;
        sql << "SELECT TOP " << limit << " * FROM " << safe_filename;
        
        if (executeSQL(sql.str(), result.data)) {
            result.success = true;
            result.message = "Records retrieved";
        } else {
            result.message = "Failed to retrieve records";
        }
        
    } catch (const std::exception& e) {
        result.message = std::string("Error: ") + e.what();
    }
    
    return result;
}

QueryResult DatabaseManager::findByDocnum(const std::string& docnum) {
    QueryResult result;
    result.success = false;
    result.index_status = IndexStatus::OK;
    
    try {
        // Common DBF files to search
        std::vector<std::string> tables = {
            "invoice.dbf", "receipt.dbf", "order.dbf", 
            "quotation.dbf", "payment.dbf", "delivery.dbf"
        };
        
        result.data = nlohmann::json::array();
        
        for (const auto& table : tables) {
            if (fileExists(table)) {
                std::ostringstream sql;
                sql << "SELECT * FROM " << table << " WHERE docnum = '" << docnum << "'";
                
                nlohmann::json temp_result;
                if (executeSQL(sql.str(), temp_result)) {
                    if (temp_result.is_array()) {
                        for (auto& record : temp_result) {
                            record["_source_file"] = table;
                            result.data.push_back(record);
                        }
                    }
                }
            }
        }
        
        if (!result.data.empty()) {
            result.success = true;
            result.message = "Document found in " + std::to_string(result.data.size()) + " table(s)";
        } else {
            result.message = "Document not found";
        }
        
    } catch (const std::exception& e) {
        result.message = std::string("Error: ") + e.what();
    }
    
    return result;
}

QueryResult DatabaseManager::add(const std::string& filename, const nlohmann::json& record) {
    QueryResult result;
    result.success = false;
    result.index_status = IndexStatus::OK;
    
    try {
        std::string safe_filename = sanitizeFilename(filename);
        
        if (!fileExists(safe_filename)) {
            result.message = "File not found: " + safe_filename;
            return result;
        }
        
        // Build INSERT statement
        std::ostringstream sql;
        std::vector<std::string> columns;
        std::vector<std::string> values;
        
        for (auto& [key, value] : record.items()) {
            columns.push_back(key);
            if (value.is_string()) {
                values.push_back("'" + value.get<std::string>() + "'");
            } else if (value.is_boolean()) {
                values.push_back(value.get<bool>() ? ".T." : ".F.");
            } else {
                values.push_back(value.dump());
            }
        }
        
        sql << "INSERT INTO " << safe_filename << " (";
        for (size_t i = 0; i < columns.size(); ++i) {
            if (i > 0) sql << ", ";
            sql << columns[i];
        }
        sql << ") VALUES (";
        for (size_t i = 0; i < values.size(); ++i) {
            if (i > 0) sql << ", ";
            sql << values[i];
        }
        sql << ")";
        
        nlohmann::json exec_result;
        if (executeSQL(sql.str(), exec_result)) {
            result.success = true;
            result.message = "Record added successfully";
            result.data = record;
            result.index_status = IndexStatus::OK;
        } else {
            result.message = "Failed to add record";
            result.index_status = IndexStatus::PENDING;
        }
        
    } catch (const std::exception& e) {
        result.message = std::string("Error: ") + e.what();
        result.index_status = IndexStatus::FAILED;
    }
    
    return result;
}

QueryResult DatabaseManager::update(const std::string& filename, const nlohmann::json& where, 
                                   const nlohmann::json& updates) {
    QueryResult result;
    result.success = false;
    result.index_status = IndexStatus::OK;
    
    try {
        std::string safe_filename = sanitizeFilename(filename);
        
        if (!fileExists(safe_filename)) {
            result.message = "File not found: " + safe_filename;
            return result;
        }
        
        // Build UPDATE statement
        std::ostringstream sql;
        sql << "UPDATE " << safe_filename << " SET ";
        
        bool first = true;
        for (auto& [key, value] : updates.items()) {
            if (!first) sql << ", ";
            sql << key << " = ";
            if (value.is_string()) {
                sql << "'" << value.get<std::string>() << "'";
            } else if (value.is_boolean()) {
                sql << (value.get<bool>() ? ".T." : ".F.");
            } else {
                sql << value.dump();
            }
            first = false;
        }
        
        std::string where_clause = buildWhereClause(where);
        if (!where_clause.empty()) {
            sql << " WHERE " << where_clause;
        }
        
        nlohmann::json exec_result;
        if (executeSQL(sql.str(), exec_result)) {
            result.success = true;
            result.message = "Record(s) updated successfully";
            result.data = updates;
            result.index_status = IndexStatus::OK;
        } else {
            result.message = "Failed to update record(s)";
            result.index_status = IndexStatus::PENDING;
        }
        
    } catch (const std::exception& e) {
        result.message = std::string("Error: ") + e.what();
        result.index_status = IndexStatus::FAILED;
    }
    
    return result;
}

QueryResult DatabaseManager::deleteRecords(const std::string& filename, const nlohmann::json& where) {
    QueryResult result;
    result.success = false;
    result.index_status = IndexStatus::OK;
    
    try {
        std::string safe_filename = sanitizeFilename(filename);
        
        if (!fileExists(safe_filename)) {
            result.message = "File not found: " + safe_filename;
            return result;
        }
        
        // Soft delete - mark as deleted
        std::ostringstream sql;
        sql << "UPDATE " << safe_filename << " SET deleted = .T.";
        
        std::string where_clause = buildWhereClause(where);
        if (!where_clause.empty()) {
            sql << " WHERE " << where_clause;
        }
        
        nlohmann::json exec_result;
        if (executeSQL(sql.str(), exec_result)) {
            result.success = true;
            result.message = "Record(s) marked as deleted";
            result.index_status = IndexStatus::OK;
        } else {
            result.message = "Failed to delete record(s)";
            result.index_status = IndexStatus::PENDING;
        }
        
    } catch (const std::exception& e) {
        result.message = std::string("Error: ") + e.what();
        result.index_status = IndexStatus::FAILED;
    }
    
    return result;
}

QueryResult DatabaseManager::undeleteRecords(const std::string& filename, const nlohmann::json& where) {
    QueryResult result;
    result.success = false;
    result.index_status = IndexStatus::OK;
    
    try {
        std::string safe_filename = sanitizeFilename(filename);
        
        if (!fileExists(safe_filename)) {
            result.message = "File not found: " + safe_filename;
            return result;
        }
        
        // Undelete - unmark as deleted
        std::ostringstream sql;
        sql << "UPDATE " << safe_filename << " SET deleted = .F.";
        
        std::string where_clause = buildWhereClause(where);
        if (!where_clause.empty()) {
            sql << " WHERE " << where_clause;
        }
        
        nlohmann::json exec_result;
        if (executeSQL(sql.str(), exec_result)) {
            result.success = true;
            result.message = "Record(s) restored";
            result.index_status = IndexStatus::OK;
        } else {
            result.message = "Failed to restore record(s)";
            result.index_status = IndexStatus::PENDING;
        }
        
    } catch (const std::exception& e) {
        result.message = std::string("Error: ") + e.what();
        result.index_status = IndexStatus::FAILED;
    }
    
    return result;
}

QueryResult DatabaseManager::pack(const std::string& filename) {
    QueryResult result;
    result.success = false;
    result.index_status = IndexStatus::OK;
    
    try {
        std::string safe_filename = sanitizeFilename(filename);
        
        if (!fileExists(safe_filename)) {
            result.message = "File not found: " + safe_filename;
            return result;
        }
        
        // CRITICAL: PACK requires exclusive access
        result.message = "PACK operation requires exclusive access. Please close ExpressD first.";
        result.warnings.push_back("ExpressD must be closed for PACK operation");
        result.warnings.push_back("Use maintenance window for PACK");
        result.warnings.push_back("Consider using soft delete instead");
        
        // In a real implementation, would check for exclusive access and execute PACK
        // For now, we reject the operation in multi-user mode
        
    } catch (const std::exception& e) {
        result.message = std::string("Error: ") + e.what();
        result.index_status = IndexStatus::FAILED;
    }
    
    return result;
}

bool DatabaseManager::executeSQL(const std::string& sql, nlohmann::json& result) {
    if (!connected_) {
        spdlog::error("Not connected to database");
        return false;
    }
    
    SQLHSTMT stmt;
    SQLRETURN ret;
    
    // Allocate statement handle
    ret = SQLAllocHandle(SQL_HANDLE_STMT, hdbc_, &stmt);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        spdlog::error("Failed to allocate statement handle");
        return false;
    }
    
    // Execute SQL
    ret = SQLExecDirectA(stmt, (SQLCHAR*)sql.c_str(), SQL_NTS);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        spdlog::error("SQL execution failed: {}", sql);
        logError(stmt, SQL_HANDLE_STMT);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return false;
    }
    
    // For SELECT queries, fetch results
    if (sql.find("SELECT") != std::string::npos) {
        SQLSMALLINT column_count;
        SQLNumResultCols(stmt, &column_count);
        
        result = nlohmann::json::array();
        
        while (SQLFetch(stmt) == SQL_SUCCESS) {
            nlohmann::json row;
            for (SQLSMALLINT i = 1; i <= column_count; ++i) {
                char column_name[256];
                SQLSMALLINT name_len;
                SQLDescribeColA(stmt, i, (SQLCHAR*)column_name, sizeof(column_name), 
                               &name_len, NULL, NULL, NULL, NULL);
                
                char value[1024];
                SQLLEN indicator;
                ret = SQLGetData(stmt, i, SQL_C_CHAR, value, sizeof(value), &indicator);
                
                if (ret == SQL_SUCCESS && indicator != SQL_NULL_DATA) {
                    row[column_name] = value;
                } else {
                    row[column_name] = nullptr;
                }
            }
            result.push_back(row);
        }
    }
    
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return true;
}

QueryResult DatabaseManager::getIndexStatus(const std::string& filename) {
    QueryResult result;
    result.success = true;
    result.message = "Index status retrieved";
    result.index_status = checkIndexHealth(filename);
    
    result.data = {
        {"file", filename},
        {"status", result.index_status == IndexStatus::OK ? "ok" : 
                  result.index_status == IndexStatus::PENDING ? "pending" : "failed"}
    };
    
    return result;
}

QueryResult DatabaseManager::reindex(const std::string& filename) {
    QueryResult result;
    result.success = false;
    
    try {
        std::string safe_filename = sanitizeFilename(filename);
        
        if (!fileExists(safe_filename)) {
            result.message = "File not found: " + safe_filename;
            return result;
        }
        
        // CRITICAL: Cannot reindex in multi-user mode without exclusive access
        result.message = "Reindex requires exclusive access. Please schedule during maintenance window.";
        result.warnings.push_back("ExpressD must be closed for exclusive reindex");
        result.warnings.push_back("Use maintenance_window configuration");
        
    } catch (const std::exception& e) {
        result.message = std::string("Error: ") + e.what();
    }
    
    return result;
}

IndexStatus DatabaseManager::checkIndexHealth(const std::string& filename) {
    // Simplified health check - in production, check CDX file integrity
    try {
        std::string safe_filename = sanitizeFilename(filename);
        
        // Remove .dbf extension to get base name
        std::string base_name = safe_filename.substr(0, safe_filename.length() - 4);
        std::string cdx_path = db_folder_path_ + "\\" + base_name + ".CDX";
        
        if (std::filesystem::exists(cdx_path)) {
            return IndexStatus::OK;
        } else {
            return IndexStatus::PENDING;
        }
    } catch (...) {
        return IndexStatus::FAILED;
    }
}

void DatabaseManager::logError(SQLHANDLE handle, SQLSMALLINT type) {
    SQLCHAR sql_state[6];
    SQLCHAR message[SQL_MAX_MESSAGE_LENGTH];
    SQLINTEGER native_error;
    SQLSMALLINT msg_len;
    
    SQLGetDiagRecA(type, handle, 1, sql_state, &native_error, 
                   message, sizeof(message), &msg_len);
    
    spdlog::error("ODBC Error: {} - {}", reinterpret_cast<const char*>(sql_state), reinterpret_cast<const char*>(message));
}

} // namespace FoxBridge
