/******************************************************************************
 * @file        transaction.h
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-26
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "dbaccess/connection/connectionlease.h"
#include "dbaccess/core/sqlerror.h"
#include "dbaccess/query/sqlresult.h"
namespace QtCommon::dbaccess {

class Transaction
{
public:
    explicit Transaction(const ConnectionLease &lease);
    ~Transaction();

    Transaction(Transaction &&other) noexcept;
    Transaction &operator=(Transaction &&) = delete;
    Transaction(const Transaction &other) = delete;
    Transaction &operator=(const Transaction &) = delete;

    bool isActive() const { return mActive; }
    const SqlError &error() const { return mBeginError; }

    SqlResult<void> commit();
    SqlResult<void> rollback();

private:
    const ConnectionLease *mLease;
    bool mActive = false;
    SqlError mBeginError;

    SqlError noActiveError() const;
};

} // namespace QtCommon::dbaccess

#endif // TRANSACTION_H
