/******************************************************************************
 * @file        sqlquery.h
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-25
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#ifndef SQLQUERY_H
#define SQLQUERY_H

#include <QList>
#include <QSqlQuery>
#include <QVariant>
#include <QVariantMap>
#include "dbaccess/connection/connectionlease.h"
#include "dbaccess/core/sqlerror.h"
#include "dbaccess/query/execinfo.h"
#include "dbaccess/query/sqlresult.h"
#include "dbaccess/query/sqlrow.h"
namespace QtCommon::dbaccess {

namespace detail {
template<typename Mapper>
using MappedType = std::invoke_result_t<Mapper&, const SqlRow&>;
}

class SqlQuery
{
public:
    SqlQuery(const ConnectionLease& lease, QString sql);

    // -- config the query
    SqlQuery& bind(const QString& name, const QVariant& value);
    SqlQuery& bind(const QVariantMap& values);

    // -- finish: run query
    SqlResult<ExecInfo> exec() const;

    template<typename T>
    SqlResult<T> scalar() const
    {
        return withExecuted([this](QSqlQuery& q) {
            if (!q.next()) {
                if (SqlError err = fetchError(q); err.hasError()) {
                    return err;
                }
                return noRowError();
            }
            return q.value(0).value<T>();
        });
    }

    template<typename Mapper, typename R = detail::MappedType<Mapper>>
    SqlResult<std::optional<R>> first(Mapper&& map) const
    {
        static_assert(!std::is_void_v<R>, "mapper must return a value");
        return withExecuted([&](QSqlQuery& q) -> SqlResult<std::optional<R>> {
            if (!q.next()) {
                if (SqlError err = fetchError(q); err.hasError())
                    return err;
                return std::optional<R>{}; // chạy OK, không có dòng nào trả ra
            }
            const SqlRow row(q);
            return std::optional<R>(map(row));
        });
    }

    template<typename Mapper, typename R = detail::MappedType<Mapper>>
    SqlResult<std::optional<R>> list(Mapper&& map) const
    {
        static_assert(!std::is_void_v<R>, "mapper must return a value");
        return withExecuted([&](QSqlQuery& q) -> SqlResult<QList<R>> {
            QList<R> out;
            const SqlRow row(q);
            while (q.next()) {
                out.append(map(row));
            }

            if (SqlError err = fetchError(q); err.hasError()) {
                return err;
            }

            return std::move(out);
        });
    }

    const QString& sql() const { return mSql; }

private:
    SqlError run(QSqlQuery& q) const;
    SqlError fetchError(const QSqlQuery& q) const; // lỗi khi đọc (sau next() == false), None nếu không có
    SqlError noRowError() const;

    template<typename OnSuccess>
    auto withExecuted(OnSuccess&& onSuccess) const -> decltype(onSuccess(std::declval<QSqlQuery&>()))
    {
        if (!mLease.isValid()) {
            return mLease.error();
        }

        QSqlQuery q(mLease.database());
        if (SqlError err = run(q); err.hasError()) {
            return err;
        }

        return onSuccess(q);
    }

private:
    const ConnectionLease& mLease;
    QString mSql;
    QVariantMap mBinds;
};

} // namespace QtCommon::dbaccess

#endif // SQLQUERY_H
