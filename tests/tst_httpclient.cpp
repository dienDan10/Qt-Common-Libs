#include <QCoreApplication>
#include <QJsonObject>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <QThread>
#include <QTimer>
#include <QUrlQuery>

#include "httpclient/httpclient.h"

using namespace std::chrono_literals;

// ============================================================================
// Mock HTTP server chạy local, không cần internet.
//   /echo           200, trả JSON gồm method, path, query, body và vài header nhận được
//   /notfound       404 kèm body JSON
//   /slow           không bao giờ trả lời (test timeout / cancel / huỷ client)
//   /delay?ms=N     trả lời sau N ms
//   /trickle        gửi 1 byte mỗi 100ms, tổng 10 byte (test timeout tính tổng thời gian)
// HEAD request: chỉ trả header, không có body.
// ============================================================================
class MockServer : public QTcpServer
{
public:
    MockServer()
    {
        listen(QHostAddress::LocalHost);
        connect(this, &QTcpServer::newConnection, this, [this]() {
            while (QTcpSocket *socket = nextPendingConnection()) {
                auto buffer = std::make_shared<QByteArray>();
                connect(socket, &QTcpSocket::readyRead, socket, [socket, buffer]() {
                    buffer->append(socket->readAll());
                    handle(socket, *buffer);
                });
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            }
        });
    }

    QUrl url(const QString &path = "/") const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1%2").arg(serverPort()).arg(path));
    }

private:
    static void handle(QTcpSocket *socket, QByteArray &buffer)
    {
        const int headerEnd = buffer.indexOf("\r\n\r\n");
        if (headerEnd < 0)
            return;

        const QList<QByteArray> lines = buffer.left(headerEnd).split('\n');
        const QList<QByteArray> requestLine = lines.first().trimmed().split(' ');
        const QByteArray method = requestLine.value(0);
        const QByteArray target = requestLine.value(1);

        QHash<QByteArray, QByteArray> headers;
        for (int i = 1; i < lines.size(); ++i) {
            const int colon = lines[i].indexOf(':');
            if (colon > 0)
                headers.insert(lines[i].left(colon).trimmed().toLower(),
                               lines[i].mid(colon + 1).trimmed());
        }

        const int contentLength = headers.value("content-length", "0").toInt();
        if (buffer.size() < headerEnd + 4 + contentLength)
            return; // chờ đủ body
        const QByteArray body = buffer.mid(headerEnd + 4, contentLength);
        buffer.clear();

        const QUrl url(QString::fromLatin1(target));
        const QString path = url.path();
        const bool isHead = (method == "HEAD");

        if (path == "/slow")
            return;

        if (path == "/notfound") {
            reply(socket, 404, "Not Found", R"({"error":"not found"})", isHead);
            return;
        }

        if (path == "/delay") {
            const int ms = QUrlQuery(url).queryItemValue("ms").toInt();
            QPointer<QTcpSocket> guard(socket);
            QTimer::singleShot(ms, socket, [guard, isHead]() {
                if (guard)
                    reply(guard, 200, "OK", R"({"delayed":true})", isHead);
            });
            return;
        }

        if (path == "/trickle") {
            socket->write("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n"
                          "Content-Length: 10\r\nConnection: close\r\n\r\n");
            auto *timer = new QTimer(socket);
            auto sent = std::make_shared<int>(0);
            QObject::connect(timer, &QTimer::timeout, socket, [socket, timer, sent]() {
                socket->write("x");
                if (++*sent == 10) {
                    timer->stop();
                    socket->disconnectFromHost();
                }
            });
            timer->start(100);
            return;
        }

        QJsonObject obj;
        obj["method"] = QString::fromLatin1(method);
        obj["path"] = path;
        obj["query"] = url.query();
        obj["body"] = QString::fromUtf8(body);
        obj["contentType"] = QString::fromLatin1(headers.value("content-type"));
        obj["auth"] = QString::fromLatin1(headers.value("authorization"));
        obj["custom"] = QString::fromLatin1(headers.value("x-custom"));
        reply(socket, 200, "OK", QJsonDocument(obj).toJson(QJsonDocument::Compact), isHead);
    }

    static void reply(QTcpSocket *socket, int status, const QByteArray &reason,
                      const QByteArray &body, bool headOnly)
    {
        QByteArray out = "HTTP/1.1 " + QByteArray::number(status) + " " + reason + "\r\n";
        out += "Content-Type: application/json\r\n";
        out += "X-Server: mock\r\n";
        out += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
        out += "Connection: close\r\n\r\n";
        if (!headOnly)
            out += body;
        socket->write(out);
        socket->disconnectFromHost();
    }
};

