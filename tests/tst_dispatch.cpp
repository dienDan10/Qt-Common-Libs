#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>
#include <QThread>
#include <QThreadPool>
#include "httpserver/router.h"
#include "testhelper.h"
#include <atomic>

using Ctx = std::shared_ptr<Context>;

class tst_Dispatch : public QObject
{
    Q_OBJECT

private slots:
    void cleanup() { QThreadPool::globalInstance()->waitForDone(); }

    void defaultRoute_runsHandlerOnWorkerThread();
    void runSync_runsHandlerOnCallingThread();
    void timeout_sends503AndIgnoresLateResponse();
    void fastHandler_isNotOverriddenByTimeout();
    void runSyncRoute_timeoutCannotInterrupt();
};

void tst_Dispatch::defaultRoute_runsHandlerOnWorkerThread()
{
    Router router;
    std::atomic<QThread *> handlerThread{nullptr};
    router.get("/x", [&](Ctx c) {
        handlerThread = QThread::currentThread();
        c->text("ok");
    });

    Fixture f;
    f.ctx->setPath("/x");
    QVERIFY(router.dispatch(f.ctx, Route::NoTimeOut));

    QTRY_COMPARE(f.sentCount(), 1);
    QVERIFY(handlerThread.load() != nullptr);
    QVERIFY(handlerThread.load() != QThread::currentThread());
}

void tst_Dispatch::runSync_runsHandlerOnCallingThread()
{
    Router router;
    QThread *handlerThread = nullptr;
    router
        .get("/x",
             [&](Ctx c) {
                 handlerThread = QThread::currentThread();
                 c->text("ok");
             })
        .runSync();

    Fixture f;
    f.ctx->setPath("/x");
    QVERIFY(router.dispatch(f.ctx, Route::NoTimeOut));

    QCOMPARE(f.sentCount(), 1); // không đợi
    QCOMPARE(handlerThread, QThread::currentThread());
}

void tst_Dispatch::timeout_sends503AndIgnoresLateResponse()
{
    Router router;
    router
        .get("/slow",
             [](Ctx c) {
                 QThread::msleep(300);
                 c->text("late", 200);
             })
        .timeout(50);

    Fixture f;
    f.ctx->setPath("/slow");
    QVERIFY(router.dispatch(f.ctx, Route::NoTimeOut));

    QTRY_COMPARE(f.sentCount(), 1);
    const auto sent = f.snapshot();
    QCOMPARE(sent[0].status, 503);
    QCOMPARE(QJsonDocument::fromJson(sent[0].body).object(),
             (QJsonObject{{"error", "Request Timeout"}}));

    QThreadPool::globalInstance()->waitForDone(); // đợi handler chạy xong
    QCOMPARE(f.sentCount(), 1);                   // response muộn không được gửi
    QCOMPARE(f.logger->warnings.size(), 1);       // mà chỉ bị log lại
}

void tst_Dispatch::fastHandler_isNotOverriddenByTimeout()
{
    Router router;
    router.get("/fast", [](Ctx c) { c->text("ok", 200); }).timeout(200);

    Fixture f;
    f.ctx->setPath("/fast");
    QVERIFY(router.dispatch(f.ctx, Route::NoTimeOut));

    QTRY_COMPARE(f.sentCount(), 1);
    QCOMPARE(f.snapshot()[0].status, 200);

    QTest::qWait(400); // qua hạn timeout, event loop vẫn chạy
    QCOMPARE(f.sentCount(), 1);
}

void tst_Dispatch::runSyncRoute_timeoutCannotInterrupt()
{
    Router router;
    router
        .get("/blocking",
             [](Ctx c) {
                 QThread::msleep(300);
                 c->text("done", 200);
             })
        .runSync()
        .timeout(50);

    Fixture f;
    f.ctx->setPath("/blocking");
    QVERIFY(router.dispatch(f.ctx, Route::NoTimeOut)); // block khoảng 300 ms

    QCOMPARE(f.sentCount(), 1);
    QCOMPARE(f.snapshot()[0].status, 200);

    QTest::qWait(100);
    QCOMPARE(f.sentCount(), 1); // timer không gửi thêm 503
}

QTEST_GUILESS_MAIN(tst_Dispatch)
#include "tst_dispatch.moc"