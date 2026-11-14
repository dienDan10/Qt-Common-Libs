/******************************************************************************
 * @file        router.h
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-14
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#ifndef ROUTER_H
#define ROUTER_H

#include <QList>
#include <QString>
#include "context.h"
#include "imiddleware.h"
#include "route.h"

class Router
{
public:
    Router();

    // route register method
    Route &get(const QString &path, Route::Handler handler);
    Route &post(const QString &path, Route::Handler handler);
    Route &put(const QString &path, Route::Handler handler);
    Route &patch(const QString &path, Route::Handler handler);
    Route &del(const QString &path, Route::Handler handler);

    // register middleware
    void use(std::shared_ptr<IMiddleware> middleware);

    // set timeout for group
    void setDefaultTimer(int ms);

    // create sub-router with the same prefix from this router
    Router group(const QString &prefix);

    // dispatch request to appropriate route
    bool dispatch(std::shared_ptr<Context> ctx, int serverDefaultTimeoutMs) const;

private:
    Route &addRoute(const QString &method, const QString &path, Route::Handler handler);

private:
    QString mPrefix;
    QList<std::shared_ptr<IMiddleware>> mMiddlewares;
    std::optional<int> mDefaultTimeoutMs;
    std::shared_ptr<QList<Route>> mRoutes;
};

#endif // ROUTER_H