// Chờ future xong mà vẫn cho event loop chạy.
// (Không dùng future.waitForFinished(): nó chặn thread -> reply không bao giờ xong)
static HttpResponse await(QFuture<HttpResponse> future, int timeoutMs = 5000)
{
    if (!QTest::qWaitFor([&]() { return future.isFinished(); }, timeoutMs)) {
        qWarning("await: request không xong trong thời gian chờ của test");
        return {};
    }
    if (future.isCanceled())
        return {};
    return future.result();
}

class TestHttpClient : public QObject
{
    Q_OBJECT

private slots:
    // ------------------------------------------------------------------------
    // Nhóm 1: request cơ bản
    // ------------------------------------------------------------------------
    void getJson()
    {
        MockServer server;
        HttpClient http;
        http.setBaseAddress(server.url("/api/"));

        const HttpResponse res = await(http.get("echo"));
        QVERIFY(res.isSuccess());
        QCOMPARE(res.statusCode, 200);

        // base "/api/" + "echo" -> "/api/echo" (giống .NET, base cần dấu / ở cuối)
        const QJsonObject obj = res.json().object();
        QCOMPARE(obj["method"].toString(), QString("GET"));
        QCOMPARE(obj["path"].toString(), QString("/api/echo"));
    }

    void responseHeaderLookup()
    {
        MockServer server;
        HttpClient http;
        http.setBaseAddress(server.url());

        const HttpResponse res = await(http.get("/echo"));
        QCOMPARE(res.header("X-Server"), QByteArray("mock"));
        QCOMPARE(res.header("x-server"), QByteArray("mock"));     // không phân biệt hoa thường
        QCOMPARE(res.header("CONTENT-TYPE"), QByteArray("application/json"));
        QVERIFY(res.header("X-Not-Exist").isEmpty());             // không có thì trả rỗng
    }

    void getWithQuery()
    {
        MockServer server;
        HttpClient http;
        http.setBaseAddress(server.url());

        QUrlQuery query;
        query.addQueryItem("page", "2");
        query.addQueryItem("q", "hà nội");
        query.addQueryItem("expr", "a&b=c");   // ký tự đặc biệt không được làm vỡ query
        const HttpResponse res = await(http.get("echo?sort=asc", query));

        QVERIFY(res.isSuccess());
        const QUrlQuery received(res.json().object()["query"].toString());
        QCOMPARE(received.queryItemValue("sort"), QString("asc"));   // query sẵn trong path
        QCOMPARE(received.queryItemValue("page"), QString("2"));
        QCOMPARE(received.queryItemValue("q", QUrl::FullyDecoded), QString("hà nội"));
        QCOMPARE(received.queryItemValue("expr", QUrl::FullyDecoded), QString("a&b=c"));
    }

    void postJson()
    {
        MockServer server;
        HttpClient http;
        http.setBaseAddress(server.url());

        const QJsonDocument payload(QJsonObject{{"name", "Tu"}, {"age", 30}});
        const HttpResponse res = await(http.post("/echo", payload));

        QVERIFY(res.isSuccess());
        const QJsonObject obj = res.json().object();
        QCOMPARE(obj["method"].toString(), QString("POST"));
        QCOMPARE(obj["contentType"].toString(), QString("application/json"));
        QCOMPARE(QJsonDocument::fromJson(obj["body"].toString().toUtf8()), payload);
    }

