#include "WindowsService.h"
#include <spdlog/spdlog.h>

namespace FoxBridge {

WindowsService* WindowsService::instance_ = nullptr;

WindowsService::WindowsService(const std::string& service_name, 
                               std::function<void()> start_callback,
                               std::function<void()> stop_callback)
    : service_name_(service_name)
    , start_callback_(start_callback)
    , stop_callback_(stop_callback)
    , status_handle_(nullptr) {
    
    instance_ = this;
    
    ZeroMemory(&service_status_, sizeof(service_status_));
    service_status_.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    service_status_.dwCurrentState = SERVICE_START_PENDING;
}

void WindowsService::run() {
    std::wstring wide_name(service_name_.begin(), service_name_.end());
    
    SERVICE_TABLE_ENTRYW service_table[] = {
        { const_cast<LPWSTR>(wide_name.c_str()), serviceMain },
        { nullptr, nullptr }
    };
    
    if (!StartServiceCtrlDispatcherW(service_table)) {
        spdlog::error("StartServiceCtrlDispatcher failed: {}", GetLastError());
    }
}

void WINAPI WindowsService::serviceMain(DWORD argc, LPWSTR* argv) {
    if (!instance_) {
        return;
    }
    
    std::wstring wide_name(instance_->service_name_.begin(), 
                          instance_->service_name_.end());
    
    instance_->status_handle_ = RegisterServiceCtrlHandlerW(
        wide_name.c_str(),
        serviceCtrlHandler
    );
    
    if (!instance_->status_handle_) {
        spdlog::error("RegisterServiceCtrlHandler failed");
        return;
    }
    
    instance_->reportStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
    
    try {
        // Call user's start callback
        if (instance_->start_callback_) {
            instance_->start_callback_();
        }
        
        instance_->reportStatus(SERVICE_RUNNING, NO_ERROR, 0);
        spdlog::info("Service started successfully");
        
        // Keep running until stopped
        while (instance_->service_status_.dwCurrentState == SERVICE_RUNNING) {
            Sleep(1000);
        }
        
    } catch (const std::exception& e) {
        spdlog::error("Service error: {}", e.what());
        instance_->reportStatus(SERVICE_STOPPED, ERROR_SERVICE_SPECIFIC_ERROR, 0);
    }
}

void WINAPI WindowsService::serviceCtrlHandler(DWORD ctrl_code) {
    if (!instance_) {
        return;
    }
    
    switch (ctrl_code) {
        case SERVICE_CONTROL_STOP:
            instance_->reportStatus(SERVICE_STOP_PENDING, NO_ERROR, 3000);
            
            // Call user's stop callback
            if (instance_->stop_callback_) {
                instance_->stop_callback_();
            }
            
            instance_->reportStatus(SERVICE_STOPPED, NO_ERROR, 0);
            spdlog::info("Service stopped");
            break;
            
        case SERVICE_CONTROL_INTERROGATE:
            break;
            
        default:
            break;
    }
}

void WindowsService::reportStatus(DWORD current_state, DWORD exit_code, DWORD wait_hint) {
    static DWORD checkpoint = 1;
    
    service_status_.dwCurrentState = current_state;
    service_status_.dwWin32ExitCode = exit_code;
    service_status_.dwWaitHint = wait_hint;
    
    if (current_state == SERVICE_START_PENDING) {
        service_status_.dwControlsAccepted = 0;
    } else {
        service_status_.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    }
    
    if (current_state == SERVICE_RUNNING || current_state == SERVICE_STOPPED) {
        service_status_.dwCheckPoint = 0;
    } else {
        service_status_.dwCheckPoint = checkpoint++;
    }
    
    SetServiceStatus(status_handle_, &service_status_);
}

bool WindowsService::install(const std::string& service_name, 
                            const std::string& display_name,
                            const std::string& executable_path) {
    SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
    if (!scm) {
        spdlog::error("OpenSCManager failed: {}", GetLastError());
        return false;
    }
    
    std::wstring wide_name(service_name.begin(), service_name.end());
    std::wstring wide_display(display_name.begin(), display_name.end());
    std::wstring wide_path(executable_path.begin(), executable_path.end());
    
    SC_HANDLE service = CreateServiceW(
        scm,
        wide_name.c_str(),
        wide_display.c_str(),
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        wide_path.c_str(),
        nullptr, nullptr, nullptr, nullptr, nullptr
    );
    
    if (!service) {
        DWORD error = GetLastError();
        CloseServiceHandle(scm);
        if (error == ERROR_SERVICE_EXISTS) {
            spdlog::info("Service already exists");
            return true;
        }
        spdlog::error("CreateService failed: {}", error);
        return false;
    }
    
    // Configure service recovery (restart on failure)
    SERVICE_FAILURE_ACTIONS failureActions = {0};
    SC_ACTION actions[3];
    
    // Restart after 1 minute on first failure
    actions[0].Type = SC_ACTION_RESTART;
    actions[0].Delay = 60000; // 60 seconds
    
    // Restart after 2 minutes on second failure
    actions[1].Type = SC_ACTION_RESTART;
    actions[1].Delay = 120000; // 2 minutes
    
    // Restart after 5 minutes on subsequent failures
    actions[2].Type = SC_ACTION_RESTART;
    actions[2].Delay = 300000; // 5 minutes
    
    failureActions.dwResetPeriod = 86400; // Reset failure count after 24 hours
    failureActions.cActions = 3;
    failureActions.lpsaActions = actions;
    
    if (!ChangeServiceConfig2W(service, SERVICE_CONFIG_FAILURE_ACTIONS, &failureActions)) {
        spdlog::warn("Failed to configure service recovery: {}", GetLastError());
    } else {
        spdlog::info("Service recovery configured: auto-restart on failure");
    }
    
    // Configure service description
    std::wstring description = L"HTTP API Server for Visual FoxPro ExpressD database. "
                              L"Provides REST API access to DBF files with multi-user safety. "
                              L"Automatically starts on system boot.";
    SERVICE_DESCRIPTIONW serviceDesc;
    serviceDesc.lpDescription = const_cast<LPWSTR>(description.c_str());
    
    if (!ChangeServiceConfig2W(service, SERVICE_CONFIG_DESCRIPTION, &serviceDesc)) {
        spdlog::warn("Failed to set service description: {}", GetLastError());
    }
    
    // Configure delayed auto-start (starts after all AUTO_START services)
    SERVICE_DELAYED_AUTO_START_INFO delayedInfo;
    delayedInfo.fDelayedAutostart = TRUE;
    
    if (!ChangeServiceConfig2W(service, SERVICE_CONFIG_DELAYED_AUTO_START_INFO, &delayedInfo)) {
        spdlog::warn("Failed to configure delayed auto-start: {}", GetLastError());
    } else {
        spdlog::info("Service configured for delayed auto-start");
    }
    
    spdlog::info("Service installed successfully with auto-start configuration");
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return true;
}

bool WindowsService::uninstall(const std::string& service_name) {
    SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) {
        spdlog::error("OpenSCManager failed: {}", GetLastError());
        return false;
    }
    
