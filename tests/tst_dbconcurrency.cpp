#include <QtTest>
#include <QMutex>
#include <QSemaphore>
#include <QSet>
#include <QSqlDatabase>
#include <QTemporaryDir>
#include <QThreadPool>
#include <atomic>
#include <utility>
#include "dbaccess/dbaccess.h"

using namespace QtCommon::dbaccess;

namespace {
constexpr int kThreads = 4;
constexpr int kTasks = 40;

// Gom kết quả từ các worker. QVERIFY/QCOMPARE chỉ dùng được trên thread chạy test,
// nên worker chỉ GHI LẠI, kiểm tra để sau khi pool chạy xong.
struct Collector
{
    QMutex mutex;
    QHash<QThread *, QSet<QString>> namesByThread;   // thread -> các connection name nó đã dùng
    QStringList errors;

    void record(QThread *thread, const QString &name)
    {
        QMutexLocker lock(&mutex);
        namesByThread[thread].insert(name);
    }

    void fail(const QString &message)
    {
        QMutexLocker lock(&mutex);
        errors << message;
    }

    QSet<QString> allNames()
    {
        QMutexLocker lock(&mutex);
        QSet<QString> all;
        for (const auto &names : std::as_const(namesByThread))
            all.unite(names);
        return all;
    }
};

QString threadTag()
{
    return QString::number(quintptr(QThread::currentThread()), 16);
}
} // namespace

class TestDbConcurrency : public QObject
{
    Q_OBJECT

    QTemporaryDir mDir;
    std::unique_ptr<DbRegistry> mRegistry;

    Database open() const { return Database::acquire(*mRegistry, "shared"); }

private slots:
    void init()
    {
        QVERIFY(mDir.isValid());

        DbConfig cfg;
        cfg.name = "shared";
        cfg.driver = "QSQLITE";
        // File thật (không phải :memory:) -> mọi connection của mọi thread cùng thấy 1 DB.
        cfg.database = mDir.filePath(QLatin1String(QTest::currentTestFunction()) + ".db");
        // Nhiều thread cùng ghi: chờ khóa tối đa 5s thay vì báo lỗi "database is locked" ngay.
        cfg.connectOptions = "QSQLITE_BUSY_TIMEOUT=5000";

        mRegistry = std::make_unique<DbRegistry>();
        QVERIFY(!mRegistry->add(std::make_shared<ThreadLocalProvider>(cfg)).hasError());

        auto db = open();
        QVERIFY2(db.isValid(), qPrintable(db.error().toString()));
        QVERIFY(db.query("PRAGMA journal_mode=WAL").exec().ok());   // cho phép đọc song song với ghi
        QVERIFY(db.query("CREATE TABLE items (id INTEGER PRIMARY KEY, thread TEXT NOT NULL)").exec().ok());
    }

    void cleanup()
    {
        mRegistry.reset();
        QVERIFY2(QSqlDatabase::connectionNames().isEmpty(),
                 qPrintable("leaked: " + QSqlDatabase::connectionNames().join(", ")));
    }

    // Mỗi thread của pool có đúng 1 connection riêng, dùng lại qua mọi task; không thread nào dùng chung.
    void eachPoolThreadHasOwnConnection()
    {
        Collector c;
        {
            QThreadPool pool;
            pool.setMaxThreadCount(kThreads);
            for (int i = 0; i < kTasks; ++i) {
                pool.start([this, &c] {
                    auto db = open();
                    if (!db.isValid()) {
                        c.fail(db.error().toString());
                        return;
                    }
                    auto r = db.query("SELECT 1").scalar<int>();   // dùng thật trên thread này
                    if (!r.ok()) {
                        c.fail(r.error().toString());
                        return;
                    }
                    c.record(QThread::currentThread(), db.lease().connectionName());
                    QThread::msleep(2);   // để task rải đều ra nhiều thread
                });
            }
            pool.waitForDone();
        }

        QVERIFY2(c.errors.isEmpty(), qPrintable(c.errors.join('\n')));
        QVERIFY(c.namesByThread.size() > 1);          // thật sự chạy trên nhiều thread
        QVERIFY(c.namesByThread.size() <= kThreads);  // số connection <= số thread

        QSet<QString> seen;
        for (auto it = c.namesByThread.cbegin(); it != c.namesByThread.cend(); ++it) {
            QCOMPARE(it.value().size(), 1);           // 1 thread -> đúng 1 connection
            const QString name = *it.value().cbegin();
            QVERIFY2(!seen.contains(name), qPrintable(name));   // 2 thread không bao giờ chung connection
            seen.insert(name);
        }
    }

