/******************************************************************************
 * @file        httpserver.h
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-15
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QHttpServer>
#include <QObject>
#include <QTcpServer>
#include "context.h"
#include "ilogger.h"
#include "router.h"

class HttpServer : public QObject
{
    Q_OBJECT
public:
    explicit HttpServer(QObject *parent = nullptr);

    void useRouter(Router router);
    void useLogger(std::shared_ptr<ILogger> logger);
    void setDefaultTimeout(int ms);
    void useErrorCallback(Context::ErrorCallback callback);
    bool listen(quint16 port);

private:
    void handlerRequest(const QHttpServerRequest &request, QHttpServerResponder &responder);

private:
    Router mRouter; // each server will only have 1 router
    int mDefaultTimeout = Route::NoTimeOut;
    std::shared_ptr<ILogger> mLogger;
    Context::ErrorCallback mErrorCallback;

    QHttpServer *mHttpServer;
};

#endif // HTTPSERVER_H
