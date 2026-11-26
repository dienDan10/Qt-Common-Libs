#include <QtTest>
#include <optional>
#include "dbaccess/connection/threadlocalprovider.h"
#include "dbaccess/query/sqlquery.h"
#include "dbaccess/query/transaction.h"

using namespace QtCommon::dbaccess;
using namespace std::chrono_literals;
using ErrType = SqlError::Type;

class TestTransaction : public QObject
{
    Q_OBJECT

    std::shared_ptr<ThreadLocalProvider> mProvider;
    std::optional<ConnectionLease> mLease;

    const ConnectionLease &lease() const { return *mLease; }

    int countRows() const
    {
        return SqlQuery(lease(), "SELECT COUNT(*) FROM items").scalar<int>().valueOr(-1);
    }

    bool insertItem(int id) const
    {
        return SqlQuery(lease(), "INSERT INTO items (id) VALUES (:id)").bind("id", id).exec().ok();
    }

    // Giả lập một hàm nghiệp vụ có đường return sớm
    SqlResult<void> insertTwoButFailSecond() const
    {
        Transaction tx(lease());
        if (!tx.isActive())
            return tx.error();
        if (!insertItem(1))
            return SqlError::make(ErrType::Statement, "first failed");
        if (!insertItem(1))   // trùng khóa -> lỗi -> return sớm, KHÔNG commit
            return SqlError::make(ErrType::Statement, "second failed");
        return tx.commit();
    }

private slots:
    void init()
    {
        DbConfig cfg;
        cfg.name = "mem";
        cfg.driver = "QSQLITE";
        cfg.database = ":memory:";
        mProvider = std::make_shared<ThreadLocalProvider>(cfg);
        mLease.emplace(mProvider->acquire(0ms));
        QVERIFY2(mLease->isValid(), qPrintable(mLease->error().toString()));
        QVERIFY(SqlQuery(lease(), "CREATE TABLE items (id INTEGER PRIMARY KEY)").exec().ok());
    }

    void cleanup()
    {
        mLease.reset();
        mProvider.reset();
    }

    void commitPersists()
    {
        Transaction tx(lease());
        QVERIFY(tx.isActive());
        QVERIFY(insertItem(1));
        QVERIFY(tx.commit().ok());
        QVERIFY(!tx.isActive());
        QCOMPARE(countRows(), 1);
    }

    void destructorRollsBack()
    {
        {
            Transaction tx(lease());
            QVERIFY(insertItem(1));
            QCOMPARE(countRows(), 1);   // trong transaction: thấy dòng mới
        }                               // không commit -> rollback
        QCOMPARE(countRows(), 0);
    }

    void explicitRollback()
    {
        Transaction tx(lease());
        QVERIFY(insertItem(1));
        QVERIFY(tx.rollback().ok());
        QCOMPARE(countRows(), 0);
        QVERIFY(tx.rollback().ok());    // rollback thừa: vô hại
    }

    void commitTwiceFails()
    {
        Transaction tx(lease());
        QVERIFY(tx.commit().ok());
        auto second = tx.commit();
        QVERIFY(!second.ok());
        QCOMPARE(second.error().type, ErrType::Transaction);
    }

    void earlyReturnRollsBackAndConnectionStaysClean()
    {
        auto r = insertTwoButFailSecond();
        QVERIFY(!r.ok());
        QCOMPARE(countRows(), 0);             // dòng đầu tiên cũng bị rollback

        Transaction next(lease());            // connection không còn treo transaction:
        QVERIFY2(next.isActive(), qPrintable(next.error().toString()));   // BEGIN mới chạy được
        QVERIFY(insertItem(5));
        QVERIFY(next.commit().ok());
        QCOMPARE(countRows(), 1);
    }

    void nestedOnSameConnectionFails()
    {
        Transaction outer(lease());
        QVERIFY(outer.isActive());

        Transaction inner(lease());           // cùng connection -> driver từ chối BEGIN lần 2
        QVERIFY(!inner.isActive());
        QCOMPARE(inner.error().type, ErrType::Transaction);
        QVERIFY(!inner.commit().ok());        // commit trên transaction lỗi -> trả lại lỗi BEGIN

        QVERIFY(insertItem(1));
        QVERIFY(outer.commit().ok());         // transaction ngoài vẫn hoạt động bình thường
        QCOMPARE(countRows(), 1);
    }

    void failedLeaseGivesInactive()
    {
        const ConnectionLease bad = ConnectionLease::failed(SqlError::make(ErrType::Timeout, "x"));
        Transaction tx(bad);
        QVERIFY(!tx.isActive());
        QCOMPARE(tx.error().type, ErrType::Timeout);
        QCOMPARE(tx.commit().error().type, ErrType::Timeout);
    }

    void moveTransfersOwnership()
    {
        Transaction a(lease());
        QVERIFY(insertItem(1));
        Transaction b = std::move(a);
        QVERIFY(!a.isActive());   // a không còn rollback trong destructor
        QVERIFY(b.isActive());
        QVERIFY(b.commit().ok());
        QCOMPARE(countRows(), 1);
    }
};

QTEST_MAIN(TestTransaction)
#include "tst_transaction.moc"