#include <QtTest>
#include "dbaccess/connection/threadlocalprovider.h"
#include "dbaccess/query/sqlrow.h"

using namespace QtCommon::dbaccess;
using namespace std::chrono_literals;

class TestSqlRow : public QObject
{
    Q_OBJECT
private slots:
    void readsTypedValuesAndNulls()
    {
        DbConfig cfg;
        cfg.name = "mem";
        cfg.driver = "QSQLITE";
        cfg.database = ":memory:";
        auto provider = std::make_shared<ThreadLocalProvider>(cfg);
        ConnectionLease lease = provider->acquire(0ms);
        QVERIFY2(lease.isValid(), qPrintable(lease.error().toString()));

        {
            QSqlQuery q(lease.database());
            QVERIFY(q.exec("CREATE TABLE people (id INTEGER, name TEXT, note TEXT)"));
            QVERIFY(q.exec("INSERT INTO people VALUES (1, 'An', NULL)"));
            QVERIFY(q.exec("SELECT id, name, note FROM people"));
            QVERIFY(q.next());

            SqlRow row(q);
            QCOMPARE(row.get<int>("id"), 1);
            QCOMPARE(row.get<QString>("name"), QString("An"));
            QCOMPARE(row.get<int>(0), 1);                    // theo index
            QCOMPARE(row.get<QString>("id"), QString("1"));  // QVariant tự đổi kiểu

            QVERIFY(row.isNull("note"));
            QVERIFY(!row.opt<QString>("note").has_value());  // NULL -> nullopt
            QCOMPARE(row.opt<QString>("name"), std::optional<QString>("An"));
        } // q hết scope trước lease
    }
};

QTEST_MAIN(TestSqlRow)
#include "tst_sqlrow.moc"