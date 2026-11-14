/******************************************************************************
 * @file        httpserver.cpp
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-15
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#include "httpserver.h"
#include "defaultlogger.h"

namespace {
QString methodToString(QHttpServerRequest::Method method)
{
    switch (method) {
    case QHttpServerRequest::Method::Get:
        return "GET";
    case QHttpServerRequest::Method::Post:
        return "POST";
    case QHttpServerRequest::Method::Put:
        return "PUT";
    case QHttpServerRequest::Method::Patch:
        return "PATCH";
    case QHttpServerRequest::Method::Delete:
        return "DELETE";
    default:
        return "UNKNOWN";
    }
}
} // namespace

HttpServer::HttpServer(QObject *parent)
    : QObject{parent}
    , mHttpServer(new QHttpServer(this))
    , mLogger(std::make_shared<DefaultLogger>())
{
    mHttpServer->setMissingHandler(this, [this](const QHttpServerRequest &request, QHttpServerResponder &responder) {
        handlerRequest(request, responder);
    });
}

void HttpServer::useRouter(Router router)
{
    mRouter = std::move(router);
}

void HttpServer::useLogger(std::shared_ptr<ILogger> logger)
{
    if (!logger) {
        mLogger->warning("Server::useLogger() called with nullptr, ignoring - keep default logger");
        return;
    }

    mLogger = std::move(logger);
}

void HttpServer::setDefaultTimeout(int ms)
{
    mDefaultTimeout = ms;
}

void HttpServer::useErrorCallback(Context::ErrorCallback callback)
{
    mErrorCallback = callback;
}

bool HttpServer::listen(quint16 port)
{
    auto mTcpServer = new QTcpServer(mHttpServer);
    if (!mTcpServer->listen(QHostAddress::Any, port)) {
        mLogger->error(QString("Server: cannot bind to port %1").arg(port));
        return false;
    }

    if (!mHttpServer->bind(mTcpServer)) {
        mLogger->error("Server: QHttpServer::bind to tcp server failed");
        return false;
    }

    mLogger->info(QString("Server is listening on port %1").arg(port));
    return true;
}

void HttpServer::handlerRequest(const QHttpServerRequest &request, QHttpServerResponder &responder)
{
    // get method and url path of request
    auto ctx = std::make_shared<Context>();
    ctx->setMethod(methodToString(request.method()));
    ctx->setPath(request.url().path());

    // get the query params in url
    QMap<QString, QString> queryParams;
    QUrlQuery query(request.url()); // QUrlQuery only decode percent-encoding (%20), so careful here
    for (const auto &item : query.queryItems()) {
        queryParams.insert(item.first, item.second);
    }
    ctx->setQueryParams(queryParams);

    ctx->setHeaders(request.headers());
    ctx->setRequestBody(request.body());
    ctx->setLogger(mLogger);
    ctx->setErrorCallback(mErrorCallback);

    // dispatch the context to router
    auto sharedResponder = std::make_shared<QHttpServerResponder>(std::move(responder));
    ctx->setResponseSender([this, sharedResponder](int statusCode, const QByteArray &body, const QHttpHeaders &headers) {
        /**
         * use QMetaObject::invokeMethod because responder can be call in another thread
         * for QHttpServer, responder can only return response in same thread as server
         * invokeMethod execute lambda in same thread as context, which is this 
         */
        QMetaObject::invokeMethod(this, [sharedResponder, statusCode, body, headers]() {
            sharedResponder->write(body, headers, static_cast<QHttpServerResponse::StatusCode>(statusCode));
        });
    });

    bool matched = mRouter.dispatch(ctx, mDefaultTimeout);
    if (!matched) {
        ctx->error(404, "Not Found");
    }
}
