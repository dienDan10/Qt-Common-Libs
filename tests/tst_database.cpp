#include <QtTest>
#include "dbaccess/dbaccess.h"

using namespace QtCommon::dbaccess;
using ErrType = SqlError::Type;

class TestDatabase : public QObject
{
    Q_OBJECT

    std::unique_ptr<DbRegistry> mRegistry;   // registry riêng cho mỗi test

private slots:
    void init()
    {
        mRegistry = std::make_unique<DbRegistry>();
        DbConfig cfg;
        cfg.name = "main";
        cfg.driver = "QSQLITE";
        cfg.database = ":memory:";
        QVERIFY(!mRegistry->add(std::make_shared<ThreadLocalProvider>(cfg)).hasError());
    }

    void cleanup()
    {
        mRegistry.reset();   // provider sống tiếp tới khi Database cuối cùng trả lease
    }

    void unknownNameGivesInvalid()
    {
        auto db = Database::acquire(*mRegistry, "nope");
        QVERIFY(!db.isValid());
        QCOMPARE(db.error().type, ErrType::InvalidConfig);
        QCOMPARE(db.query("SELECT 1").scalar<int>().error().type, ErrType::InvalidConfig);
    }

    void queryThroughFacade()
    {
        auto db = Database::acquire(*mRegistry, "main");
        QVERIFY2(db.isValid(), qPrintable(db.error().toString()));
        QVERIFY(db.query("CREATE TABLE t (v INTEGER)").exec().ok());
        QVERIFY(db.query("INSERT INTO t VALUES (:v)").bind("v", 7).exec().ok());
        QCOMPARE(db.query("SELECT v FROM t").scalar<int>().value(), 7);
    }

    void transactionThroughFacade()
    {
        auto db = Database::acquire(*mRegistry, "main");
        QVERIFY(db.query("CREATE TABLE t (v INTEGER)").exec().ok());
        {
            auto tx = db.transaction();
            QVERIFY(db.query("INSERT INTO t VALUES (1)").exec().ok());
        }   // không commit -> rollback
        QCOMPARE(db.query("SELECT COUNT(*) FROM t").scalar<int>().value(), 0);

        auto tx = db.transaction();
        QVERIFY(db.query("INSERT INTO t VALUES (1)").exec().ok());
        QVERIFY(tx.commit().ok());
        QCOMPARE(db.query("SELECT COUNT(*) FROM t").scalar<int>().value(), 1);
    }

    void moveKeepsExistingQueriesValid()
    {
        auto db = Database::acquire(*mRegistry, "main");
        const SqlQuery q = db.query("SELECT 42");   // q giữ địa chỉ lease (nằm trên heap)

        Database moved = std::move(db);             // chỉ con trỏ đổi chủ, lease không đổi chỗ
        QCOMPARE(q.scalar<int>().value(), 42);      // q vẫn dùng được
        QVERIFY(moved.isValid());

        QVERIFY(!db.isValid());                     // object đã bị move: lỗi chứ không crash
        QVERIFY(!db.query("SELECT 1").scalar<int>().ok());
    }

    void sameThreadSharesConnection()
    {
        auto a = Database::acquire(*mRegistry, "main");
        auto b = Database::acquire(*mRegistry, "main");
        QCOMPARE(a.lease().connectionName(), b.lease().connectionName());   // thread-local: cùng connection
    }

    // Không test được lúc chạy (nhưng là chủ đích): những dòng sau KHÔNG compile
    // void deletedOnTemporary()
    // {
    //     auto q = Database::acquire(*mRegistry, "main").query("SELECT 1");
    //     auto tx = Database::acquire(*mRegistry, "main").transaction();
    // }
};

QTEST_MAIN(TestDatabase)
#include "tst_database.moc"