/******************************************************************************
 * @file        route.h
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-14
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#ifndef ROUTE_H
#define ROUTE_H
#include <QList>
#include <QString>
#include "context.h"
#include "imiddleware.h"
#include <functional>

class Route
{
public:
    Route();
    using Handler = std::function<void(std::shared_ptr<Context>)>;
    static constexpr int NoTimeOut = -1;

    Route &timeout(int ms);
    Route &disableTimeout();
    int resolveTimeout(int serverDefaultTimeout) const;

    Route &use(std::shared_ptr<IMiddleware> middleware);

    // getter and setter
    QString getMethod() const;
    QString getPattern() const;
    const QList<QString> &getSegments() const;
    const Handler &getHandler() const;
    const QList<std::shared_ptr<IMiddleware>> &getMiddlewares() const;
    std::optional<int> getOwnTimeoutMs() const;
    std::optional<int> getGroupDefaultTimeoutMs() const;
    Route &runSync();

    void setMethod(const QString &newMethod);
    void setPattern(const QString &newPattern);
    void setSegments(const QList<QString> &newSegments);
    void setHandler(const Handler &newHandler);
    void setMiddlewares(const QList<std::shared_ptr<IMiddleware>> &newMiddlewares);
    void setOwnTimeoutMs(std::optional<int> newOwnTimeoutMs);
    void setGroupDefaultTimeoutMs(std::optional<int> newGroupDefaultTimeoutMs);
    const bool getRunSync() const;

private:
    QString mMethod;  // method of this route
    QString mPattern; // path of this route
    QList<QString> mSegments; // parts of the request path, use to compare and extract params
    Handler mHandler;
    QList<std::shared_ptr<IMiddleware>> mMiddlewares;
    std::optional<int> mOwnTimeoutMs;
    std::optional<int> mGroupDefaultTimeoutMs;
    bool mRunInWorker = true;
};

#endif // ROUTE_H
