cmake_minimum_required(VERSION 3.21)
project(QtLibs LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Test Network)
qt_standard_project_setup()

# Gom tất cả class thành một static lib để các test link vào
file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.h)
add_library(qtlibs STATIC ${LIB_SOURCES})
target_include_directories(qtlibs PUBLIC src)
target_link_libraries(qtlibs PUBLIC Qt6::Core Qt6::Widgets Qt6::Network)

# Mỗi file tst_*.cpp thành một test executable
enable_testing()
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS tests/tst_*.cpp)
foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    qt_add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE qtlibs Qt6::Test)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()