    void postRawWithContentType()
    {
        MockServer server;
        HttpClient http;
        http.setBaseAddress(server.url());

        const HttpResponse res = await(http.post("/echo", QByteArray("a=1&b=2"),
                                                 "application/x-www-form-urlencoded"));
        const QJsonObject obj = res.json().object();
        QCOMPARE(obj["contentType"].toString(), QString("application/x-www-form-urlencoded"));
        QCOMPARE(obj["body"].toString(), QString("a=1&b=2"));
    }

    void putPatchDelete()
    {
        MockServer server;
        HttpClient http;
        http.setBaseAddress(server.url());

        QCOMPARE(await(http.put("/echo", QByteArray("x"), "text/plain")).json()["method"].toString(),
                 QString("PUT"));
        QCOMPARE(await(http.patch("/echo", QJsonDocument(QJsonObject{{"a", 1}})))
                     .json()["method"].toString(),
                 QString("PATCH"));
        QCOMPARE(await(http.del("/echo")).json()["method"].toString(), QString("DELETE"));
    }

    void headReturnsHeadersOnly()
    {
        MockServer server;
        HttpClient http;
        http.setBaseAddress(server.url());

        const HttpResponse res = await(http.head("/echo"));
        QVERIFY(res.isSuccess());
        QVERIFY(res.body.isEmpty());
        QCOMPARE(res.header("X-Server"), QByteArray("mock"));
    }

    void defaultAndPerRequestHeaders()
    {
        MockServer server;
        HttpClient http;
        http.setBaseAddress(server.url());
        http.setDefaultHeader("Authorization", "Bearer abc");
        http.setDefaultHeader("X-Custom", "default");

        QJsonObject obj = await(http.get("/echo")).json().object();
        QCOMPARE(obj["auth"].toString(), QString("Bearer abc"));
        QCOMPARE(obj["custom"].toString(), QString("default"));

        // header truyền riêng ghi đè default
        obj = await(http.get("/echo", HttpClient::Headers{{"X-Custom", "override"}})).json().object();
        QCOMPARE(obj["custom"].toString(), QString("override"));

        http.removeDefaultHeader("Authorization");
        http.removeDefaultHeader("X-Not-Exist");   // xoá header không có: không lỗi
        obj = await(http.get("/echo")).json().object();
        QCOMPARE(obj["auth"].toString(), QString());
    }

    void invalidJsonReportsParseError()
    {
        HttpResponse res;
        res.body = R"({"name": "Tu", )";   // JSON bị cắt giữa chừng

        QJsonParseError err;
        const QJsonDocument doc = res.json(&err);
        QVERIFY(doc.isNull());
        QVERIFY(err.error != QJsonParseError::NoError);

        res.body.clear();                  // body rỗng cũng là lỗi parse, nhưng phân biệt được qua body
        res.json(&err);
        QVERIFY(err.error != QJsonParseError::NoError);
    }

    // ------------------------------------------------------------------------
    // Nhóm 2: lỗi và timeout
    // ------------------------------------------------------------------------
    void httpErrorStillHasBody()
    {
        MockServer server;
        HttpClient http;
        http.setBaseAddress(server.url());

        const HttpResponse res = await(http.get("/notfound"));
        QVERIFY(!res.isSuccess());
        QVERIFY(!res.isNetworkError());
        QCOMPARE(res.statusCode, 404);
        QCOMPARE(res.json()["error"].toString(), QString("not found"));
    }

    void connectionRefused()
    {
        quint16 port;
        {
            QTcpServer tmp;
            tmp.listen(QHostAddress::LocalHost);
            port = tmp.serverPort();
        } // đóng lại -> port không còn ai nghe

        HttpClient http;
        const HttpResponse res = await(http.get(QStringLiteral("http://127.0.0.1:%1/").arg(port)));
        QVERIFY(res.isNetworkError());
        QCOMPARE(res.error, QNetworkReply::ConnectionRefusedError);
        QVERIFY(!res.errorString.isEmpty());
    }

