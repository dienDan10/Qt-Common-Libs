/******************************************************************************
 * @file        context.h
 * @brief       Chứa dữ liệu của request và response, chứa luôn method để
 *              trả response về client. Vòng đời trong 1 request một instance
 *
 * @author      diendanx
 * @date        2026-07-29
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/

#ifndef CONTEXT_H
#define CONTEXT_H

#include <QByteArray>
#include <QHttpHeaders>
#include <QMap>
#include <QSharedPointer>
#include <QString>
#include "ilogger.h"
#include <atomic>
#include <functional>

struct CookieOptions
{
    QString path = "/";
    QString domain; // trống - mặc định là domain hiện tại
    int maxAgeSeconds = -1;
    bool httpOnly = true;
    bool secure = true;
    QString sameSite = "Lax"; // Strict | Lax | None
};

class Context
{
public:
    Context();
    using ResponseSender
        = std::function<void(int statusCode, const QByteArray &body, const QHttpHeaders &headers)>;
    using BeforeResponseHook = std::function<void(Context &ctx)>;
    using ErrorCallback = std::function<void(std::shared_ptr<Context> ctx, const std::exception &e)>;

    // REQUEST
    QString method() const;
    QString path() const;
    QString pathParam(const QString &key, const QString &defaultValue = QString()) const;
    QString queryParam(const QString &key, const QString &defaultValue = QString()) const;
    QByteArray requestHeader(const QByteArray &key,
                             const QByteArray &defaultValue = QByteArray()) const;
    QMap<QString, QString> cookies() const;
    QString cookie(const QString &name, const QString defaultValue = QString()) const;
    QByteArray requestBody() const;
    QJsonObject requestBodyJson(bool *ok = nullptr) const;
    QString getRequestIp() const;

    void setMethod(const QString &method);
    void setPath(const QString &path);
    void setPathParams(const QMap<QString, QString> &params);
    void setQueryParams(const QMap<QString, QString> &params);
    void setHeaders(const QHttpHeaders &header);
    void setRequestBody(const QByteArray &body);
    void setRequestIp(const QString &ip);

    // RESPONSE
    void json(const QJsonObject &obj, int statusCode = 200);
    void json(const QJsonArray &arr, int statusCode = 200);
    void text(const QString &text, int statusCode = 200);
    void error(int statusCode, const QString &message);
    void setResponseHeader(const QByteArray &name, const QByteArray &value);
    void setCookie(const QString &name,
                   const QString &value,
                   const CookieOptions &options = CookieOptions());

    // state of the response
    bool hasResponded() const;
    int statusCode() const;
    QByteArray responseBody() const;

    // hook to add middleware to process the response before sending
    void onBeforeResponse(BeforeResponseHook hook);

    // connect context to the real connection
    void setResponseSender(ResponseSender sender);

    // logger
    void setLogger(std::shared_ptr<ILogger> logger);
    std::shared_ptr<ILogger> logger() const;

    // error callback
    void setErrorCallback(ErrorCallback callback);
    ErrorCallback errorCallback() const;

private:
    void finalizeResponse(int statusCode, const QByteArray &body, const QByteArray &contentType);

private:
    QString mMethod;
    QString mPath;
    QMap<QString, QString> mPathParams;
    QMap<QString, QString> mQueryParams;
    QHttpHeaders mRequestHeaders;
    QByteArray mRequestBody;
    QString mRequestIp;

    int mStatusCode = 0;
    QByteArray mResponseBody;
    QHttpHeaders mResponseHeaders;

    std::atomic<bool> mResponded{false};
    QList<BeforeResponseHook> mBeforeResponseHooks;
    ResponseSender mResponseSender;
    ErrorCallback mErrorCallback;
    std::shared_ptr<ILogger> mLogger;
};

#endif