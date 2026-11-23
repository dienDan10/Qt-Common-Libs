/******************************************************************************
 * @file        dbconfig.h
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-20
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#ifndef DBCONFIG_H
#define DBCONFIG_H
#include <QString>
#include "sqlerror.h"
#include <chrono>

class QSqlDatabase;

namespace QtCommon::dbaccess {

struct DbConfig
{
    QString name = QStringLiteral("default");
    QString driver; // "QPSQL", "QMYSQL", "QSQLITE"
    QString host;
    int port = -1;
    QString database;
    QString user;
    QString password;
    QString connectOptions;

    bool isSqlite() const { return driver == QLatin1String("QSQLITE"); }

    SqlError validate() const; // type None = hợp lệ
    void applyTo(QSqlDatabase &db) const;

    QString validationQuery = QStringLiteral("SELECT 1");
    std::chrono::milliseconds validateAfterIdle{30000};
};

} // namespace QtCommon::dbaccess

#endif // DBCONFIG_H
