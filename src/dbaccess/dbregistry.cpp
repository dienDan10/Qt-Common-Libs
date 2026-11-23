/******************************************************************************
 * @file        dbregistry.cpp
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-23
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#include "dbregistry.h"

namespace QtCommon::dbaccess {

DbRegistry &DbRegistry::instance()
{
    static DbRegistry registry;
    return registry;
}

SqlError DbRegistry::add(std::shared_ptr<IConnectionProvider> provider)
{
    using T = SqlError::Type;
    if (!provider) {
        return SqlError::make(T::InvalidConfig, QStringLiteral("Provider is null"));
    }

    if (SqlError err = provider->config().validate(); err.hasError()) {
        return err;
    }

    const QString name = provider->config().name;
    QWriteLocker locker(&mLock);
    if (mProviders.contains(name)) {
        return SqlError::make(T::InvalidConfig, QStringLiteral("[%1] already registered"));
    }

    mProviders.insert(name, std::move(provider));
    return {};
}

bool DbRegistry::remove(const QString &name)
{
    QWriteLocker locker(&mLock);
    return mProviders.remove(name) > 0;
}

std::shared_ptr<IConnectionProvider> DbRegistry::provider(const QString &name) const
{
    QReadLocker locker(&mLock);
    return mProviders.value(name);
}

QStringList DbRegistry::names() const
{
    QReadLocker locker(&mLock);
    return mProviders.keys();
}

void DbRegistry::closeAll()
{
    QList<std::shared_ptr<IConnectionProvider>> all;
    {
        QReadLocker lock(&mLock);
        all = mProviders.values();
    }

    for (const auto &p : std::as_const(all)) {
        p->closeAll();
    }
}

} // namespace QtCommon::dbaccess