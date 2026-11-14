/******************************************************************************
 * @file        router.cpp
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-14
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#include "router.h"
#include <QThreadPool>
#include <QTimer>
#include "errorhandler.h"
namespace {
QList<QString> splitPath(const QString &path)
{
    return path.split('/', Qt::SkipEmptyParts);
}

bool matchSegments(const QList<QString> &routeSegments,
                   const QList<QString> &requestSegments,
                   QMap<QString, QString> &outParams)
{
    if (routeSegments.size() != requestSegments.size())
        return false;

    for (int i = 0; i < routeSegments.size(); ++i) {
        const QString &routeSeg = routeSegments.at(i);
        const QString &requestSeg = requestSegments.at(i);

        if (routeSeg.startsWith(':')) {
            outParams.insert(routeSeg.mid(1), requestSeg);
        } else if (routeSeg != requestSeg) {
            return false;
        }
    }

    return true;
}

void runChain(int index,
              std::shared_ptr<Context> ctx,
              const QList<std::shared_ptr<IMiddleware>> &middlewares,
              const Route::Handler &finalHandler)
{
    try {
        if (index >= middlewares.size()) {
            finalHandler(ctx);
            return;
        }

        middlewares.at(index)->process(ctx, [index, ctx, middlewares, finalHandler]() {
            runChain(index + 1, ctx, middlewares, finalHandler);
        });
    } catch (const std::exception &e) {
        ErrorHandler::handle(ctx, e);
    }
};

} // namespace

Router::Router()
    : mRoutes(std::make_shared<QList<Route>>())
{}

Route &Router::get(const QString &path, Route::Handler handler)
{
    return addRoute("GET", path, std::move(handler));
}

Route &Router::post(const QString &path, Route::Handler handler)
{
    return addRoute("POST", path, std::move(handler));
}

Route &Router::put(const QString &path, Route::Handler handler)
{
    return addRoute("PUT", path, std::move(handler));
}

Route &Router::patch(const QString &path, Route::Handler handler)
{
    return addRoute("PATCH", path, std::move(handler));
}

Route &Router::del(const QString &path, Route::Handler handler)
{
    return addRoute("DELETE", path, std::move(handler));
}

void Router::use(std::shared_ptr<IMiddleware> middleware)
{
    mMiddlewares.append(std::move(middleware));
}

void Router::setDefaultTimer(int ms)
{
    mDefaultTimeoutMs = ms;
}

Router Router::group(const QString &prefix)
{
    Router child;
    child.mRoutes = mRoutes;
    child.mPrefix = mPrefix + prefix;
    child.mMiddlewares = mMiddlewares;
    child.mDefaultTimeoutMs = mDefaultTimeoutMs;
    return child;
}

bool Router::dispatch(std::shared_ptr<Context> ctx, int serverDefaultTimeoutMs) const
{
    QList<QString> requestSegments = splitPath(ctx->path());
    QMap<QString, QString> requestParams;
    for (const Route &route : *mRoutes) {
        if (route.getMethod() != ctx->method())
            continue;

        requestParams.clear();
        if (!matchSegments(route.getSegments(), requestSegments, requestParams))
            continue;

        ctx->setPathParams(requestParams);

        int timeoutMs = route.resolveTimeout(serverDefaultTimeoutMs);
        if (timeoutMs != Route::NoTimeOut && timeoutMs > 0) {
            auto timer = std::make_shared<QTimer>();
            timer->setSingleShot(timeoutMs);

            QObject::connect(timer.get(), &QTimer::timeout, [ctx]() {
                if (!ctx->hasResponded()) {
                    ctx->error(503, "Request Timeout");
                }
            });

            ctx->onBeforeResponse([timer](Context &ctx) {
                QMetaObject::invokeMethod(timer.get(), [timer]() { timer->stop(); });
            });

            timer->start();
        }

        if (!route.getRunSync()) {
            auto middlewares = route.getMiddlewares();
            auto handler = route.getHandler();

            QThreadPool::globalInstance()->start(
                [ctx, middlewares, handler]() { runChain(0, ctx, middlewares, handler); });
        } else {
            runChain(0, ctx, route.getMiddlewares(), route.getHandler());
        }
        return true;
    }

    return false;
}

Route &Router::addRoute(const QString &method, const QString &path, Route::Handler handler)
{
    QString fullPath = mPrefix + path;
    // throw error if this route has been registered
    for (const Route &route : *mRoutes) {
        if (route.getMethod() == method && route.getPattern() == fullPath) {
            throw std::logic_error(QString("Router: duplicate route registered: %1 %2")
                                       .arg(method, fullPath)
                                       .toStdString());
        }
    }

    Route route;
    route.setMethod(method);
    route.setPattern(fullPath);
    route.setSegments(splitPath(fullPath));
    route.setHandler(std::move(handler));
    route.setMiddlewares(mMiddlewares);
    route.setGroupDefaultTimeoutMs(mDefaultTimeoutMs);

    mRoutes->append(std::move(route));
    return mRoutes->last();
}
