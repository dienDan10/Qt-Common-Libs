/******************************************************************************
 * @file        transaction.cpp
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-26
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#include "transaction.h"
#include <QSqlDatabase>
#include <QSqlError>

namespace QtCommon::dbaccess {

Transaction::Transaction(const ConnectionLease &lease)
    : mLease(&lease)
{
    if (!lease.isValid()) {
        mBeginError = lease.error();
        return;
    }

    QSqlDatabase db = lease.database();
    if (!db.transaction()) {
        mBeginError = SqlError::fromQt(db.lastError());
        if (!mBeginError.hasError()) { // incase some driver don't support transaction
            mBeginError = SqlError::make(SqlError::Type::Transaction,
                                         QStringLiteral("Cannot begin transaction"));
        }
        return;
    }
    mActive = true;
}

Transaction::~Transaction()
{
    if (!mActive)
        return;

    QSqlDatabase db = mLease->database();
    if (!db.rollback()) {
        qWarning("Transaction: implicit rollback failed: %s", qPrintable(db.lastError().text()));
    }
}

Transaction::Transaction(Transaction &&other) noexcept
    : mLease(other.mLease)
    , mActive(std::exchange(other.mActive, false))
    , mBeginError(std::move(other.mBeginError))
{}

SqlResult<void> Transaction::commit()
{
    if (!mActive) {
        return noActiveError();
    }

    QSqlDatabase db = mLease->database();
    mActive = false;
    if (db.commit())
        return {};
    // rollback if cannot commit
    SqlError err = SqlError::fromQt(db.lastError());
    db.rollback();
    return err;
}

SqlResult<void> Transaction::rollback()
{
    if (!mActive)
        return {};

    QSqlDatabase db = mLease->database();
    mActive = false;
    if (db.rollback())
        return {};
    return SqlError::fromQt(db.lastError());
}

SqlError Transaction::noActiveError() const
{
    if (mBeginError.hasError())
        return mBeginError;
    return SqlError::make(SqlError::Type::Transaction,
                          QStringLiteral("Transaction is not active (already commited)"));
}
} // namespace QtCommon::dbaccess
