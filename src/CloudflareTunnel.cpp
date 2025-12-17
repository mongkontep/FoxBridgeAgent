#include "CloudflareTunnel.h"
#include <spdlog/spdlog.h>
#include <sstream>

namespace FoxBridge {

CloudflareTunnel::CloudflareTunnel(const std::string& token, int local_port)
    : token_(token)
    , local_port_(local_port)
    , running_(false)
    , watchdog_enabled_(false)
    , process_handle_(nullptr) {
    
    ZeroMemory(&process_info_, sizeof(process_info_));
}

CloudflareTunnel::~CloudflareTunnel() {
    stop();
}

bool CloudflareTunnel::start() {
    if (running_) {
        spdlog::warn("Cloudflare Tunnel already running");
        return true;
    }
    
    if (launchCloudflared()) {
        running_ = true;
        spdlog::info("Cloudflare Tunnel started");
        return true;
    }
    
    return false;
}

void CloudflareTunnel::stop() {
    if (!running_) {
        return;
    }
    
    disableWatchdog();
    killProcess();
    running_ = false;
    spdlog::info("Cloudflare Tunnel stopped");
}

bool CloudflareTunnel::launchCloudflared() {
    std::string cloudflared_path = findCloudflaredPath();
    if (cloudflared_path.empty()) {
        spdlog::error("cloudflared.exe not found in PATH");
        return false;
    }
    
    // Build command line
    std::ostringstream cmd;
    cmd << "\"" << cloudflared_path << "\" tunnel"
        << " --url http://127.0.0.1:" << local_port_
        << " run"
        << " --token " << token_;
    
    std::string cmd_str = cmd.str();
    
    STARTUPINFOA startup_info;
    ZeroMemory(&startup_info, sizeof(startup_info));
    startup_info.cb = sizeof(startup_info);
    startup_info.dwFlags = STARTF_USESHOWWINDOW;
    startup_info.wShowWindow = SW_HIDE;
    
    ZeroMemory(&process_info_, sizeof(process_info_));
    
    if (!CreateProcessA(
        nullptr,
        const_cast<char*>(cmd_str.c_str()),
        nullptr, nullptr, FALSE,
        CREATE_NO_WINDOW,
        nullptr, nullptr,
        &startup_info,
        &process_info_)) {
        
        spdlog::error("Failed to start cloudflared: {}", GetLastError());
        return false;
    }
    
    process_handle_ = process_info_.hProcess;
    spdlog::info("cloudflared process started (PID: {})", process_info_.dwProcessId);
    return true;
}

void CloudflareTunnel::killProcess() {
    if (process_handle_) {
        TerminateProcess(process_handle_, 0);
        WaitForSingleObject(process_handle_, 5000);
        CloseHandle(process_info_.hThread);
        CloseHandle(process_info_.hProcess);
        process_handle_ = nullptr;
    }
}

void CloudflareTunnel::enableWatchdog(int check_interval_seconds) {
    if (watchdog_enabled_) {
        return;
    }
    
    watchdog_enabled_ = true;
    watchdog_thread_ = std::make_unique<std::thread>(&CloudflareTunnel::watchdogLoop, 
                                                      this, check_interval_seconds);
    spdlog::info("Cloudflare Tunnel watchdog enabled (interval: {}s)", check_interval_seconds);
}

void CloudflareTunnel::disableWatchdog() {
    if (!watchdog_enabled_) {
        return;
    }
    
    watchdog_enabled_ = false;
    if (watchdog_thread_ && watchdog_thread_->joinable()) {
        watchdog_thread_->join();
    }
    spdlog::info("Cloudflare Tunnel watchdog disabled");
}

void CloudflareTunnel::watchdogLoop(int interval_seconds) {
    while (watchdog_enabled_ && running_) {
        Sleep(interval_seconds * 1000);
        
        if (!running_) {
            break;
        }
        
        // Check if process is still alive
        DWORD exit_code;
        if (GetExitCodeProcess(process_handle_, &exit_code)) {
            if (exit_code != STILL_ACTIVE) {
                spdlog::warn("Cloudflare Tunnel process died (exit code: {}), restarting...", exit_code);
                killProcess();
                Sleep(2000);  // Wait before restart
                launchCloudflared();
            }
        }
    }
}

std::string CloudflareTunnel::findCloudflaredPath() {
    // Try common locations
    const char* paths[] = {
        "cloudflared.exe",
        "C:\\Program Files\\cloudflared\\cloudflared.exe",
        "C:\\Program Files (x86)\\cloudflared\\cloudflared.exe"
    };
    
    for (const char* path : paths) {
        DWORD attribs = GetFileAttributesA(path);
        if (attribs != INVALID_FILE_ATTRIBUTES && !(attribs & FILE_ATTRIBUTE_DIRECTORY)) {
            return path;
        }
    }
    
    // Check PATH environment variable
    char buffer[MAX_PATH];
    if (SearchPathA(nullptr, "cloudflared.exe", nullptr, MAX_PATH, buffer, nullptr)) {
        return buffer;
    }
    
    return "";
}

} // namespace FoxBridge
