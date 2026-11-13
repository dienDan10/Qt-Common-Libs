/******************************************************************************
 * @file        context.cpp
 * @brief
 *
 * @author      diendanx
 * @date        2026-07-29
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/

#include "context.h"
#include <QJsonObject>
#include <QJsonParseError>

Context::Context() {}

QString Context::method() const
{
    return mMethod;
}

QString Context::path() const
{
    return mPath;
}

QString Context::pathParam(const QString &key, const QString &defaultValue) const
{
    return mPathParams.value(key, defaultValue);
}

QString Context::queryParam(const QString &key, const QString &defaultValue) const
{
    return mQueryParams.value(key, defaultValue);
}

QByteArray Context::requestHeader(const QByteArray &key, const QByteArray &defaultValue) const
{
    return mRequestHeaders.value(key, defaultValue).toByteArray();
}

QMap<QString, QString> Context::cookies() const
{
    QMap<QString, QString> cookies;
    QList<QByteArray> lines = mRequestHeaders.values(QHttpHeaders::WellKnownHeader::Cookie);
    if (lines.isEmpty())
        return cookies;

    QString raw = QString::fromUtf8(lines.join("; "));
    const QStringList pairs = raw.split("; ", Qt::SkipEmptyParts);
    for (const QString &pair : pairs) {
        int eq = pair.indexOf('=');
        if (eq > 0) {
            cookies.insert(pair.left(eq).trimmed(), pair.mid(eq + 1).trimmed());
        }
    }
    return cookies;
}

QString Context::cookie(const QString &name, const QString defaultValue) const
{
    return cookies().value(name, defaultValue);
}

QByteArray Context::requestBody() const
{
    return mRequestBody;
}

QJsonObject Context::requestBodyJson(bool *ok) const
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(mRequestBody, &parseError);
    bool success = (parseError.error == QJsonParseError::NoError) && doc.isObject();
    if (ok)
        *ok = success;

    return success ? doc.object() : QJsonObject();
}

QString Context::getRequestIp() const
{
    return mRequestIp;
}

void Context::setMethod(const QString &method)
{
    mMethod = method;
}

void Context::setPath(const QString &path)
{
    mPath = path;
}

void Context::setPathParams(const QMap<QString, QString> &params)
{
    mPathParams = params;
}

void Context::setQueryParams(const QMap<QString, QString> &params)
{
    mQueryParams = params;
}

void Context::setHeaders(const QHttpHeaders &header)
{
    mRequestHeaders = header;
}

void Context::setRequestBody(const QByteArray &body)
{
    mRequestBody = body;
}

void Context::setRequestIp(const QString &ip)
{
    mRequestIp = ip;
}

void Context::json(const QJsonObject &obj, int statusCode)
{
    finalizeResponse(statusCode,
                     QJsonDocument(obj).toJson(QJsonDocument::Compact),
                     "application/json");
}

void Context::json(const QJsonArray &arr, int statusCode)
{
    finalizeResponse(statusCode,
                     QJsonDocument(arr).toJson(QJsonDocument::Compact),
                     "application/json");
}

void Context::text(const QString &text, int statusCode)
{
    finalizeResponse(statusCode, text.toUtf8(), "text/plain; charset=utf-8");
}

void Context::error(int statusCode, const QString &message)
{
    QJsonObject obj;
    obj["error"] = message;
    return json(obj, statusCode);
}

void Context::setResponseHeader(const QByteArray &name, const QByteArray &value)
{
    mResponseHeaders.replaceOrAppend(name, value);
}

void Context::setCookie(const QString &name, const QString &value, const CookieOptions &options)
{
    QString cookieStr = QString("%1=%2; Path=%3").arg(name, value, options.path);

    if (!options.domain.isEmpty())
        cookieStr += "; Domain=" + options.domain;
    if (options.maxAgeSeconds >= 0)
        cookieStr += "; Max-Age=" + QString::number(options.maxAgeSeconds);
    if (options.httpOnly)
        cookieStr += "; HttpOnly";
    if (options.secure)
        cookieStr += "; Secure";
    if (!options.sameSite.isEmpty())
        cookieStr += "; SameSite=" + options.sameSite;

    mResponseHeaders.append(QHttpHeaders::WellKnownHeader::SetCookie, cookieStr.toUtf8());
}

bool Context::hasResponded() const
{
    return mResponded.load();
}

int Context::statusCode() const
{
    return mStatusCode;
}

QByteArray Context::responseBody() const
{
    return mResponseBody;
}

void Context::onBeforeResponse(BeforeResponseHook hook)
{
    mBeforeResponseHooks.append(std::move(hook));
}

void Context::setResponseSender(ResponseSender sender)
{
    mResponseSender = std::move(sender);
}

void Context::setLogger(std::shared_ptr<ILogger> logger)
{
    mLogger = std::move(logger);
}

std::shared_ptr<ILogger> Context::logger() const
{
    return mLogger;
}

void Context::setErrorCallback(ErrorCallback callback)
{
    mErrorCallback = std::move(callback);
}

Context::ErrorCallback Context::errorCallback() const
{
    return mErrorCallback;
}

void Context::finalizeResponse(int statusCode, const QByteArray &body, const QByteArray &contentType)
{
    bool expected = false;
    if (!mResponded.compare_exchange_strong(expected, true)) {
        // request đã được gửi, log
        mLogger->warning(
            QString("Response already sent, ignoring duplicate call for %1 %2").arg(mMethod, mPath));
        return;
    }

    mStatusCode = statusCode;
    mResponseBody = body;
    mResponseHeaders.replaceOrAppend(QHttpHeaders::WellKnownHeader::ContentType, contentType);

    for (int i = 0; i < mBeforeResponseHooks.size(); ++i) {
        mBeforeResponseHooks.at(i)(*this);
    }

    if (!mResponseSender) {
        // không tồn tại phương thức gửi, log
        mLogger->error(
            QString("No response sender set for %1 %2, response dropped").arg(mMethod, mPath));
        return;
    }

    mResponseSender(statusCode, body, mResponseHeaders);
}