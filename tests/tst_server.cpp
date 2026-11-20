#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
#include <QThread>
#include <QThreadPool>
#include <QtTest>
#include "httpclient/httpclient.h"
#include "httpserver/httpexception.h"
#include "httpserver/httpserver.h"
#include <tuple>

using namespace std::chrono_literals;
using Ctx = std::shared_ptr<Context>;

namespace {

class NullLogger : public ILogger
{
public:
    void debug(const QString &) override {}
    void info(const QString &) override {}
    void warning(const QString &) override {}
    void error(const QString &) override {}
};

// Không dùng future.waitForFinished(): nó chặn thread nên reply không bao giờ xong.
// Hết giờ thì trả response rỗng (statusCode 0) để assert của test báo lỗi rõ ràng.
HttpResponse await(QFuture<HttpResponse> future, int timeoutMs = 5000)
{
    if (!QTest::qWaitFor([&]() { return future.isFinished(); }, timeoutMs)) {
        HttpResponse timedOut;
        timedOut.error = QNetworkReply::TimeoutError;
        return timedOut;
    }
    return future.result();
}

} // namespace

class tst_Server : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup() { QThreadPool::globalInstance()->waitForDone(); }
    void cleanupTestCase();

    void bothExecutionModes_respond();
    void pathAndQueryParams_arriveInHandler();
    void postBody_reachesHandler();
    void errors_mapToStatusCodes();
    void slowHandler_gets503WithoutWaiting();
    void concurrentRequests_doNotMixResponses();

private:
    std::unique_ptr<HttpServer> mServer;
    std::unique_ptr<HttpClient> mHttp;
};

void tst_Server::initTestCase()
{
    Router router;
    router.get("/hello", [](Ctx c) { c->json(QJsonObject{{"msg", "hi"}}); });
    router.get("/sync", [](Ctx c) { c->json(QJsonObject{{"msg", "sync"}}); }).runSync();

    router.get("/users/:id", [](Ctx c) {
        c->json(QJsonObject{{"id", c->pathParam("id")}, {"sort", c->queryParam("sort")}});
    });

    router.post("/echo", [](Ctx c) {
        bool ok = false;
        const QJsonObject body = c->requestBodyJson(&ok);
        if (!ok)
            throw BadRequestException("invalid json");
        c->json(body, 201);
    });

    router.get("/boom", [](Ctx) { throw std::runtime_error("boom"); });
    router.get("/forbidden", [](Ctx) { throw ForbiddenException("nope"); });

    router.get("/throw-int", [](Ctx) { throw 42; });
    router.get("/throw-in-callback", [](Ctx) { throw std::runtime_error("trigger-callback"); });

    router
        .get("/slow",
             [](Ctx c) {
                 QThread::msleep(500);
                 c->text("late");
             })
        .timeout(100);

    router.get("/work/:id", [](Ctx c) {
        QThread::msleep(50);
        c->json(QJsonObject{{"id", c->pathParam("id")}});
    });

    mServer = std::make_unique<HttpServer>();
    mServer->useLogger(std::make_shared<NullLogger>());
    mServer->useErrorCallback([](Ctx, const std::exception &e) {
        if (QByteArray(e.what()) == "trigger-callback")
            throw std::logic_error("callback failed");
    });
    mServer->useRouter(std::move(router));

    // HttpServer chưa cho biết cổng đã bind nên tìm một cổng trống trước
    quint16 port = 0;
    {
        QTcpServer probe;
        QVERIFY(probe.listen(QHostAddress::LocalHost, 0));
        port = probe.serverPort();
    }
    QVERIFY(mServer->listen(port));

    mHttp = std::make_unique<HttpClient>();
    mHttp->setBaseAddress(QUrl(QStringLiteral("http://127.0.0.1:%1").arg(port)));
    mHttp->setTimeout(5s);
}

void tst_Server::cleanupTestCase()
{
    mHttp.reset();
    mServer.reset();
}

void tst_Server::bothExecutionModes_respond()
{
    const QList<QPair<QString, QString>> cases = {{"/hello", "hi"}, {"/sync", "sync"}};

    for (const auto &[path, expectedMsg] : cases) {
        const HttpResponse res = await(mHttp->get(path));

        QVERIFY2(res.statusCode == 200, qPrintable(path));
        QVERIFY(res.header("Content-Type").startsWith("application/json"));
        QCOMPARE(res.json().object().value("msg").toString(), expectedMsg);
    }
}

void tst_Server::pathAndQueryParams_arriveInHandler()
{
    QUrlQuery query;
    query.addQueryItem("sort", "asc");

    const HttpResponse res = await(mHttp->get("/users/42", query));

    QCOMPARE(res.statusCode, 200);
    QCOMPARE(res.json().object(), (QJsonObject{{"id", "42"}, {"sort", "asc"}}));
}

void tst_Server::postBody_reachesHandler()
{
    const QJsonObject payload{{"a", 1}, {"b", "x"}};

    const HttpResponse res = await(mHttp->post("/echo", QJsonDocument(payload)));

    QCOMPARE(res.statusCode, 201);
    QCOMPARE(res.json().object(), payload);
}

void tst_Server::errors_mapToStatusCodes()
{
    const QList<std::tuple<QString, int, QString>> cases = {
        {"/boom", 500, "Internal Server Error"},
        {"/forbidden", 403, "nope"},
        {"/missing", 404, "Not Found"},
        {"/throw-int", 500, "Internal Server Error"},
        {"/throw-in-callback", 500, "Internal Server Error"},
    };

    for (const auto &[path, status, message] : cases) {
        const HttpResponse res = await(mHttp->get(path));

        QVERIFY2(res.statusCode == status,
                 qPrintable(QStringLiteral("%1: got %2").arg(path).arg(res.statusCode)));
        QCOMPARE(res.json().object().value("error").toString(), message);
    }
}

void tst_Server::slowHandler_gets503WithoutWaiting()
{
    QElapsedTimer timer;
    timer.start();

    const HttpResponse res = await(mHttp->get("/slow"));

    QCOMPARE(res.statusCode, 503);
    QCOMPARE(res.json().object().value("error").toString(), QStringLiteral("Request Timeout"));
    QVERIFY2(timer.elapsed() < 400, qPrintable(QString::number(timer.elapsed())));
}

void tst_Server::concurrentRequests_doNotMixResponses()
{
    constexpr int count = 10;

    QList<QFuture<HttpResponse>> futures;
    for (int i = 0; i < count; ++i)
        futures << mHttp->get(QStringLiteral("/work/%1").arg(i));

    for (int i = 0; i < count; ++i) {
        const HttpResponse res = await(futures[i]);

        QCOMPARE(res.statusCode, 200);
        QCOMPARE(res.json().object().value("id").toString(), QString::number(i));
    }
}

QTEST_GUILESS_MAIN(tst_Server)
#include "tst_server.moc"