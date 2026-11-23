/******************************************************************************
 * @file        connectionlease.cpp
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-23
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#include "connectionlease.h"
#include <QSqlDatabase>
#include "iconnectionprovider.h"

namespace QtCommon::dbaccess {

ConnectionLease::ConnectionLease(std::shared_ptr<IConnectionProvider> provider, QString name)
    : mProvider(std::move(provider))
    , mName(std::move(name))
{}

void ConnectionLease::releaseNow() noexcept
{
    if (!mProvider)
        return;

    auto provider = std::move(mProvider);
    provider->release(mName);
    mName.clear();
}

ConnectionLease ConnectionLease::failed(SqlError error)
{
    if (!error.hasError()) {
        error = SqlError::make(SqlError::Type::Unknown,
                               QStringLiteral("lease failed without reason"));
    }

    ConnectionLease lease;
    lease.mError = std::move(error);
    return lease;
}

ConnectionLease::ConnectionLease(ConnectionLease &&other) noexcept
    : mProvider(std::move(other.mProvider))
    , mName(std::move(other.mName))
    , mError(std::move(other.mError))
{}

ConnectionLease::~ConnectionLease()
{
    releaseNow();
}

QSqlDatabase ConnectionLease::database() const
{
    if (!mProvider)
        return {};

    return QSqlDatabase::database(mName, false); // don't auto open it here
}

ConnectionLease &ConnectionLease::operator=(ConnectionLease &&other) noexcept
{
    if (this != &other) {
        releaseNow();
        mProvider = std::move(other.mProvider);
        mName = std::move(other.mName);
        mError = std::move(mError);
    }

    return *this;
}

ConnectionLease IConnectionProvider::makeLease(const QString &connectionName)
{
    return ConnectionLease(shared_from_this(), connectionName);
}

} // namespace QtCommon::dbaccess
