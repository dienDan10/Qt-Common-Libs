find_package(Qt6 6.8 REQUIRED COMPONENTS Core Sql)

add_library(qtcommon_dbaccess STATIC
    core/sqlerror.h  core/sqlerror.cpp
    core/dbconfig.h core/dbconfig.cpp
    connection/iconnectionprovider.h
    connection/connectionlease.h connection/connectionlease.cpp
    connection/dbregistry.h connection/dbregistry.cpp
    connection/threadlocalprovider.h connection/threadlocalprovider.cpp
    query/sqlresult.h
    query/sqlrow.h
    query/execinfo.h
    query/sqlquery.h query/sqlquery.cpp
)
add_library(QtCommon::DbAccess ALIAS qtcommon_dbaccess)

target_include_directories(qtcommon_dbaccess PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_features(qtcommon_dbaccess PUBLIC cxx_std_17)
set_target_properties(qtcommon_dbaccess PROPERTIES AUTOMOC ON)
target_link_libraries(qtcommon_dbaccess PUBLIC Qt6::Core Qt6::Sql)