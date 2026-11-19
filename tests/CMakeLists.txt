qt_add_executable(tst_httpclient tst_httpclient.cpp)
target_link_libraries(tst_httpclient PRIVATE QtCommon::HttpClient Qt6::Test)
add_test(NAME tst_httpclient COMMAND tst_httpclient)

qt_add_executable(tst_context tst_context.cpp testhelper.h)
target_link_libraries(tst_context PRIVATE QtCommon::HttpServer Qt6::Test)
add_test(NAME tst_context COMMAND tst_context)

qt_add_executable(tst_errorhandler tst_errorhandler.cpp testhelper.h)
target_link_libraries(tst_errorhandler PRIVATE QtCommon::HttpServer Qt6::Test)
add_test(NAME tst_errorhandler COMMAND tst_errorhandler)

qt_add_executable(tst_router tst_router.cpp testhelper.h)
target_link_libraries(tst_router PRIVATE QtCommon::HttpServer Qt6::Test)
add_test(NAME tst_router COMMAND tst_router)

qt_add_executable(tst_middleware tst_middleware.cpp testhelper.h)
target_link_libraries(tst_middleware PRIVATE QtCommon::HttpServer Qt6::Test)
add_test(NAME tst_middleware COMMAND tst_middleware)

qt_add_executable(tst_dispatch tst_dispatch.cpp testhelper.h)
target_link_libraries(tst_dispatch PRIVATE QtCommon::HttpServer Qt6::Test)
add_test(NAME tst_dispatch COMMAND tst_dispatch)