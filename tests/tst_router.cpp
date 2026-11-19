#include <QObject>
#include <QtTest>
#include "httpserver/router.h"
#include "testhelper.h"

using Ctx = std::shared_ptr<Context>;

class tst_Router : public QObject
{
    Q_OBJECT

private slots:
    void matchesPathAndExtractsParams();
    void unmatchedRequest_returnsFalseAndDoesNothing();
    void group_addsPrefixAndSharesRoutes();
    void duplicateRoute_throws();
};

void tst_Router::matchesPathAndExtractsParams()
{
    Router router;
    QString userId, postId;
    router
        .get("/users/:id/posts/:postId",
             [&](Ctx c) {
                 userId = c->pathParam("id");
                 postId = c->pathParam("postId");
                 c->text("ok");
             })
        .runSync();

    Fixture f;
    f.ctx->setPath("/users/42/posts/7");

    QVERIFY(router.dispatch(f.ctx, Route::NoTimeOut));
    QCOMPARE(userId, QStringLiteral("42"));
    QCOMPARE(postId, QStringLiteral("7"));
    QCOMPARE(f.sent.size(), 1);
}

void tst_Router::unmatchedRequest_returnsFalseAndDoesNothing()
{
    Router router;
    bool called = false;
    router
        .get("/users/:id",
             [&](Ctx c) {
                 called = true;
                 c->text("ok");
             })
        .runSync();

    const QList<QPair<QString, QString>> requests = {
        {"POST", "/users/1"},      // sai method
        {"GET", "/users"},         // thiếu segment
        {"GET", "/users/1/extra"}, // thừa segment
        {"GET", "/other/1"},       // sai segment tĩnh
    };

    for (const auto &[method, path] : requests) {
        Fixture f;
        f.ctx->setMethod(method);
        f.ctx->setPath(path);

        QVERIFY2(!router.dispatch(f.ctx, Route::NoTimeOut), qPrintable(method + " " + path));
        QVERIFY(f.sent.isEmpty());
    }
    QVERIFY(!called);
}

void tst_Router::group_addsPrefixAndSharesRoutes()
{
    Router router;
    Router api = router.group("/api");
    Router v1 = api.group("/v1");
    v1.get("/ping", [](Ctx c) { c->text("pong"); }).runSync();

    {
        Fixture f;
        f.ctx->setPath("/api/v1/ping");
        QVERIFY(router.dispatch(f.ctx, Route::NoTimeOut));
        QCOMPARE(f.sent.size(), 1);
        QCOMPARE(f.sent[0].body, QByteArray("pong"));
    }
    {
        Fixture f;
        f.ctx->setPath("/ping"); // không có prefix thì không khớp
        QVERIFY(!router.dispatch(f.ctx, Route::NoTimeOut));
    }
}

void tst_Router::duplicateRoute_throws()
{
    Router router;
    auto handler = [](Ctx) {};

    router.get("/a", handler);
    QVERIFY_THROWS_EXCEPTION(std::logic_error, router.get("/a", handler));

    router.post("/a", handler); // khác method nên hợp lệ

    Router x = router.group("/x");
    x.get("/b", handler);
    QVERIFY_THROWS_EXCEPTION(std::logic_error, router.get("/x/b", handler));
}

QTEST_GUILESS_MAIN(tst_Router)
#include "tst_router.moc"