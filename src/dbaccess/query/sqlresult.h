/******************************************************************************
 * @file        sqlresult.h
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-24
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#ifndef SQLRESULT_H
#define SQLRESULT_H
#include "dbaccess/core/sqlerror.h"
#include <optional>
namespace QtCommon::dbaccess {

namespace detail {
inline SqlError ensureError(SqlError e)
{
    if (!e.hasError()) {
        e = SqlError::make(SqlError::Type::Unknown, QStringLiteral("Failed without reason"));
    }

    return e;
}

} // namespace detail

template<typename T>
class SqlResult
{
public:
    SqlResult(T value)
        : mValue(std::move(value))
    {}
    SqlResult(SqlError error)
        : mError(detail::ensureError(std::move(error)))
    {}

    bool ok() const { return mValue.has_value(); }
    explicit operator bool() const { return ok(); }
    const SqlError &error() const { return mError; }

    const T &value() const & // can only be called by a lvalue object
    {
        if (!mValue) {
            Q_ASSERT_X(ok(), "SqlResult::value", qPrintable(mError.toString()));
            static const T fallback{}; // use static to make fallback live through the process life
            return fallback;
        }

        return *mValue;
    }

    T value() && // can only be called by a rvalue object
    {
        Q_ASSERT_X(ok(), "SqlResult::value", qPrintable(mError.toString()));
        return mValue ? std::move(*mValue) : T{};
    }

    T valueOr(T fallback) const & { return mValue ? *mValue : std::move(fallback); }
    T valueOr(T fallback) && { return mValue ? std::move(*mValue) : std::move(fallback); }

private:
    std::optional<T> mValue;
    SqlError mError; // Type None when ok
};

// use for query operation which does not return anything
template<>
class SqlResult<void>
{
public:
    SqlResult() = default; // thành công
    SqlResult(SqlError error)
        : mError(detail::ensureError(std::move(error)))
    {}

    bool ok() const { return !mError.hasError(); }
    explicit operator bool() const { return ok(); }
    const SqlError &error() const { return mError; }

private:
    SqlError mError;
};

} // namespace QtCommon::dbaccess

#endif // SQLRESULT_H
