cmake_minimum_required(VERSION 3.21)
project(QtLibs LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Network HttpServer Test)
qt_standard_project_setup()   # bật AUTOMOC cho tests

add_subdirectory(src/httpclient)
add_subdirectory(src/httpserver)

enable_testing()
add_subdirectory(tests)