find_package(Qt6 6.7 REQUIRED COMPONENTS Core Network)

add_library(qtcommon_httpclient STATIC
    httpclient.cpp httpclient.h
    httpresponse.cpp httpresponse.h
)
add_library(QtCommon::HttpClient ALIAS qtcommon_httpclient)

# trỏ lên src/ để #include "httpclient/httpclient.h"
target_include_directories(qtcommon_httpclient PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_features(qtcommon_httpclient PUBLIC cxx_std_17)
set_target_properties(qtcommon_httpclient PROPERTIES AUTOMOC ON)
target_link_libraries(qtcommon_httpclient PUBLIC Qt6::Core Qt6::Network)