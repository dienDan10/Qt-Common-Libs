/******************************************************************************
 * @file        iconnectionprovider.h
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-23
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#ifndef ICONNECTIONPROVIDER_H
#define ICONNECTIONPROVIDER_H
#include "connectionlease.h"
#include "dbaccess/core/dbconfig.h"
#include <chrono>
#include <memory>

namespace QtCommon::dbaccess {

class IConnectionProvider : public std::enable_shared_from_this<IConnectionProvider>
{
public:
    virtual ~IConnectionProvider() = default;
    IConnectionProvider(const IConnectionProvider &) = delete;            // don't allow copy constructor
    IConnectionProvider &operator=(const IConnectionProvider &) = delete; // don't allow copy assignment

    virtual const DbConfig &config() const = 0;                             // get config of this DB
    virtual ConnectionLease acquire(std::chrono::milliseconds timeout) = 0; // get a connection to DB
    virtual void closeAll() = 0;

protected:
    IConnectionProvider() = default;
    ConnectionLease makeLease(const QString &connectionName)
    {
        return ConnectionLease(shared_from_this(), connectionName);
    };

private:
    friend class ConnectionLease;
    virtual void release(const QString &connectionName) noexcept = 0; // return this connection
};

} // namespace QtCommon::dbaccess

#endif // ICONNECTIONPROVIDER_H
