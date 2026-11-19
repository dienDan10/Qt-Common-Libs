#include <QObject>
#include <QtTest>
#include "httpserver/errorhandler.h"
#include "testhelper.h"

class tst_ErrorHandler : public QObject
{
    Q_OBJECT

private slots:
    void httpException_sendsItsStatusAndMessage();
    void otherException_logsCallsCallbackAndSends500();
    void callbackThatResponds_preventsDefault500();
    void alreadyResponded_isNotOverwritten();
};

void tst_ErrorHandler::httpException_sendsItsStatusAndMessage()
{
    Fixture f;
    int callbackCalls = 0;
    f.ctx->setErrorCallback(
        [&](std::shared_ptr<Context>, const std::exception &) { ++callbackCalls; });

    ErrorHandler::handle(f.ctx, NotFoundException("missing"));

    QCOMPARE(f.sent.size(), 1);
    QCOMPARE(f.sent[0].status, 404);
    QCOMPARE(QJsonDocument::fromJson(f.sent[0].body).object(), (QJsonObject{{"error", "missing"}}));
    QCOMPARE(callbackCalls, 0);
    QVERIFY(f.logger->errors.isEmpty());
}

void tst_ErrorHandler::otherException_logsCallsCallbackAndSends500()
{
    Fixture f;
    int callbackCalls = 0;
    f.ctx->setErrorCallback(
        [&](std::shared_ptr<Context>, const std::exception &) { ++callbackCalls; });

    ErrorHandler::handle(f.ctx, std::runtime_error("boom"));

    QCOMPARE(f.logger->errors.size(), 1);
    QVERIFY(f.logger->errors[0].contains("GET /test"));
    QVERIFY(f.logger->errors[0].contains("boom"));
    QCOMPARE(callbackCalls, 1);
    QCOMPARE(f.sent.size(), 1);
    QCOMPARE(f.sent[0].status, 500);
    QCOMPARE(QJsonDocument::fromJson(f.sent[0].body).object(),
             (QJsonObject{{"error", "Internal Server Error"}}));
}

void tst_ErrorHandler::callbackThatResponds_preventsDefault500()
{
    Fixture f;
    f.ctx->setErrorCallback(
        [](std::shared_ptr<Context> c, const std::exception &) { c->text("custom", 502); });

    ErrorHandler::handle(f.ctx, std::runtime_error("boom"));

    QCOMPARE(f.sent.size(), 1);
    QCOMPARE(f.sent[0].status, 502);
    QCOMPARE(f.sent[0].body, QByteArray("custom"));
}

void tst_ErrorHandler::alreadyResponded_isNotOverwritten()
{
    {
        Fixture f;
        f.ctx->text("done", 200);

        ErrorHandler::handle(f.ctx, BadRequestException("late"));

        QCOMPARE(f.sent.size(), 1);
        QCOMPARE(f.sent[0].status, 200);
    }
    {
        Fixture f;
        f.ctx->text("done", 200);

        ErrorHandler::handle(f.ctx, std::runtime_error("late"));

        QCOMPARE(f.sent.size(), 1);
        QCOMPARE(f.sent[0].status, 200);
    }
}

QTEST_APPLESS_MAIN(tst_ErrorHandler)
#include "tst_errorhandler.moc"