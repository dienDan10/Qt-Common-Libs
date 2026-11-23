#include <QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QThread>
#include <memory>
#include "dbaccess/threadlocalprovider.h"

using namespace QtCommon::dbaccess;
using namespace std::chrono_literals;

namespace {
DbConfig memConfig(const QString &name = "mem")
{
    DbConfig cfg;
    cfg.name = name;
    cfg.driver = "QSQLITE";
    cfg.database = ":memory:";
    return cfg;
}
} // namespace

class TestThreadLocalProvider : public QObject
{
    Q_OBJECT
private slots:
    void acquireOpensRealConnection()
    {
        auto p = std::make_shared<ThreadLocalProvider>(memConfig());
        ConnectionLease lease = p->acquire(0ms);
        QVERIFY2(lease.isValid(), qPrintable(lease.error().toString()));

        QSqlDatabase db = lease.database();
        QVERIFY(db.isOpen());
        QSqlQuery q(db);
        QVERIFY(q.exec("SELECT 1"));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 1);
    } // hủy theo thứ tự ngược: q, db, lease, p -> query chết trước khi provider dọn

    void sameThreadReusesConnection()
    {
        auto p = std::make_shared<ThreadLocalProvider>(memConfig());
        QString first, second;
        { ConnectionLease l = p->acquire(0ms); first = l.connectionName(); }
        { ConnectionLease l = p->acquire(0ms); second = l.connectionName(); }
        QCOMPARE(first, second);
    }

    void eachThreadOwnsConnectionAndCleansUp()
    {
        auto p = std::make_shared<ThreadLocalProvider>(memConfig());
        QString mainName;
        { ConnectionLease l = p->acquire(0ms); mainName = l.connectionName(); }

        QString workerName;
        bool workerOpen = false;
        std::unique_ptr<QThread> t(QThread::create([&] {
            ConnectionLease l = p->acquire(0ms);
            workerName = l.connectionName();
            workerOpen = l.database().isOpen();
        }));
        t->start();
        QVERIFY(t->wait(5000));

        QVERIFY(workerOpen);
        QVERIFY(workerName != mainName);
        QVERIFY(!QSqlDatabase::contains(workerName));   // đã dọn khi thread phát finished()
        QVERIFY(QSqlDatabase::contains(mainName));      // của main vẫn còn
    }

    void twoProvidersSameConfigNameDoNotClash()
    {
        auto a = std::make_shared<ThreadLocalProvider>(memConfig("dup"));
        auto b = std::make_shared<ThreadLocalProvider>(memConfig("dup"));
        ConnectionLease la = a->acquire(0ms);
        ConnectionLease lb = b->acquire(0ms);
        QVERIFY(la.isValid() && lb.isValid());
        QVERIFY(la.connectionName() != lb.connectionName());
    }

    void openFailureGivesFailedLease()
    {
        DbConfig cfg = memConfig("bad");
        cfg.database = "/thu-muc-khong-ton-tai/x.db";   // SQLite không tạo được file
        auto p = std::make_shared<ThreadLocalProvider>(cfg);
        ConnectionLease l = p->acquire(0ms);
        QVERIFY(!l.isValid());
        QCOMPARE(l.error().type, SqlError::Type::Connection);
    }

    void reopensWhenClosed()
    {
        auto p = std::make_shared<ThreadLocalProvider>(memConfig());
        { ConnectionLease l = p->acquire(0ms); l.database().close(); }   // giả lập connection bị đóng
        ConnectionLease l = p->acquire(0ms);
        QVERIFY(l.isValid());
        QVERIFY(l.database().isOpen());
    }

    void closeAllRejectsAndCleansCurrentThread()
    {
        auto p = std::make_shared<ThreadLocalProvider>(memConfig());
        QString name;
        { ConnectionLease l = p->acquire(0ms); name = l.connectionName(); }

        p->closeAll();
        QVERIFY(!QSqlDatabase::contains(name));
        QVERIFY(!p->acquire(0ms).isValid());
    }
};

QTEST_MAIN(TestThreadLocalProvider)
#include "tst_threadlocalprovider.moc"