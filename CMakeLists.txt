cmake_minimum_required(VERSION 3.20)
project(FoxBridgeAgent VERSION 1.0.0 LANGUAGES CXX)

# C++20 Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find packages
find_package(Boost 1.75 REQUIRED COMPONENTS system)
find_package(ODBC REQUIRED)

# nlohmann/json (header-only)
include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)
FetchContent_MakeAvailable(json)

# spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)
FetchContent_MakeAvailable(spdlog)

# Source files
set(SOURCES
    src/main.cpp
    src/DatabaseManager.cpp
    src/HttpServer.cpp
    src/WindowsService.cpp
    src/CloudflareTunnel.cpp
    src/IndexMaintenance.cpp
)

# Header files
set(HEADERS
    include/Config.h
    include/DatabaseManager.h
    include/HttpServer.h
    include/WindowsService.h
    include/CloudflareTunnel.h
    include/IndexMaintenance.h
)

# Executable
add_executable(FoxBridgeAgent ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(FoxBridgeAgent PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(FoxBridgeAgent PRIVATE
    Boost::system
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    odbc32
)

# Windows-specific settings
if(WIN32)
    target_compile_definitions(FoxBridgeAgent PRIVATE
        _WIN32_WINNT=0x0601  # Windows 7 or later
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    
    # Link Windows libraries
    target_link_libraries(FoxBridgeAgent PRIVATE
        ws2_32
        advapi32
    )
endif()

# Installation
install(TARGETS FoxBridgeAgent
    RUNTIME DESTINATION bin
)

install(FILES
    ${CMAKE_SOURCE_DIR}/config/config.json.template
    DESTINATION config
)

# Build information
message(STATUS "========================================")
message(STATUS "FoxBridgeAgent Build Configuration")
message(STATUS "========================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Boost Version: ${Boost_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "========================================")