    // Ghi đồng thời từ nhiều thread, mỗi task 1 transaction: không mất dòng, không lỗi.
    void concurrentWritesAllLand()
    {
        Collector c;
        {
            QThreadPool pool;
            pool.setMaxThreadCount(kThreads);
            for (int i = 0; i < kTasks; ++i) {
                pool.start([this, &c, i] {
                    auto db = open();
                    auto tx = db.transaction();
                    if (!tx.isActive()) {
                        c.fail(tx.error().toString());
                        return;
                    }
                    auto r = db.query("INSERT INTO items (id, thread) VALUES (:id, :thread)")
                                 .bind("id", i)
                                 .bind("thread", threadTag())
                                 .exec();
                    if (!r.ok()) {
                        c.fail(r.error().toString());
                        return;   // tx tự rollback
                    }
                    QThread::msleep(2);
                    if (auto cr = tx.commit(); !cr.ok())
                        c.fail(cr.error().toString());
                });
            }
            pool.waitForDone();
        }

        QVERIFY2(c.errors.isEmpty(), qPrintable(c.errors.join('\n')));

        auto db = open();
        QCOMPARE(db.query("SELECT COUNT(*) FROM items").scalar<int>().value(), kTasks);
        QCOMPARE(db.query("SELECT COUNT(DISTINCT id) FROM items").scalar<int>().value(), kTasks);
        QVERIFY(db.query("SELECT COUNT(DISTINCT thread) FROM items").scalar<int>().value() > 1);
    }

    // Thread của pool kết thúc -> mỗi thread phát finished() -> connection của nó tự được dọn.
    // Lưu ý: QThreadPool::waitForDone() KHÔNG chỉ chờ task xong, nó còn cho mọi thread kết thúc.
    void connectionsCleanedWhenPoolThreadsExit()
    {
        Collector c;
        {
            QThreadPool pool;
            pool.setMaxThreadCount(kThreads);
            for (int i = 0; i < kTasks; ++i) {
                pool.start([this, &c] {
                    auto db = open();
                    c.record(QThread::currentThread(), db.lease().connectionName());
                    if (!QSqlDatabase::contains(db.lease().connectionName()))
                        c.fail("connection missing while thread alive");
                    QThread::msleep(2);
                });
            }
            pool.waitForDone();   // các thread kết thúc ở đây
        }

        QVERIFY2(c.errors.isEmpty(), qPrintable(c.errors.join('\n')));
        const QSet<QString> names = c.allNames();
        QVERIFY(!names.isEmpty());
        for (const QString &name : names)
            QVERIFY2(!QSqlDatabase::contains(name), qPrintable("not cleaned: " + name));
    }

    // closeAll() gọi từ thread test: mọi worker thấy cờ closed và tự dọn connection của mình.
    // Dùng semaphore thay cho waitForDone() để các thread CÒN SỐNG giữa 2 đợt task.
    void closeAllStopsEveryThread()
    {
        QThreadPool pool;
        pool.setMaxThreadCount(kThreads);
        pool.setExpiryTimeout(-1);

        Collector round1;
        QSemaphore done1;
        for (int i = 0; i < kTasks; ++i) {
            pool.start([this, &round1, &done1] {
                auto db = open();
                round1.record(QThread::currentThread(), db.lease().connectionName());
                QThread::msleep(2);
                done1.release();
            });
        }
        done1.acquire(kTasks);

        mRegistry->provider("shared")->closeAll();   // từ thread test: chỉ dọn được phần của thread test

        Collector round2;
        QSemaphore done2;
        std::atomic<int> rejected{0};
        for (int i = 0; i < kTasks; ++i) {
            pool.start([this, &round2, &done2, &rejected] {
                if (!open().isValid())
                    ++rejected;
                round2.record(QThread::currentThread(), {});
                QThread::msleep(2);
                done2.release();
            });
        }
        done2.acquire(kTasks);

        QCOMPARE(rejected.load(), kTasks);   // không thread nào còn mượn được

        // Thread nào chạy ở cả 2 đợt thì connection cũ đã được dọn ngay trong acquire() đợt 2
        int checked = 0;
        for (QThread *thread : round2.namesByThread.keys()) {
            for (const QString &name : round1.namesByThread.value(thread)) {
                QVERIFY2(!QSqlDatabase::contains(name), qPrintable("not cleaned: " + name));
                ++checked;
            }
        }
        QVERIFY(checked > 0);   // có thread dùng lại thật, test mới có nghĩa

        pool.waitForDone();
    }
};

QTEST_MAIN(TestDbConcurrency)
#include "tst_dbconcurrency.moc"