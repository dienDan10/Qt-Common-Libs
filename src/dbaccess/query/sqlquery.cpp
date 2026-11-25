/******************************************************************************
 * @file        sqlquery.cpp
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-25
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#include "sqlquery.h"
#include <QSqlError>

namespace QtCommon::dbaccess {

namespace {
QString placeholder(const QString &name)
{
    return name.startsWith(u':') ? name : u':' + name;
}
} // namespace

SqlQuery::SqlQuery(const ConnectionLease &lease, QString sql)
    : mLease(lease)
    , mSql(std::move(sql))
{}

SqlQuery &SqlQuery::bind(const QString &name, const QVariant &value)
{
    mBinds.insert(placeholder(name), value);
    return *this;
}

SqlQuery &SqlQuery::bind(const QVariantMap &values)
{
    for (auto it = values.cbegin(); it != values.cend(); it++) {
        bind(it.key(), it.value());
    }

    return *this;
}

SqlResult<ExecInfo> SqlQuery::exec() const
{
    return withExecuted([](QSqlQuery &q) { return ExecInfo{q.numRowsAffected(), q.lastInsertId()}; });
}

SqlError SqlQuery::run(QSqlQuery &q) const
{
    q.setForwardOnly(true); // only move forward with the result list

    if (!q.prepare(mSql)) {
        return SqlError::fromQt(q.lastError(), mSql);
    }

    for (auto it = mBinds.cbegin(); it != mBinds.cend(); ++it) {
        q.bindValue(it.key(), it.value());
    }

    if (!q.exec()) {
        return SqlError::fromQt(q.lastError(), mSql);
    }

    return {};
}

SqlError SqlQuery::fetchError(const QSqlQuery &q) const
{
    const QSqlError e = q.lastError();
    return e.isValid() ? SqlError::fromQt(e, mSql) : SqlError{};
}

SqlError SqlQuery::noRowError() const
{
    SqlError e = SqlError::make(SqlError::Type::NoRow, QStringLiteral("query returned no row"));
    e.sql = mSql;
    return e;
}

} // namespace QtCommon::dbaccess
