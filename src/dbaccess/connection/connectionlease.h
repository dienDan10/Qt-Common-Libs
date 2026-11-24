/******************************************************************************
 * @file        connectionlease.h
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-23
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#ifndef CONNECTIONLEASE_H
#define CONNECTIONLEASE_H

#include <QString>
#include "dbaccess/core/sqlerror.h"
#include <memory>

class QSqlDatabase;

namespace QtCommon::dbaccess {

class IConnectionProvider;

class ConnectionLease
{
public:
    static ConnectionLease failed(SqlError error);

    ConnectionLease(ConnectionLease &&other) noexcept;
    ConnectionLease &operator=(ConnectionLease &&other) noexcept;
    ConnectionLease(ConnectionLease &other) = delete;            // don't allow copy constructor
    ConnectionLease &operator=(ConnectionLease &other) = delete; // don't allow copy assignment
    ~ConnectionLease();

    bool isValid() const { return mProvider != nullptr; };
    const SqlError &error() { return mError; };
    const QString &connectionName() const { return mName; };

    QSqlDatabase database() const;

private:
    friend class IConnectionProvider;
    ConnectionLease() = default;
    ConnectionLease(std::shared_ptr<IConnectionProvider> provider, QString name);
    void releaseNow() noexcept;

    std::shared_ptr<IConnectionProvider> mProvider;
    QString mName;
    SqlError mError;
};

} // namespace QtCommon::dbaccess

#endif // CONNECTIONLEASE_H