    void timeoutGivesTimeoutError()
    {
        MockServer server;
        HttpClient http;
        http.setBaseAddress(server.url());
        http.setTimeout(300ms);

        const HttpResponse res = await(http.get("/slow"));
        QVERIFY(res.isNetworkError());
        QCOMPARE(res.error, QNetworkReply::TimeoutError);
        QCOMPARE(res.errorString, QString("Request timed out"));
    }

    void timeoutCountsTotalTime()
    {
        // Server vẫn gửi dữ liệu đều đặn (mỗi 100ms) nhưng tổng cộng mất ~1s.
        // Timeout 300ms phải tính trên tổng thời gian -> vẫn timeout,
        // khác với setTransferTimeout của Qt (bị reset mỗi khi có dữ liệu).
        MockServer server;
        HttpClient http;
        http.setBaseAddress(server.url());
        http.setTimeout(300ms);

        const HttpResponse res = await(http.get("/trickle"));
        QCOMPARE(res.error, QNetworkReply::TimeoutError);
    }

    void requestFinishingBeforeTimeoutSucceeds()
    {
        MockServer server;
        HttpClient http;
        http.setBaseAddress(server.url());
        http.setTimeout(1000ms);

        const HttpResponse res = await(http.get("/delay?ms=100"));
        QVERIFY(res.isSuccess());
        QCOMPARE(res.error, QNetworkReply::NoError);
    }

    void zeroTimeoutMeansNoTimeout()
    {
        MockServer server;
        HttpClient http;
        http.setBaseAddress(server.url());
        http.setTimeout(0ms);

        const HttpResponse res = await(http.get("/delay?ms=200"));
        QVERIFY(res.isSuccess());
        QCOMPARE(res.error, QNetworkReply::NoError);
    }

    void cancelAllGivesCanceledNotTimeout()
    {
        MockServer server;
        HttpClient http;
        http.setBaseAddress(server.url());
        http.setTimeout(5000ms);

        auto f1 = http.get("/slow");
        auto f2 = http.get("/slow");
        QTest::qWait(50);
        http.cancelAll();

        QCOMPARE(await(f1).error, QNetworkReply::OperationCanceledError);
        QCOMPARE(await(f2).error, QNetworkReply::OperationCanceledError);
    }

    void thenContinuationRunsOnContextThread()
    {
        MockServer server;
        HttpClient http;
        http.setBaseAddress(server.url());

        int status = 0;
        bool sameThread = false;
        http.get("/echo").then(this, [&](const HttpResponse &res) {
            status = res.statusCode;
            sameThread = QThread::currentThread() == thread();
        });

        QTRY_COMPARE(status, 200);
        QVERIFY(sameThread);
    }

    // ------------------------------------------------------------------------
    // Nhóm 3: vòng đời
    // ------------------------------------------------------------------------
    void destroyingClientStillDeliversResult()
    {
        MockServer server;
        auto *http = new HttpClient;
        http->setBaseAddress(server.url());

        bool called = false;
        QNetworkReply::NetworkError error = QNetworkReply::NoError;
        auto future = http->get("/slow").then(this, [&](const HttpResponse &res) {
            called = true;
            error = res.error;
        });

        QTest::qWait(50);
        delete http;   // vd: đóng dialog khi request đang chạy

        // .then() phải được gọi với lỗi rõ ràng, không bị cancel âm thầm
        QTRY_VERIFY(called);
        QCOMPARE(error, QNetworkReply::OperationCanceledError);
    }

    void requestAfterManagerDestroyed()
    {
        auto *nam = new QNetworkAccessManager;
        HttpClient http(nam);
        delete nam;

        QVERIFY(http.manager() == nullptr);
        auto future = http.get("http://127.0.0.1:1/");
        QVERIFY(future.isFinished());   // trả kết quả ngay, không gửi gì
        const HttpResponse res = future.result();
        QCOMPARE(res.error, QNetworkReply::OperationCanceledError);
        QCOMPARE(res.errorString, QString("Network manager has been destroyed"));
    }

