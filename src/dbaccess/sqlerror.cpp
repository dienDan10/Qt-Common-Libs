#include "sqlerror.h"
#include <QSqlError>

namespace QtCommon::dbaccess {

SqlError SqlError::make(Type type, const QString &message)
{
    SqlError e;
    e.type = type;
    e.message = message;
    return e;
}

SqlError SqlError::fromQt(const QSqlError &error, const QString &sql)
{
    SqlError e;
    switch (error.type()) {
    case QSqlError::NoError:
        return e;
    case QSqlError::ConnectionError:
        e.type = Type::Connection;
        break;
    case QSqlError::StatementError:
        e.type = Type::Statement;
        break;
    case QSqlError::TransactionError:
        e.type = Type::Transaction;
        break;
    default:
        e.type = Type::Unknown;
        break;
    }

    e.message = error.text();
    e.nativeCode = error.nativeErrorCode();
    e.sql = sql;
    return e;
}

QString SqlError::toString() const
{
    if (!hasError()) {
        return QStringLiteral("No Error");
    }
    QString s = message;
    if (!nativeCode.isEmpty()) {
        s += QStringLiteral(" [%1]").arg(nativeCode);
    }
    if (!sql.isEmpty()) {
        s += QStringLiteral(" | sql: %1").arg(sql);
    }
    return s;
}

} // namespace QtCommon::dbaccess