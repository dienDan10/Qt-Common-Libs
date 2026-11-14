if(QTCOMMON_BUILD_HTTPCLIENT)
    add_library(qtcommon_httpclient STATIC
        httpclient/httpclient.cpp
        httpclient/httpclient.h
        httpclient/httpresponse.cpp
        httpclient/httpresponse.h
    )

    add_library(QtCommon::HttpClient ALIAS qtcommon_httpclient)

    target_include_directories(qtcommon_httpclient
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
    )

    target_link_libraries(qtcommon_httpclient
        PUBLIC
            Qt6::Core
            Qt6::Network
    )
endif()

if (QTCOMMON_BUILD_HTTPSERVER)
    add_library(qtcommon_httpserver STATIC
        httpserver/context.h    httpserver/context.cpp
        httpserver/ilogger.h
        httpserver/httpexception.h httpserver/httpexception.cpp
        httpserver/errorhandler.h
        httpserver/route.h httpserver/route.cpp
        httpserver/router.h httpserver/router.cpp
        httpserver/imiddleware.h
    )

    add_library(QtCommon::HttpServer ALIAS qtcommon_httpserver)

    target_include_directories(qtcommon_httpserver
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
    )

    target_link_libraries(qtcommon_httpserver
        PUBLIC
            Qt6::Core
            Qt6::Network
            Qt6::HttpServer
    )
endif()