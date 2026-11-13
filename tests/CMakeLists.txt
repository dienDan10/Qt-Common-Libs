if(QTCOMMON_BUILD_HTTPCLIENT)
    qt_add_executable(tst_httpclient
        tst_httpclient.cpp
    )

    target_link_libraries(tst_httpclient
        PRIVATE
            QtCommon::HttpClient
            Qt6::Test
    )

    add_test(NAME tst_httpclient COMMAND tst_httpclient)
endif()