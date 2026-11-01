/******************************************************************************
 * @file        httpclient.cpp
 * @brief
 *
 * @author      diendanx
 * @date        2026-10-27
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#include "httpclient.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QTimer>

namespace {
QNetworkAccessManager *sharedManager()
{
    thread_local QPointer<QNetworkAccessManager> nam;
    if (nam)
        return nam;

    nam = new QNetworkAccessManager;

    QThread *current = QThread::currentThread();
    QCoreApplication *app = QCoreApplication::instance();
    if (app && current == app->thread()) {
        nam->setParent(app);
    } else {
        QObject::connect(current, &QThread::finished, nam, &QObject::deleteLater);
    }

    return nam;
}

struct RequestState
{
    QPromise<HttpResponse> promise;
    bool isTimeout = false;
};
} // namespace

HttpClient::HttpClient(QObject *parent)
    : QObject(parent)
    , mManager(sharedManager())
{}

HttpClient::HttpClient(QNetworkAccessManager *manager, QObject *parent)
    : QObject(parent)
    , mManager(manager)
{}

HttpClient::~HttpClient()
{
    cancelAll();
}

void HttpClient::setBaseAddress(const QUrl &url)
{
    mBaseAddress = url;
}

QUrl HttpClient::getBaseAddress() const
{
    return mBaseAddress;
}

void HttpClient::setDefaultHeader(const QByteArray &name, const QByteArray &value)
{
    mDefaultHeaders.insert(name, value);
}

void HttpClient::removeDefaultHeader(const QByteArray &name)
{
    mDefaultHeaders.remove(name);
}

void HttpClient::setTimeout(std::chrono::milliseconds timeout)
{
    mTimeout = timeout;
}

std::chrono::milliseconds HttpClient::getTimeout() const
{
    return mTimeout;
}

void HttpClient::setIgnoreSslErrors(const bool value)
{
    mIgnoreSslErrors = value;
}

HttpClient::Headers HttpClient::getDefaultHeaders() const
{
    return mDefaultHeaders;
}

QFuture<HttpResponse> HttpClient::get(const QString &path, const Headers &headers)
{
    return send("GET", path, {}, {}, headers);
}

QFuture<HttpResponse> HttpClient::get(const QString &path,
                                      const QUrlQuery &query,
                                      const Headers &headers)
{
    QUrl url = resolveUrl(path);
    QUrlQuery merged(url);
    // QUrl::FullyEncoded - giữ nguyên trạng thái query, không thay đổi gì
    for (const auto &item : query.queryItems(QUrl::FullyEncoded)) {
        merged.addQueryItem(item.first, item.second);
    }

    url.setQuery(merged);
    return sendRequest("GET", url, {}, {}, headers);
}

QFuture<HttpResponse> HttpClient::post(const QString &path,
                                       const QByteArray &body,
                                       const QByteArray &contentType,
                                       const Headers &headers)
{
    return send("POST", path, body, contentType, headers);
}

QFuture<HttpResponse> HttpClient::post(const QString &path,
                                       const QJsonDocument &json,
                                       const Headers &headers)
{
    return send("POST", path, json.toJson(QJsonDocument::Compact), "application/json", headers);
}

QFuture<HttpResponse> HttpClient::put(const QString &path,
                                      const QByteArray &body,
                                      const QByteArray &contentType,
                                      const Headers &headers)
{
    return send("PUT", path, body, contentType, headers);
}

QFuture<HttpResponse> HttpClient::put(const QString &path,
                                      const QJsonDocument &json,
                                      const Headers &headers)
{
    return send("PUT", path, json.toJson(QJsonDocument::Compact), "application/json", headers);
}

QFuture<HttpResponse> HttpClient::patch(const QString &path,
                                        const QByteArray &body,
                                        const QByteArray &contentType,
                                        const Headers &headers)
{
    return send("PATCH", path, body, contentType, headers);
}

QFuture<HttpResponse> HttpClient::patch(const QString &path,
                                        const QJsonDocument &json,
                                        const Headers &headers)
{
    return send("PATCH", path, json.toJson(QJsonDocument::Compact), "application/json", headers);
}

QFuture<HttpResponse> HttpClient::del(const QString &path, const Headers &headers)
{
    return send("DELETE", path, {}, {}, headers);
}

QFuture<HttpResponse> HttpClient::head(const QString &path, const Headers &headers)
{
    return send("HEAD", path, {}, {}, headers);
}

QFuture<HttpResponse> HttpClient::send(const QByteArray &method,
                                       const QString &path,
                                       const QByteArray &body,
                                       const QByteArray &contentType,
                                       const Headers &headers)
{
    return sendRequest(method, resolveUrl(path), body, contentType, headers);
}

void HttpClient::cancelAll()
{
    if (!mManager) {
        mPending.clear();
        return;
    }
    // copy pending vì abort() làm signal finished phát luôn, và đồng thời xóa reply khỏi
    // pending, tức vừa duyệt vừa xóa
    const auto pending = mPending;
    for (QNetworkReply *reply : pending)
        reply->abort();
}

QUrl HttpClient::resolveUrl(const QString &path) const
{
    const QUrl url(path);
    if (url.isRelative() && mBaseAddress.isValid())
        return mBaseAddress.resolved(url);
    return url;
}

QFuture<HttpResponse> HttpClient::sendRequest(const QByteArray &method,
                                              const QUrl &url,
                                              const QByteArray &body,
                                              const QByteArray &contentType,
                                              const Headers &headers)
{
    if (!mManager) {
        QPromise<HttpResponse> promise;
        QFuture<HttpResponse> future = promise.future();
        promise.start();
        HttpResponse res;
        res.error = QNetworkReply::OperationCanceledError;
        res.errorString = QStringLiteral("Network manager has been destroyed");
        promise.addResult(res);
        promise.finish();
        return future; // future đã xong sẵn, .then() sẽ chạy ngay
    }

    QNetworkRequest request(url);

    for (auto it = mDefaultHeaders.cbegin(); it != mDefaultHeaders.cend(); it++) {
        request.setRawHeader(it.key(), it.value());
    }

    for (auto it = headers.cbegin(); it != headers.cend(); it++) { // ghi đè default nếu trùng
        request.setRawHeader(it.key(), it.value());
    }

    if (!contentType.isEmpty()) {
        request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    }

    QNetworkReply *reply = nullptr;
    if (method == "GET")
        reply = mManager->get(request);
    else if (method == "POST")
        reply = mManager->post(request, body);
    else if (method == "PUT")
        reply = mManager->put(request, body);
    else if (method == "DELETE" && body.isEmpty())
        reply = mManager->deleteResource(request);
    else if (method == "HEAD")
        reply = mManager->head(request);
    else
        reply = mManager->sendCustomRequest(request, method, body);

    // tạo một promise để đợi khi response trả về
    auto promiseState = std::make_shared<RequestState>();
    QFuture<HttpResponse> future = (promiseState->promise).future();
    (promiseState->promise).start();

    if (mTimeout > std::chrono::milliseconds::zero()) {
        auto timer = new QTimer(reply);
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, reply, [promiseState, reply]() {
            promiseState->isTimeout = true;
            reply->abort();
        });
        timer->start(mTimeout);
    }

    mPending.append(reply);

    if (mIgnoreSslErrors) { // không báo lỗi ssl mà tiếp tục gửi request
        connect(reply, &QNetworkReply::sslErrors, reply, [reply](const QList<QSslError> &) {
            reply->ignoreSslErrors();
        });
    }

    connect(reply, &QNetworkReply::finished, this, [this, reply, promiseState]() {
        mPending.removeOne(reply);
        HttpResponse res = makeResponse(reply);

        if (promiseState->isTimeout) {
            res.error = QNetworkReply::TimeoutError;
            res.errorString = QStringLiteral("Request timed out");
        }

        (promiseState->promise).addResult(res);
        (promiseState->promise).finish();
        reply->deleteLater();
    });

    return future;
}

HttpResponse HttpClient::makeResponse(QNetworkReply *reply)
{
    HttpResponse res;
    res.statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->isOpen()) {
        res.body = reply->readAll();
    }
    res.headers = reply->rawHeaderPairs();
    res.error = reply->error();
    res.errorString = (res.error == QNetworkReply::NoError) ? QString() : reply->errorString();
    res.url = reply->url();
    return res;
}
