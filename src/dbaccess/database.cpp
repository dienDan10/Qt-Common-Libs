/******************************************************************************
 * @file        database.cpp
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-26
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#include "database.h"

namespace QtCommon::dbaccess {

namespace {
const ConnectionLease &moveFromLease()
{
    static const ConnectionLease lease = ConnectionLease::failed(
        SqlError::make(SqlError::Type::Unknown, QStringLiteral("Database object was moved-from")));

    return lease;
}
} // namespace

Database Database::acquire(const QString &name, std::chrono::milliseconds timeout)
{
    return acquire(DbRegistry::instance(), name, timeout);
}

Database Database::acquire(DbRegistry &registry,
                           const QString &name,
                           std::chrono::milliseconds timeout)
{
    auto provider = registry.provider(name);
    if (!provider) {
        return Database(std::make_unique<ConnectionLease>(ConnectionLease::failed(
            SqlError::make(SqlError::Type::InvalidConfig,
                           QStringLiteral("no database registered as '%1'").arg(name)))));
    }

    return Database(std::make_unique<ConnectionLease>(provider->acquire(timeout)));
}

SqlQuery Database::query(QString sql) const &
{
    return SqlQuery(lease(), std::move(sql));
}

Transaction Database::transaction() const &
{
    return Transaction(lease());
}

const ConnectionLease &Database::lease() const
{
    return mLease ? *mLease : moveFromLease();
}
} // namespace QtCommon::dbaccess