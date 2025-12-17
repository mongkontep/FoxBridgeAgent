#pragma once

#include <windows.h>
#include <string>
#include <functional>

namespace FoxBridge {

class WindowsService {
public:
    WindowsService(const std::string& service_name, 
                   std::function<void()> start_callback,
                   std::function<void()> stop_callback);
    
    static bool install(const std::string& service_name, 
                       const std::string& display_name,
                       const std::string& executable_path);
    static bool uninstall(const std::string& service_name);
    static bool start(const std::string& service_name);
    static bool stop(const std::string& service_name);
    
    void run();
    
private:
    std::string service_name_;
    std::function<void()> start_callback_;
    std::function<void()> stop_callback_;
    SERVICE_STATUS service_status_;
    SERVICE_STATUS_HANDLE status_handle_;
    
    static WindowsService* instance_;
    static void WINAPI serviceMain(DWORD argc, LPWSTR* argv);
    static void WINAPI serviceCtrlHandler(DWORD ctrl_code);
    
    void reportStatus(DWORD current_state, DWORD exit_code, DWORD wait_hint);
};

} // namespace FoxBridge
