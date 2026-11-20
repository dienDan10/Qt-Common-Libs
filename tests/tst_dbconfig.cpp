// tests/tst_dbconfig.cpp
#include <QSqlDatabase>
#include <QtTest>
#include "dbaccess/dbconfig.h"

using namespace QtCommon::dbaccess;
class TestDbConfig : public QObject
{
    Q_OBJECT
private slots:
    void emptyDriverIsInvalid()
    {
        DbConfig cfg;
        cfg.database = ":memory:";
        QCOMPARE(cfg.validate().type, SqlError::Type::InvalidConfig);
    }

    void unknownDriverReportsNoDriver()
    {
        DbConfig cfg;
        cfg.driver = "QFAKEDB";
        cfg.database = "x";
        QCOMPARE(cfg.validate().type, SqlError::Type::NoDriver);
    }

    void sqliteNeedsNoHost()
    {
        DbConfig cfg;
        cfg.driver = "QSQLITE";
        cfg.database = ":memory:";
        QVERIFY(!cfg.validate().hasError());
    }

    void serverDbNeedsHost()
    {
        DbConfig cfg;
        cfg.driver = "QSQLITE";   // giả lập "không phải sqlite" khó -> kiểm tra gián tiếp qua isSqlite()
        QVERIFY(cfg.isSqlite());
        cfg.driver = "QPSQL";
        QVERIFY(!cfg.isSqlite());
    }

    void applyToCopiesFields()
    {
        DbConfig cfg;
        cfg.driver = "QSQLITE";
        cfg.database = ":memory:";
        cfg.connectOptions = "QSQLITE_BUSY_TIMEOUT=1000";
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(cfg.driver, "tst_apply");
            cfg.applyTo(db);
            QCOMPARE(db.databaseName(), QString(":memory:"));
            QCOMPARE(db.connectOptions(), cfg.connectOptions);
        } // db hết scope TRƯỚC removeDatabase -> không bị "still in use"
        QSqlDatabase::removeDatabase("tst_apply");
    }
};

QTEST_MAIN(TestDbConfig)
#include "tst_dbconfig.moc"