#include <QtTest>
#include <optional>
#include "dbaccess/connection/threadlocalprovider.h"
#include "dbaccess/query/sqlquery.h"

using namespace QtCommon::dbaccess;
using namespace std::chrono_literals;
using ErrType = SqlError::Type;

namespace {
struct User
{
    int id = 0;
    QString name;
    int age = 0;
    std::optional<QString> note;
};

User mapUser(const SqlRow &r)
{
    return User{r.get<int>("id"), r.get<QString>("name"), r.get<int>("age"), r.opt<QString>("note")};
}
} // namespace

class TestSqlQuery : public QObject
{
    Q_OBJECT

    std::shared_ptr<ThreadLocalProvider> mProvider;
    std::optional<ConnectionLease> mLease;

    const ConnectionLease &lease() const { return *mLease; }

    void insertUser(int id, const QString &name, int age, const QVariant &note = {})
    {
        auto r = SqlQuery(lease(), "INSERT INTO users (id, name, age, note) VALUES (:id, :name, :age, :note)")
                     .bind("id", id)
                     .bind("name", name)
                     .bind("age", age)
                     .bind("note", note)   // QVariant rỗng -> NULL
                     .exec();
        QVERIFY2(r.ok(), qPrintable(r.error().toString()));
    }

private slots:
    // ------------------------------------------------------------------
    // Chạy trước / sau MỖI test: mỗi test một DB in-memory mới
    // ------------------------------------------------------------------
    void init()
    {
        DbConfig cfg;
        cfg.name = "mem";
        cfg.driver = "QSQLITE";
        cfg.database = ":memory:";
        mProvider = std::make_shared<ThreadLocalProvider>(cfg);
        mLease.emplace(mProvider->acquire(0ms));
        QVERIFY2(mLease->isValid(), qPrintable(mLease->error().toString()));

        auto r = SqlQuery(lease(), "CREATE TABLE users ("
                                   " id INTEGER PRIMARY KEY,"
                                   " name TEXT NOT NULL,"
                                   " age INTEGER,"
                                   " note TEXT)")
                     .exec();
        QVERIFY2(r.ok(), qPrintable(r.error().toString()));
    }

    void cleanup()
    {
        mLease.reset();      // trả lease trước
        mProvider.reset();   // hủy provider -> dọn connection của thread này -> DB in-memory mất
    }

    // ------------------------------------------------------------------
    // Phần 1: bind + exec
    // ------------------------------------------------------------------
    void insertWithBind()
    {
        auto r = SqlQuery(lease(), "INSERT INTO users (id, name, age) VALUES (:id, :name, :age)")
                     .bind("id", 1)          // không có dấu :
                     .bind(":name", "An")    // có dấu :
                     .bind("age", 20)
                     .exec();
        QVERIFY2(r.ok(), qPrintable(r.error().toString()));
        QCOMPARE(r.value().rowsAffected, 1);
        QCOMPARE(r.value().lastInsertId.toInt(), 1);
    }

    void bindMap()
    {
        auto r = SqlQuery(lease(), "INSERT INTO users (id, name, age) VALUES (:id, :name, :age)")
                     .bind(QVariantMap{{"id", 2}, {"name", "Binh"}, {"age", 30}})
                     .exec();
        QVERIFY2(r.ok(), qPrintable(r.error().toString()));
        QCOMPARE(SqlQuery(lease(), "SELECT name FROM users WHERE id = 2").scalar<QString>().value(),
                 QString("Binh"));
    }

    void updateCountsRows()
    {
        insertUser(1, "An", 20);
        insertUser(2, "Binh", 30);
        auto r = SqlQuery(lease(), "UPDATE users SET age = age + 1").exec();
        QVERIFY(r.ok());
        QCOMPARE(r.value().rowsAffected, 2);
    }

    void syntaxErrorReportsSql()
    {
        auto r = SqlQuery(lease(), "SELEC 1").exec();
        QVERIFY(!r.ok());
        QCOMPARE(r.error().type, ErrType::Statement);
        QCOMPARE(r.error().sql, QString("SELEC 1"));   // lỗi luôn kèm câu SQL gây ra nó
    }

    void missingBindFails()
    {
        // Thiếu :name. Tùy driver: báo thiếu tham số, hoặc bind NULL rồi vi phạm NOT NULL.
        auto r = SqlQuery(lease(), "INSERT INTO users (id, name) VALUES (:id, :name)")
                     .bind("id", 1)
                     .exec();
        QVERIFY(!r.ok());
    }

    void constraintViolation()
    {
        insertUser(1, "An", 20);
        auto r = SqlQuery(lease(), "INSERT INTO users (id, name) VALUES (1, 'Trung')").exec();
        QVERIFY(!r.ok());   // trùng khóa chính
        QVERIFY(!r.error().message.isEmpty());
    }

