/******************************************************************************
 * @file        dbconfig.cpp
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-20
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#include "dbaccess/core/dbconfig.h"
#include <QSqlDatabase>

namespace QtCommon::dbaccess {

SqlError DbConfig::validate() const
{
    using T = SqlError::Type;

    if (name.isEmpty()) {
        return SqlError::make(T::InvalidConfig, QStringLiteral("config name is empty"));
    }
    if (driver.isEmpty()) {
        return SqlError::make(T::InvalidConfig, QStringLiteral("[%1] driver is empty").arg(name));
    }
    if (!QSqlDatabase::isDriverAvailable(driver)) {
        return SqlError::make(T::NoDriver,
                              QStringLiteral("[%1] driver %2 not available, have: %3")
                                  .arg(name, driver, QSqlDatabase::drivers().join(", ")));
    }
    if (database.isEmpty())
        return SqlError::make(T::InvalidConfig, QStringLiteral("[%1] database is empty").arg(name));
    if (!isSqlite() && host.isEmpty())
        return SqlError::make(T::InvalidConfig, QStringLiteral("[%1] host is empty").arg(name));

    return {};
}

void DbConfig::applyTo(QSqlDatabase &db) const
{
    db.setHostName(host);
    db.setPort(port);
    db.setDatabaseName(database);
    db.setUserName(user);
    db.setPassword(password);
    db.setConnectOptions(connectOptions);
}

} // namespace QtCommon::dbaccess