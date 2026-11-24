#include <QtTest>
#include "dbaccess/query/sqlresult.h"

using namespace QtCommon::dbaccess;
using T = SqlError::Type;

class TestSqlResult : public QObject
{
    Q_OBJECT
private slots:
    void okHoldsValue()
    {
        SqlResult<int> r = 42;
        QVERIFY(r.ok());
        QVERIFY(bool(r));
        QCOMPARE(r.value(), 42);
        QVERIFY(!r.error().hasError());
    }

    void errorHasNoValue()
    {
        SqlResult<int> r = SqlError::make(T::Statement, "syntax error");
        QVERIFY(!r.ok());
        QCOMPARE(r.error().type, T::Statement);
        QCOMPARE(r.valueOr(7), 7);
    }

    void emptyErrorBecomesUnknown()
    {
        SqlResult<int> r = SqlError{};   // type None
        QVERIFY(!r.ok());                // không được thành trạng thái "không giá trị, không lỗi"
        QCOMPARE(r.error().type, T::Unknown);
    }

    void errorForwardsAcrossTypes()
    {
        SqlResult<int> a = SqlError::make(T::Connection, "lost");
        SqlResult<QString> b = a.error();   // chuyển tiếp lỗi sang kiểu khác
        QVERIFY(!b.ok());
        QCOMPARE(b.error().type, T::Connection);
        QCOMPARE(b.error().message, QString("lost"));
    }

    void rvalueValueMovesOut()
    {
        QList<int> list = SqlResult<QList<int>>(QList<int>{1, 2, 3}).value();   // gọi bản value() &&
        QCOMPARE(list, (QList<int>{1, 2, 3}));
    }

    void optionalPayload()   // đúng kiểu mà first() sẽ trả
    {
        SqlResult<std::optional<int>> found = std::optional<int>(5);
        SqlResult<std::optional<int>> notFound = std::optional<int>();
        QVERIFY(found.ok() && found.value().has_value());
        QVERIFY(notFound.ok() && !notFound.value().has_value());   // chạy OK nhưng không có dòng
    }

    void boolPayloadIsNotSuccessFlag()
    {
        SqlResult<bool> r = false;
        QVERIFY(r.ok());          // if (r) nghĩa là CHẠY THÀNH CÔNG...
        QCOMPARE(r.value(), false);   // ...còn giá trị phải đọc qua value()
    }

    void voidResult()
    {
        SqlResult<void> ok;
        QVERIFY(ok.ok());

        SqlResult<void> fail = SqlError::make(T::Transaction, "commit failed");
        QVERIFY(!fail.ok());
        QCOMPARE(fail.error().type, T::Transaction);
    }
};

QTEST_MAIN(TestSqlResult)
#include "tst_sqlresult.moc"