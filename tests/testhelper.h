#include <QHttpHeaders>
#include <QMutex>
#include <QString>
#include <QStringList>
#include "httpserver/context.h"
#include "httpserver/ilogger.h"

namespace {

class FakeLogger : public ILogger
{
public:
    void debug(const QString &m) override { debugs << m; }
    void info(const QString &m) override { infos << m; }
    void warning(const QString &m) override { warnings << m; }
    void error(const QString &m) override { errors << m; }

    QStringList debugs, infos, warnings, errors;
};

struct Sent
{
    int status;
    QByteArray body;
    QHttpHeaders headers;
};

// Context + logger giả + sender giả, mỗi test tạo một cái mới
struct Fixture
{
    Fixture()
    {
        ctx->setMethod("GET");
        ctx->setPath("/test");
        ctx->setLogger(logger);
        ctx->setResponseSender(
            [this](int status, const QByteArray &body, const QHttpHeaders &headers) {
                QMutexLocker lock(&mutex);
                sent.append({status, body, headers});
            });
    }
    Fixture(const Fixture &) = delete;

    // dùng 2 hàm này khi sender có thể chạy trên thread khác
    int sentCount()
    {
        QMutexLocker lock(&mutex);
        return sent.size();
    }
    QList<Sent> snapshot()
    {
        QMutexLocker lock(&mutex);
        return sent;
    }

    std::shared_ptr<Context> ctx = std::make_shared<Context>();
    std::shared_ptr<FakeLogger> logger = std::make_shared<FakeLogger>();
    QList<Sent> sent;
    QMutex mutex;
};

} // namespace
