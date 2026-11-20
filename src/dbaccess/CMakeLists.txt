find_package(Qt6 6.8 REQUIRED COMPONENTS Core Sql)

add_library(qtcommon_dbaccess STATIC
    sqlerror.h  sqlerror.cpp
    dbconfig.h dbconfig.cpp
)
add_library(QtCommon::DbAccess ALIAS qtcommon_dbaccess)

target_include_directories(qtcommon_dbaccess PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_features(qtcommon_dbaccess PUBLIC cxx_std_17)
set_target_properties(qtcommon_dbaccess PROPERTIES AUTOMOC ON)
target_link_libraries(qtcommon_dbaccess PUBLIC Qt6::Core Qt6::Sql)