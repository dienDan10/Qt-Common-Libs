/******************************************************************************
 * @file        dbregistry.h
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-23
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#ifndef DBREGISTRY_H
#define DBREGISTRY_H

#include <QHash>
#include <QReadWriteLock>
#include "dbaccess/connection/iconnectionprovider.h"
#include <memory>

namespace QtCommon::dbaccess {

class DbRegistry
{
public:
    static DbRegistry &instance();

    DbRegistry() = default;
    DbRegistry(const DbRegistry &) = delete;
    DbRegistry &operator=(const DbRegistry &) = delete;

    SqlError add(std::shared_ptr<IConnectionProvider> provider);
    bool remove(const QString &name);
    std::shared_ptr<IConnectionProvider> provider(const QString &name) const;
    QStringList names() const;
    void closeAll();

private:
    mutable QReadWriteLock mLock;
    QHash<QString, std::shared_ptr<IConnectionProvider>> mProviders;
};

} // namespace QtCommon::dbaccess

#endif // DBREGISTRY_H