    void managerDestroyedWhileRequestPending()
    {
        // Tình huống lúc tắt app: QNAM chết trước client khi request còn chạy.
        // Yêu cầu: không crash; future kết thúc (ở trạng thái bị cancel).
        MockServer server;
        auto *nam = new QNetworkAccessManager;
        auto *http = new HttpClient(nam);
        http->setBaseAddress(server.url());
        http->setTimeout(5000ms);

        auto future = http->get("/slow");
        QTest::qWait(50);
        delete nam;          // reply chết theo QNAM
        // future phải kết thúc NGAY, không phải đợi tới khi timer timeout hết hạn
        QVERIFY(future.isFinished());
        QVERIFY(future.isCanceled());

        delete http;         // cancelAll() không được đụng vào reply đã chết
    }

    // ------------------------------------------------------------------------
    // Nhóm 4: sharedManager()
    // ------------------------------------------------------------------------
    void sameThreadSharesManager()
    {
        HttpClient a;
        HttpClient b;
        QVERIFY(a.manager() != nullptr);
        QCOMPARE(a.manager(), b.manager());
        QCOMPARE(a.manager()->parent(), QCoreApplication::instance());   // main thread -> gắn qApp
    }

    void explicitManagerIsUsed()
    {
        QNetworkAccessManager own;
        HttpClient shared;
        HttpClient custom(&own);
        QCOMPARE(custom.manager(), &own);
        QVERIFY(custom.manager() != shared.manager());
    }

    void sharedManagerRecreatedAfterDeletion()
    {
        QNetworkAccessManager *first = nullptr;
        {
            HttpClient a;
            first = a.manager();
        }
        delete first;   // ai đó xoá QNAM dùng chung

        HttpClient b;
        QVERIFY(b.manager() != nullptr);   // QPointer trong sharedManager() phát hiện -> tạo mới

        MockServer server;
        b.setBaseAddress(server.url());
        QVERIFY(await(b.get("/echo")).isSuccess());
    }

    void workerThreadHasOwnManagerAndCleansUp()
    {
        MockServer server;   // server chạy ở main thread
        HttpClient mainClient;
        QNetworkAccessManager *mainNam = mainClient.manager();

        QThread worker;
        QObject context;     // object sống trong worker để chạy code trên thread đó
        context.moveToThread(&worker);
        worker.start();

        QPointer<QNetworkAccessManager> workerNam;
        bool sameWithinWorker = false;
        bool namInWorkerThread = false;
        std::atomic<int> status{0};
        HttpClient *workerClient = nullptr;

        QMetaObject::invokeMethod(&context, [&]() {
            workerClient = new HttpClient;
            HttpClient second;
            workerNam = workerClient->manager();
            sameWithinWorker = (second.manager() == workerClient->manager());
            namInWorkerThread = (workerNam->thread() == QThread::currentThread());

            workerClient->setBaseAddress(server.url());
            workerClient->get("/echo").then(workerClient, [&](const HttpResponse &res) {
                status = res.statusCode;
            });
        }, Qt::BlockingQueuedConnection);

        QVERIFY(workerNam);
        QVERIFY(workerNam.data() != mainNam);   // mỗi thread một QNAM
        QVERIFY(sameWithinWorker);              // cùng thread -> dùng chung
        QVERIFY(namInWorkerThread);
        QVERIFY(workerNam->parent() == nullptr);

        // request trên worker thread chạy được (worker có event loop)
        QTRY_COMPARE(status.load(), 200);

        QMetaObject::invokeMethod(&context, [&]() { delete workerClient; },
                                  Qt::BlockingQueuedConnection);

        worker.quit();
        QVERIFY(worker.wait(3000));

        // QNAM của worker được dọn khi thread kết thúc
        QVERIFY(workerNam.isNull());
        // QNAM của main thread không bị ảnh hưởng
        QCOMPARE(mainClient.manager(), mainNam);
    }
};

QTEST_GUILESS_MAIN(TestHttpClient)
#include "tst_httpclient.moc"