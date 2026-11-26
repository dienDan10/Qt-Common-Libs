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

qt_add_executable(tst_server tst_server.cpp)
target_link_libraries(tst_server
    PRIVATE
        QtCommon::HttpServer
        QtCommon::HttpClient
        Qt6::Test
)
add_test(NAME tst_server COMMAND tst_server)

qt_add_executable(tst_dbconfig tst_dbconfig.cpp)
target_link_libraries(tst_dbconfig PRIVATE QtCommon::DbAccess Qt6::Test)
add_test(NAME tst_dbconfig COMMAND tst_dbconfig)

qt_add_executable(tst_connectionlease tst_connectionlease.cpp)
target_link_libraries(tst_connectionlease PRIVATE QtCommon::DbAccess Qt6::Test)
add_test(NAME tst_connectionlease COMMAND tst_connectionlease)

qt_add_executable(tst_threadlocalprovider tst_threadlocalprovider.cpp)
target_link_libraries(tst_threadlocalprovider PRIVATE QtCommon::DbAccess Qt6::Test)
add_test(NAME tst_threadlocalprovider COMMAND tst_threadlocalprovider)

qt_add_executable(tst_sqlresult tst_sqlresult.cpp)
target_link_libraries(tst_sqlresult PRIVATE QtCommon::DbAccess Qt6::Test)
add_test(NAME tst_sqlresult COMMAND tst_sqlresult)

qt_add_executable(tst_sqlrow tst_sqlrow.cpp)
target_link_libraries(tst_sqlrow PRIVATE QtCommon::DbAccess Qt6::Test)
add_test(NAME tst_sqlrow COMMAND tst_sqlrow)

qt_add_executable(tst_sqlquery tst_sqlquery.cpp)
target_link_libraries(tst_sqlquery PRIVATE QtCommon::DbAccess Qt6::Test)
add_test(NAME tst_sqlquery COMMAND tst_sqlquery)

qt_add_executable(tst_transaction tst_transaction.cpp)
target_link_libraries(tst_transaction PRIVATE QtCommon::DbAccess Qt6::Test)
add_test(NAME tst_transaction COMMAND tst_transaction)

qt_add_executable(tst_database tst_database.cpp)
target_link_libraries(tst_database PRIVATE QtCommon::DbAccess Qt6::Test)
add_test(NAME tst_database COMMAND tst_database)

qt_add_executable(tst_dbconcurrency tst_dbconcurrency.cpp)
target_link_libraries(tst_dbconcurrency PRIVATE QtCommon::DbAccess Qt6::Test)
add_test(NAME tst_dbconcurrency COMMAND tst_dbconcurrency)