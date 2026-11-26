/******************************************************************************
 * @file        database.h
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-26
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#ifndef DATABASE_H
#define DATABASE_H

#include "dbaccess/connection/dbregistry.h"
#include "dbaccess/query/sqlquery.h"
#include "dbaccess/query/transaction.h"
#include <chrono>
#include <memory>

namespace QtCommon::dbaccess {

class Database
{
public:
    static constexpr std::chrono::milliseconds DefaultTimeout{5000};
    static Database acquire(const QString &name = QStringLiteral("default"),
                            std::chrono::milliseconds timeout = DefaultTimeout);
    static Database acquire(DbRegistry &registry,
                            const QString &name,
                            std::chrono::milliseconds timeout = DefaultTimeout);

    Database(Database &&) noexcept = default;
    Database &operator=(Database &&) noexcept = default;
    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    bool isValid() const { return lease().isValid(); }
    const SqlError &error() const { return lease().error(); }

    SqlQuery query(QString sql) const &;
    SqlQuery query(QString sql) && = delete; // không cho gọi đối với rvalue

    Transaction transaction() const &;
    Transaction transaction() && = delete; // không cho gọi với rvalue

    // để public phòng trường hợp cần lấy thẳng connection
    const ConnectionLease &lease() const;

private:
    explicit Database(std::unique_ptr<ConnectionLease> lease)
        : mLease(std::move(lease))
    {}

    std::unique_ptr<ConnectionLease> mLease;
};

} // namespace QtCommon::dbaccess

#endif // DATABASE_H
