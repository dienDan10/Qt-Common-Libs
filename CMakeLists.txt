cmake_minimum_required(VERSION 3.21)
project(QtLibs LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(QTCOMMON_BUILD_HTTPCLIENT
    "Build the HttpClient module"
    ON
)

option(QTCOMMON_BUILD_HTTPSERVER
    "Build the HttpServer module"
    ON
)

find_package(Qt6 REQUIRED COMPONENTS Core)

if(QTCOMMON_BUILD_HTTPCLIENT)
    find_package(Qt6 REQUIRED COMPONENTS Network)
endif()

if (QTCOMMON_BUILD_HTTPSERVER)
    find_package(Qt6 REQUIRED COMPONENTS Network HttpServer)
endif()

qt_standard_project_setup()

add_subdirectory(src)

include(CTest)

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()