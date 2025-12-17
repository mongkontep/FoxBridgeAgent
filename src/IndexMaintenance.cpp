#include "IndexMaintenance.h"
#include <spdlog/spdlog.h>
#include <sstream>

namespace FoxBridge {

IndexMaintenance::IndexMaintenance(std::shared_ptr<DatabaseManager> db_manager,
                                   const std::string& maintenance_window)
    : db_manager_(db_manager)
    , maintenance_window_(maintenance_window)
    , running_(false) {
}

IndexMaintenance::~IndexMaintenance() {
    stop();
}

void IndexMaintenance::start() {
    if (running_) {
        return;
    }
    
    running_ = true;
    worker_thread_ = std::make_unique<std::thread>(&IndexMaintenance::workerLoop, this);
    spdlog::info("Index maintenance worker started");
}

void IndexMaintenance::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    queue_cv_.notify_all();
    
    if (worker_thread_ && worker_thread_->joinable()) {
        worker_thread_->join();
    }
    
    spdlog::info("Index maintenance worker stopped");
}

void IndexMaintenance::queueReindex(const std::string& table) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    MaintenanceTask task;
    task.table = table;
    task.operation = "reindex";
    task.retry_count = 0;
    task.scheduled_time = std::chrono::system_clock::now();
    
    task_queue_.push(task);
    queue_cv_.notify_one();
    
    spdlog::info("Queued reindex task for table: {}", table);
}

void IndexMaintenance::queueVerify(const std::string& table) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    MaintenanceTask task;
    task.table = table;
    task.operation = "verify";
    task.retry_count = 0;
    task.scheduled_time = std::chrono::system_clock::now();
    
    task_queue_.push(task);
    queue_cv_.notify_one();
    
    spdlog::info("Queued verify task for table: {}", table);
}

bool IndexMaintenance::isInMaintenanceWindow() const {
    int start_hour, start_min, end_hour, end_min;
    parseMaintenanceWindow(start_hour, start_min, end_hour, end_min);
    
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* local_time = std::localtime(&now_time);
    
    int current_minutes = local_time->tm_hour * 60 + local_time->tm_min;
    int start_minutes = start_hour * 60 + start_min;
    int end_minutes = end_hour * 60 + end_min;
    
    return current_minutes >= start_minutes && current_minutes <= end_minutes;
}

void IndexMaintenance::workerLoop() {
    while (running_) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        
        // Wait for task or timeout
        queue_cv_.wait_for(lock, std::chrono::seconds(60), [this] {
            return !task_queue_.empty() || !running_;
        });
        
        if (!running_) {
            break;
        }
        
        if (task_queue_.empty()) {
            continue;
        }
        
        // Check if we're in maintenance window
        if (!isInMaintenanceWindow()) {
            spdlog::debug("Outside maintenance window, waiting...");
            continue;
        }
        
        MaintenanceTask task = task_queue_.front();
        task_queue_.pop();
        lock.unlock();
        
        // Process task
        spdlog::info("Processing maintenance task: {} on table {}", 
                    task.operation, task.table);
        
        if (!processTask(task)) {
            // Task failed, requeue if retries available
            if (task.retry_count < 3) {
                task.retry_count++;
                task.scheduled_time = std::chrono::system_clock::now() + 
                                     std::chrono::minutes(5);
                
                std::lock_guard<std::mutex> requeue_lock(queue_mutex_);
                task_queue_.push(task);
                
                spdlog::warn("Task failed, requeued (retry {})", task.retry_count);
            } else {
                spdlog::error("Task failed after max retries: {} on {}", 
                            task.operation, task.table);
            }
        }
    }
}

bool IndexMaintenance::processTask(const MaintenanceTask& task) {
    try {
        if (task.operation == "reindex") {
            auto result = db_manager_->reindex(task.table);
            return result.success;
        } else if (task.operation == "verify") {
            auto result = db_manager_->getIndexStatus(task.table);
            return result.success;
        }
        return false;
        
    } catch (const std::exception& e) {
        spdlog::error("Maintenance task exception: {}", e.what());
        return false;
    }
}

void IndexMaintenance::parseMaintenanceWindow(int& start_hour, int& start_min, 
                                              int& end_hour, int& end_min) const {
    // Parse format "HH:MM-HH:MM"
    std::istringstream iss(maintenance_window_);
    char colon, dash;
    
    iss >> start_hour >> colon >> start_min >> dash >> end_hour >> colon >> end_min;
    
    if (iss.fail()) {
        // Default window: 2:00 AM - 4:00 AM
        start_hour = 2;
        start_min = 0;
        end_hour = 4;
        end_min = 0;
    }
}

} // namespace FoxBridge