    void failedLeaseForwardsError()
    {
        // Lỗi tầng 1 đi thẳng lên, không đụng tới Qt, ở MỌI hàm kết thúc
        const ConnectionLease bad =
            ConnectionLease::failed(SqlError::make(ErrType::Timeout, "pool exhausted"));
        QCOMPARE(SqlQuery(bad, "SELECT 1").exec().error().type, ErrType::Timeout);
        QCOMPARE(SqlQuery(bad, "SELECT 1").scalar<int>().error().type, ErrType::Timeout);
        QCOMPARE(SqlQuery(bad, "SELECT 1").first(mapUser).error().type, ErrType::Timeout);
        QCOMPARE(SqlQuery(bad, "SELECT 1").list(mapUser).error().type, ErrType::Timeout);
    }

    // ------------------------------------------------------------------
    // Phần 2: scalar
    // ------------------------------------------------------------------
    void scalarCount()
    {
        insertUser(1, "An", 20);
        insertUser(2, "Binh", 30);
        auto r = SqlQuery(lease(), "SELECT COUNT(*) FROM users").scalar<int>();
        QVERIFY(r.ok());
        QCOMPARE(r.value(), 2);
    }

    void scalarWithBind()
    {
        insertUser(1, "An", 20);
        auto r = SqlQuery(lease(), "SELECT name FROM users WHERE id = :id").bind("id", 1).scalar<QString>();
        QVERIFY(r.ok());
        QCOMPARE(r.value(), QString("An"));
    }

    void scalarNoRowIsError()
    {
        auto r = SqlQuery(lease(), "SELECT id FROM users WHERE id = 999").scalar<int>();
        QVERIFY(!r.ok());
        QCOMPARE(r.error().type, ErrType::NoRow);   // phân biệt được với lỗi SQL
        QVERIFY(!r.error().sql.isEmpty());
    }

    void scalarNullGivesDefault()
    {
        // Bảng rỗng: MAX() vẫn trả 1 dòng, giá trị NULL -> T{}
        auto r = SqlQuery(lease(), "SELECT MAX(age) FROM users").scalar<int>();
        QVERIFY(r.ok());
        QCOMPARE(r.value(), 0);
    }

    // ------------------------------------------------------------------
    // Phần 2: first
    // ------------------------------------------------------------------
    void firstFound()
    {
        insertUser(1, "An", 20, "ghi chu");
        auto r = SqlQuery(lease(), "SELECT * FROM users WHERE id = :id").bind("id", 1).first(mapUser);
        QVERIFY2(r.ok(), qPrintable(r.error().toString()));
        QVERIFY(r.value().has_value());

        const User &u = *r.value();
        QCOMPARE(u.id, 1);
        QCOMPARE(u.name, QString("An"));
        QCOMPARE(u.age, 20);
        QCOMPARE(u.note, std::optional<QString>("ghi chu"));
    }

    void firstNotFoundIsOkEmpty()
    {
        auto r = SqlQuery(lease(), "SELECT * FROM users WHERE id = 999").first(mapUser);
        QVERIFY(r.ok());                   // KHÔNG phải lỗi
        QVERIFY(!r.value().has_value());   // chỉ là không có dòng
    }

    // ------------------------------------------------------------------
    // Phần 2: list
    // ------------------------------------------------------------------
    void listMapsAllRowsInOrder()
    {
        insertUser(3, "Chi", 25);
        insertUser(1, "An", 20);
        insertUser(2, "Binh", 30);
        auto r = SqlQuery(lease(), "SELECT * FROM users ORDER BY id").list(mapUser);
        QVERIFY2(r.ok(), qPrintable(r.error().toString()));

        const QList<User> &users = r.value();
        QCOMPARE(users.size(), 3);
        QCOMPARE(users[0].name, QString("An"));
        QCOMPARE(users[1].name, QString("Binh"));
        QCOMPARE(users[2].name, QString("Chi"));
    }

    void listEmptyIsOk()
    {
        auto r = SqlQuery(lease(), "SELECT * FROM users").list(mapUser);
        QVERIFY(r.ok());
        QVERIFY(r.value().isEmpty());
    }

    void mapperHandlesNull()
    {
        insertUser(1, "An", 20);                  // note = NULL
        insertUser(2, "Binh", 30, "co note");
        auto r = SqlQuery(lease(), "SELECT * FROM users ORDER BY id").list(mapUser);
        QVERIFY(r.ok());
        QVERIFY(!r.value()[0].note.has_value());
        QCOMPARE(r.value()[1].note, std::optional<QString>("co note"));
    }

    void lambdaMapper()
    {
        insertUser(1, "An", 20);
        insertUser(2, "Binh", 30);
        auto r = SqlQuery(lease(), "SELECT name FROM users ORDER BY id")
                     .list([](const SqlRow &row) { return row.get<QString>(0); });   // R tự suy ra: QString
        QVERIFY(r.ok());
        QCOMPARE(r.value(), (QList<QString>{"An", "Binh"}));
    }

    void queryObjectIsReusable()
    {
        insertUser(1, "An", 20);
        const SqlQuery count(lease(), "SELECT COUNT(*) FROM users");
        QCOMPARE(count.scalar<int>().value(), 1);

        insertUser(2, "Binh", 30);
        QCOMPARE(count.scalar<int>().value(), 2);   // hàm kết thúc là const: chạy lại được nhiều lần
    }
};

QTEST_MAIN(TestSqlQuery)
#include "tst_sqlquery.moc"