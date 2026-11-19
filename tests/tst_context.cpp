/******************************************************************************
 * @file        tst_context.cpp
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-19
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>
#include "testhelper.h"

class tst_Context : public QObject
{
    Q_OBJECT

private slots:
    void json_sendsStatusBodyAndContentType();
    void secondResponse_isIgnoredAndWarned();
    void beforeResponseHooks_runInOrderBeforeSender();
    void error_sendsJsonErrorBody();
};

void tst_Context::json_sendsStatusBodyAndContentType()
{
    Fixture f;

    f.ctx->json(QJsonObject{{"a", 1}}, 201);

    QVERIFY(f.ctx->hasResponded());
    QCOMPARE(f.sent.size(), 1);
    QCOMPARE(f.sent[0].status, 201);
    QCOMPARE(f.sent[0].body, QByteArray(R"({"a":1})"));
    QCOMPARE(f.sent[0].headers.value("Content-Type").toByteArray(), QByteArray("application/json"));
}

void tst_Context::secondResponse_isIgnoredAndWarned()
{
    Fixture f;

    f.ctx->text("first", 200);
    f.ctx->text("second", 500);

    QCOMPARE(f.sent.size(), 1);
    QCOMPARE(f.sent[0].status, 200);
    QCOMPARE(f.sent[0].body, QByteArray("first"));
    QCOMPARE(f.ctx->statusCode(), 200);     // trạng thái của ctx cũng không bị ghi đè
    QCOMPARE(f.logger->warnings.size(), 1); // lần gọi thứ hai bị log lại
}

void tst_Context::beforeResponseHooks_runInOrderBeforeSender()
{
    Fixture f;
    QStringList order;

    f.ctx->onBeforeResponse(
        [&](Context &c) { order << QString("hook1(status=%1)").arg(c.statusCode()); });
    f.ctx->onBeforeResponse([&](Context &) { order << "hook2"; });
    f.ctx->setResponseSender(
        [&](int, const QByteArray &, const QHttpHeaders &) { order << "sender"; });

    f.ctx->text("ok", 202);

    QCOMPARE(order, (QStringList{"hook1(status=202)", "hook2", "sender"}));
}

void tst_Context::error_sendsJsonErrorBody()
{
    Fixture f;

    f.ctx->error(404, "nope");

    QCOMPARE(f.sent.size(), 1);
    QCOMPARE(f.sent[0].status, 404);
    QCOMPARE(QJsonDocument::fromJson(f.sent[0].body).object(), (QJsonObject{{"error", "nope"}}));
    QCOMPARE(f.sent[0].headers.value("Content-Type").toByteArray(), QByteArray("application/json"));
}

QTEST_APPLESS_MAIN(tst_Context)
#include "tst_context.moc"