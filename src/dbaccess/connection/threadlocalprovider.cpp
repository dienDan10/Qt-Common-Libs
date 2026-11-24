/******************************************************************************
 * @file        threadlocalprovider.cpp
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-23
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#include "dbaccess/connection/threadlocalprovider.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>

namespace QtCommon::dbaccess {

namespace {

std::atomic<quint64> gNextProviderId{1};

void closeAndRemove(const QString &name)
{
    {
        QSqlDatabase db = QSqlDatabase::database(name, false);
        if (db.isValid()) {
            db.close();
        }
    }

    QSqlDatabase::removeDatabase(name);
}

struct Entry
{
    QString connectionName;
    QElapsedTimer lastUsed;
};

struct ThreadConnections
{
    QHash<quint64, Entry> entries;
    bool cleanupHooked = false;

    void drop(quint64 providerId)
    {
        const auto it = entries.find(providerId);
        if (it == entries.end())
            return;
        const QString name = it->connectionName;
        entries.erase(it);
        closeAndRemove(name);
    }

    void dropAll()
    {
        const auto all = std::exchange(entries, {});
        for (const Entry &e : all) {
            closeAndRemove(e.connectionName);
        }
    }

    ~ThreadConnections()
    {
        if (QCoreApplication::instance())
            dropAll();
    }
};

thread_local ThreadConnections tConnections;

void hookThreadCleanup()
{
    if (tConnections.cleanupHooked)
        return;
    tConnections.cleanupHooked = true;

    QObject::connect(QThread::currentThread(), &QThread::finished, [] { tConnections.dropAll(); });
}

SqlError ensureUsable(Entry &entry, const DbConfig &cfg)
{
    QSqlDatabase db = QSqlDatabase::database(entry.connectionName, false);

    if (!db.isOpen()) {
        return db.open() ? SqlError{} : SqlError::fromQt(db.lastError());
    }

    const bool idleTooLong = !cfg.validationQuery.isEmpty() && entry.lastUsed.isValid()
                             && entry.lastUsed.elapsed() >= cfg.validateAfterIdle.count();
    if (!idleTooLong)
        return {};

    bool alive = false;
    {
        QSqlQuery ping(db);
        alive = ping.exec(cfg.validationQuery);
    }
    if (alive)
        return {};

    db.close();
    return db.open() ? SqlError{} : SqlError::fromQt(db.lastError());
}

} // namespace

ThreadLocalProvider::ThreadLocalProvider(DbConfig config)
    : mConfig(std::move(config))
    , mId(gNextProviderId.fetch_add(1))
{}

ThreadLocalProvider::~ThreadLocalProvider()
{
    tConnections.drop(mId);
}

ConnectionLease ThreadLocalProvider::acquire(std::chrono::milliseconds timeout)
{
    if (mClosed.load()) {
        tConnections.drop(mId);
        return ConnectionLease::failed(
            SqlError::make(SqlError::Type::Connection,
                           QStringLiteral("[%1] provider is closed").arg(mConfig.name)));
    }

    auto it = tConnections.entries.find(mId);
    if (it == tConnections.entries.end()) {
        const QString name = makeConnectionName();
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(mConfig.driver, name);
            mConfig.applyTo(db);
        }
        hookThreadCleanup();
        it = tConnections.entries.insert(mId, Entry{name, {}});
    }

    if (SqlError err = ensureUsable(*it, mConfig); err.hasError()) {
        return ConnectionLease::failed(std::move(err));
    }

    it->lastUsed.restart();
    return makeLease(it->connectionName);
}

void ThreadLocalProvider::closeAll()
{
    mClosed.store(true);
    tConnections.drop(mId);
}

void ThreadLocalProvider::release(const QString &connectionName) noexcept
{
    const auto it = tConnections.entries.find(mId);
    if (it != tConnections.entries.end())
        it->lastUsed.restart();
}

QString ThreadLocalProvider::makeConnectionName() const
{
    return QStringLiteral("qtc:%1:%2:%3")
        .arg(mConfig.name)
        .arg(mId)
        .arg(quintptr(QThread::currentThread()), 0, 16);
}
} // namespace QtCommon::dbaccess