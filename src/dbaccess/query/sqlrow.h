/******************************************************************************
 * @file        sqlrow.h
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-24
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#ifndef SQLROW_H
#define SQLROW_H
#include <QSqlQuery>
#include <QSqlRecord>
namespace QtCommon::dbaccess {

class SqlRow
{
public:
    explicit SqlRow(const QSqlQuery& query)
        : mQuery(query) {};
    SqlRow(const SqlRow&) = delete;
    SqlRow& operator=(const SqlRow&) = delete;

    template<typename T>
    T get(const QString& column) const
    {
        return mQuery.value(column).value<T>();
    }

    template<typename T>
    T get(int index) const
    {
        return mQuery.value(index).value<T>();
    }

    template<typename T>
    std::optional<T> opt(const QString& column) const
    {
        if (mQuery.isNull(column))
            return std::nullopt;
        return get<T>(column);
    }

    template<typename T>
    std::optional<T> opt(int index) const
    {
        if (mQuery.isNull(index))
            return std::nullopt;
        return get<T>(index);
    }

    bool isNull(const QString& column) const { return mQuery.isNull(column); }
    bool isNull(int index) const { return mQuery.isNull(index); }

    QSqlRecord record() const { return mQuery.record(); }

private:
    const QSqlQuery& mQuery;
};

} // namespace QtCommon::dbaccess

#endif // SQLROW_H
