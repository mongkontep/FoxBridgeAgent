#pragma once

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <windows.h>

namespace FoxBridge {

class CloudflareTunnel {
public:
    explicit CloudflareTunnel(const std::string& token, int local_port);
    ~CloudflareTunnel();
    
    bool start();
    void stop();
    bool isRunning() const { return running_; }
    
    // Watchdog: restart if process dies
    void enableWatchdog(int check_interval_seconds = 30);
    void disableWatchdog();
    
private:
    std::string token_;
    int local_port_;
    std::atomic<bool> running_;
    std::atomic<bool> watchdog_enabled_;
    
    HANDLE process_handle_;
    PROCESS_INFORMATION process_info_;
    
    std::unique_ptr<std::thread> watchdog_thread_;
    
    bool launchCloudflared();
    void killProcess();
    void watchdogLoop(int interval_seconds);
    std::string findCloudflaredPath();
};

} // namespace FoxBridge
