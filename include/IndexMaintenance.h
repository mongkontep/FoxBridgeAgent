#pragma once

#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <chrono>
#include "DatabaseManager.h"

namespace FoxBridge {

struct MaintenanceTask {
    std::string table;
    std::string operation;  // "reindex", "verify"
    int retry_count = 0;
    std::chrono::system_clock::time_point scheduled_time;
};

class IndexMaintenance {
public:
    explicit IndexMaintenance(std::shared_ptr<DatabaseManager> db_manager,
                             const std::string& maintenance_window);
    ~IndexMaintenance();
    
    void start();
    void stop();
    
    void queueReindex(const std::string& table);
    void queueVerify(const std::string& table);
    
    bool isInMaintenanceWindow() const;
    
private:
    std::shared_ptr<DatabaseManager> db_manager_;
    std::string maintenance_window_;  // Format: "HH:MM-HH:MM"
    
    std::queue<MaintenanceTask> task_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    std::atomic<bool> running_;
    std::unique_ptr<std::thread> worker_thread_;
    
    void workerLoop();
    bool processTask(const MaintenanceTask& task);
    void parseMaintenanceWindow(int& start_hour, int& start_min, 
                               int& end_hour, int& end_min) const;
};

} // namespace FoxBridge
