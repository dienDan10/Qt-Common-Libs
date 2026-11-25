#ifndef SQLERROR_H
#define SQLERROR_H

#include <QString>
class QSqlError;

namespace QtCommon::dbaccess {

struct SqlError
{
    enum class Type {
        None,
        InvalidConfig,
        NoDriver,
        Timeout,
        NoRow,
        Connection,  //  Qsql
        Statement,   //  Qsql
        Transaction, // Qsql
        Unknown
    };

    Type type = Type::None;
    QString message;
    QString nativeCode; // Mã lỗi gốc của DB
    QString sql;

    bool hasError() const { return type != Type::None; }
    QString toString() const;

    static SqlError make(Type type, const QString &message);
    static SqlError fromQt(const QSqlError &error, const QString &sql = {});
};

} // namespace QtCommon::dbaccess

#endif