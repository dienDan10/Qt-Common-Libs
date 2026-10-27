/******************************************************************************
 * @file        httpclient.h
 * @brief
 *
 * @author      diendanx
 * @date        2026-10-27
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H
#include <QFuture>
#include <QNetworkAccessManager>
#include <QObject>
#include "httpresponse.h"
class HttpClient : public QObject
{
    Q_OBJECT
public:
    using Headers = QHash<QByteArray, QByteArray>;
    explicit HttpClient(QObject *parent = nullptr);
    explicit HttpClient(QNetworkAccessManager *manager, QObject *parent = nullptr);

    // cấu hình http client
    void setBaseAddress(const QUrl &url);
    QUrl getBaseAddress() const;

    void setDefaultHeader(const QByteArray &name, const QByteArray &value);
    void removeDefaultHeader(const QByteArray &name);
    Headers getDefaultHeaders() const;

    void setTimeout(std::chrono::milliseconds timeout);
    std::chrono::milliseconds getTimeout() const;

    // requests
    QFuture<HttpResponse> get(const QString &path, const Headers &headers = {});
    QFuture<HttpResponse> get(const QString &path,
                              const QUrlQuery &query,
                              const Headers &headers = {});

    QFuture<HttpResponse> post(const QString &path,
                               const QByteArray &body,
                               const QByteArray &contentType = "application/json",
                               const Headers &headers = {});
    QFuture<HttpResponse> post(const QString &path,
                               const QJsonDocument &json,
                               const Headers &headers = {});

    QFuture<HttpResponse> put(const QString &path,
                              const QByteArray &body,
                              const QByteArray &contentType = "application/json",
                              const Headers &headers = {});
    QFuture<HttpResponse> put(const QString &path,
                              const QJsonDocument &json,
                              const Headers &headers = {});

    QFuture<HttpResponse> patch(const QString &path,
                                const QByteArray &body,
                                const QByteArray &contentType = "application/json",
                                const Headers &headers = {});
    QFuture<HttpResponse> patch(const QString &path,
                                const QJsonDocument &json,
                                const Headers &headers = {});

    QFuture<HttpResponse> del(const QString &path, const Headers &headers = {});

    QFuture<HttpResponse> head(const QString &path, const Headers &headers = {});

    QFuture<HttpResponse> send(const QByteArray &method,
                               const QString &path,
                               const QByteArray &body = {},
                               const QByteArray &contentType = {},
                               const Headers &headers = {});

    void cancelAll();

private:
    QUrl resolveUrl(const QString &path) const;
    QFuture<HttpResponse> sendRequest(const QByteArray &method,
                                      const QUrl &url,
                                      const QByteArray &body,
                                      const QByteArray &contentType,
                                      const Headers &headers);
    static HttpResponse makeResponse(QNetworkReply *reply);

private:
    QNetworkAccessManager *mManager = nullptr;
    QUrl mBaseAddress;
    Headers mDefaultHeaders;
    std::chrono::milliseconds mTimeout{30000};
    bool mIgnoreSslErrors = false;
    QList<QNetworkReply *> mPending;
};

#endif // HTTPCLIENT_H
