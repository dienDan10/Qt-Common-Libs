#include <QObject>
#include <QtTest>
#include "httpserver/httpexception.h"
#include "httpserver/router.h"
#include "testhelper.h"
#include <stdexcept>

using Ctx = std::shared_ptr<Context>;

namespace {

class RecordingMiddleware : public IMiddleware
{
public:
    RecordingMiddleware(QString name, QStringList *log)
        : mName(std::move(name))
        , mLog(log)
    {}

    void process(Ctx, NextFunction next) override
    {
        *mLog << mName + ":before";
        next();
        *mLog << mName + ":after";
    }

private:
    QString mName;
    QStringList *mLog;
};

class BlockingMiddleware : public IMiddleware
{
public:
    void process(Ctx ctx, NextFunction) override { ctx->error(401, "no token"); }
};

class ThrowingMiddleware : public IMiddleware
{
public:
    void process(Ctx, NextFunction) override { throw UnauthorizedException("denied"); }
};

} // namespace

class tst_Middleware : public QObject
{
    Q_OBJECT

private slots:
    void chain_runsGlobalThenRouteMiddlewareAroundHandler();
    void middlewareThatDoesNotCallNext_stopsTheChain();
    void middlewareIsSnapshottedAtRegistration();
    void exceptions_areTurnedIntoResponses();
};

void tst_Middleware::chain_runsGlobalThenRouteMiddlewareAroundHandler()
{
    QStringList log;
    Router router;
    router.use(std::make_shared<RecordingMiddleware>("global", &log));
    router
        .get("/x",
             [&](Ctx c) {
                 log << "handler";
                 c->text("ok");
             })
        .use(std::make_shared<RecordingMiddleware>("route", &log))
        .runSync();

    Fixture f;
    f.ctx->setPath("/x");
    QVERIFY(router.dispatch(f.ctx, Route::NoTimeOut));

    QCOMPARE(log,
             (QStringList{"global:before", "route:before", "handler", "route:after", "global:after"}));
}

void tst_Middleware::middlewareThatDoesNotCallNext_stopsTheChain()
{
    QStringList log;
    bool handlerCalled = false;
    Router router;
    router.use(std::make_shared<BlockingMiddleware>());
    router.use(std::make_shared<RecordingMiddleware>("after-blocker", &log));
    router
        .get("/x",
             [&](Ctx c) {
                 handlerCalled = true;
                 c->text("ok");
             })
        .runSync();

    Fixture f;
    f.ctx->setPath("/x");
    QVERIFY(router.dispatch(f.ctx, Route::NoTimeOut));

    QVERIFY(!handlerCalled);
    QVERIFY(log.isEmpty());
    QCOMPARE(f.sent.size(), 1);
    QCOMPARE(f.sent[0].status, 401);
}

void tst_Middleware::middlewareIsSnapshottedAtRegistration()
{
    QStringList log;
    Router router;
    auto handler = [](Ctx c) { c->text("ok"); };

    router.get("/before", handler).runSync(); // chưa có middleware nào
    router.use(std::make_shared<RecordingMiddleware>("mw", &log));
    router.get("/after", handler).runSync(); // có "mw"
    Router g = router.group("/g");           // group thừa kế "mw" lúc này
    router.use(std::make_shared<RecordingMiddleware>("late", &log));
    g.get("/x", handler).runSync(); // không có "late"

    auto run = [&](const QString &path) {
        log.clear();
        Fixture f;
        f.ctx->setPath(path);
        router.dispatch(f.ctx, Route::NoTimeOut);
        return log;
    };

    QCOMPARE(run("/before"), QStringList{});
    QCOMPARE(run("/after"), (QStringList{"mw:before", "mw:after"}));
    QCOMPARE(run("/g/x"), (QStringList{"mw:before", "mw:after"}));
}

void tst_Middleware::exceptions_areTurnedIntoResponses()
{
    {
        bool handlerCalled = false;
        Router router;
        router.use(std::make_shared<ThrowingMiddleware>());
        router
            .get("/x",
                 [&](Ctx c) {
                     handlerCalled = true;
                     c->text("ok");
                 })
            .runSync();

        Fixture f;
        f.ctx->setPath("/x");
        QVERIFY(router.dispatch(f.ctx, Route::NoTimeOut));

        QVERIFY(!handlerCalled);
        QCOMPARE(f.sent.size(), 1);
        QCOMPARE(f.sent[0].status, 401);
    }
    {
        Router router;
        router.get("/x", [](Ctx) { throw std::runtime_error("boom"); }).runSync();

        Fixture f;
        f.ctx->setPath("/x");
        QVERIFY(router.dispatch(f.ctx, Route::NoTimeOut));

        QCOMPARE(f.sent.size(), 1);
        QCOMPARE(f.sent[0].status, 500);
    }
}

QTEST_GUILESS_MAIN(tst_Middleware)
#include "tst_middleware.moc"