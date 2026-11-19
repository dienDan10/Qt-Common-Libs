find_package(Qt6 6.7 REQUIRED COMPONENTS Core Network HttpServer)

add_library(qtcommon_httpserver STATIC
    context.h context.cpp
    ilogger.h defaultlogger.h
    httpexception.h httpexception.cpp
    errorhandler.h
    imiddleware.h
    route.h route.cpp
    router.h router.cpp
    httpserver.h httpserver.cpp
)
add_library(QtCommon::HttpServer ALIAS qtcommon_httpserver)

target_include_directories(qtcommon_httpserver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_features(qtcommon_httpserver PUBLIC cxx_std_17)
set_target_properties(qtcommon_httpserver PROPERTIES AUTOMOC ON)
target_link_libraries(qtcommon_httpserver PUBLIC Qt6::Core Qt6::Network Qt6::HttpServer)