    std::wstring wide_name(service_name.begin(), service_name.end());
    SC_HANDLE service = OpenServiceW(scm, wide_name.c_str(), SERVICE_STOP | DELETE);
    if (!service) {
        CloseServiceHandle(scm);
        spdlog::error("OpenService failed: {}", GetLastError());
        return false;
    }
    
    // Stop service first
    SERVICE_STATUS status;
    ControlService(service, SERVICE_CONTROL_STOP, &status);
    
    // Delete service
    if (!DeleteService(service)) {
        DWORD error = GetLastError();
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        spdlog::error("DeleteService failed: {}", error);
        return false;
    }
    
    spdlog::info("Service uninstalled successfully");
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return true;
}

bool WindowsService::start(const std::string& service_name) {
    SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) {
        return false;
    }
    
    std::wstring wide_name(service_name.begin(), service_name.end());
    SC_HANDLE service = OpenServiceW(scm, wide_name.c_str(), SERVICE_START);
    if (!service) {
        CloseServiceHandle(scm);
        return false;
    }
    
    bool result = StartServiceW(service, 0, nullptr);
    
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return result;
}

bool WindowsService::stop(const std::string& service_name) {
    SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) {
        return false;
    }
    
    std::wstring wide_name(service_name.begin(), service_name.end());
    SC_HANDLE service = OpenServiceW(scm, wide_name.c_str(), SERVICE_STOP);
    if (!service) {
        CloseServiceHandle(scm);
        return false;
    }
    
    SERVICE_STATUS status;
    bool result = ControlService(service, SERVICE_CONTROL_STOP, &status);
    
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return result;
}

} // namespace FoxBridge
