/******************************************************************************
 * @file        threadlocalprovider.h
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-23
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#ifndef THREADLOCALPROVIDER_H
#define THREADLOCALPROVIDER_H

#include "iconnectionprovider.h"
#include <atomic>

namespace QtCommon::dbaccess {

class ThreadLocalProvider final : public IConnectionProvider
{
public:
    explicit ThreadLocalProvider(DbConfig config);
    ~ThreadLocalProvider() override;

    // IConnectionProvider interface
public:
    const DbConfig &config() const override { return mConfig; };
    ConnectionLease acquire(std::chrono::milliseconds timeout) override;
    void closeAll() override;
    bool isClosed() const { return mClosed.load(); }

private:
    void release(const QString &connectionName) noexcept override;
    QString makeConnectionName() const;

private:
    const DbConfig mConfig;
    const quint64 mId;
    std::atomic<bool> mClosed{false};
};

} // namespace QtCommon::dbaccess

#endif // THREADLOCALPROVIDER_H
