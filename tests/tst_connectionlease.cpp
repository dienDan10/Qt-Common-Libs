#include <QSqlDatabase>
#include <QtTest>
#include "dbaccess/dbregistry.h"

using namespace QtCommon::dbaccess;

namespace {
DbConfig sqliteConfig(const QString &name)
{
    DbConfig cfg;
    cfg.name = name;
    cfg.driver = "QSQLITE";
    cfg.database = ":memory:";
    return cfg;
}

class FakeProvider : public IConnectionProvider
{
public:
    explicit FakeProvider(DbConfig cfg) : mCfg(std::move(cfg)) {}
    const DbConfig &config() const override { return mCfg; }
    ConnectionLease acquire(std::chrono::milliseconds) override
    {
        ++acquired;
        return makeLease(QStringLiteral("fake_%1").arg(acquired));
    }
    void closeAll() override { ++closed; }

    int acquired = 0;
    int closed = 0;
    QStringList released;

private:
    void release(const QString &name) noexcept override { released << name; }
    DbConfig mCfg;
};

constexpr std::chrono::milliseconds kNoWait{0};
} // namespace

class TestConnectionLease : public QObject
{
    Q_OBJECT
private slots:
    void releaseOnScopeExit()
    {
        auto p = std::make_shared<FakeProvider>(sqliteConfig("a"));
        {
            ConnectionLease lease = p->acquire(kNoWait);
            QVERIFY(lease.isValid());
            QVERIFY(p->released.isEmpty());
        }
        QCOMPARE(p->released, QStringList{"fake_1"});
    }

    void moveTransfersOwnership()
    {
        auto p = std::make_shared<FakeProvider>(sqliteConfig("a"));
        {
            ConnectionLease a = p->acquire(kNoWait);
            ConnectionLease b = std::move(a);
            QVERIFY(!a.isValid());   // moved-from: không giữ gì, không trả gì
            QVERIFY(b.isValid());
        }
        QCOMPARE(p->released.size(), 1);   // trả đúng 1 lần
    }

    void moveAssignReleasesOldFirst()
    {
        auto p = std::make_shared<FakeProvider>(sqliteConfig("a"));
        ConnectionLease a = p->acquire(kNoWait);   // fake_1
        ConnectionLease b = p->acquire(kNoWait);   // fake_2
        b = std::move(a);
        QCOMPARE(p->released, QStringList{"fake_2"});   // cái cũ của b được trả ngay
        QCOMPARE(b.connectionName(), QString("fake_1"));
    }

    void failedLeaseNeverReleases()
    {
        ConnectionLease lease = ConnectionLease::failed(
            SqlError::make(SqlError::Type::Timeout, "pool exhausted"));
        QVERIFY(!lease.isValid());
        QCOMPARE(lease.error().type, SqlError::Type::Timeout);
        QVERIFY(!lease.database().isValid());
    }

    void registryRejectsInvalidAndDuplicate()
    {
        DbRegistry reg;   // instance riêng, không đụng singleton
        DbConfig bad;
        bad.name = "bad";
        QCOMPARE(reg.add(std::make_shared<FakeProvider>(bad)).type, SqlError::Type::InvalidConfig);

        QVERIFY(!reg.add(std::make_shared<FakeProvider>(sqliteConfig("main"))).hasError());
        QCOMPARE(reg.add(std::make_shared<FakeProvider>(sqliteConfig("main"))).type,
                 SqlError::Type::InvalidConfig);
        QVERIFY(reg.provider("main"));
        QVERIFY(!reg.provider("nope"));
    }

    void leaseOutlivesRegistryRemoval()
    {
        DbRegistry reg;
        auto p = std::make_shared<FakeProvider>(sqliteConfig("main"));
        std::weak_ptr<FakeProvider> watch = p;
        reg.add(std::move(p));
        {
            ConnectionLease lease = reg.provider("main")->acquire(kNoWait);
            QVERIFY(reg.remove("main"));
            QVERIFY(!watch.expired());   // lease vẫn giữ provider sống
        }
        QVERIFY(watch.expired());        // lease trả xong -> provider mới bị hủy
    }
};

QTEST_MAIN(TestConnectionLease)
#include "tst_connectionlease.moc"