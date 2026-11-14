/******************************************************************************
 * @file        route.cpp
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-14
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#include "route.h"

Route::Route() {}

Route &Route::timeout(int ms)
{
    mOwnTimeoutMs = ms;
    return *this;
}

Route &Route::disableTimeout()
{
    mOwnTimeoutMs = NoTimeOut;
    return *this;
}

int Route::resolveTimeout(int serverDefaultTimeout) const
{
    if (mOwnTimeoutMs)
        return mOwnTimeoutMs.value();
    if (mGroupDefaultTimeoutMs)
        return mGroupDefaultTimeoutMs.value();
    return serverDefaultTimeout;
}

Route &Route::use(std::shared_ptr<IMiddleware> middleware)
{
    mMiddlewares.append(std::move(middleware));
    return *this;
}

QString Route::getMethod() const
{
    return mMethod;
}

QString Route::getPattern() const
{
    return mPattern;
}

const QList<QString> &Route::getSegments() const
{
    return mSegments;
}

const Route::Handler &Route::getHandler() const
{
    return mHandler;
}

const QList<std::shared_ptr<IMiddleware> > &Route::getMiddlewares() const
{
    return mMiddlewares;
}

std::optional<int> Route::getOwnTimeoutMs() const
{
    return mOwnTimeoutMs;
}

std::optional<int> Route::getGroupDefaultTimeoutMs() const
{
    return mGroupDefaultTimeoutMs;
}

Route &Route::runSync()
{
    mRunInWorker = false;
    return *this;
}

void Route::setMethod(const QString &newMethod)
{
    mMethod = newMethod;
}

void Route::setPattern(const QString &newPattern)
{
    mPattern = newPattern;
}

void Route::setSegments(const QList<QString> &newSegments)
{
    mSegments = newSegments;
}

void Route::setHandler(const Handler &newHandler)
{
    mHandler = newHandler;
}

void Route::setMiddlewares(const QList<std::shared_ptr<IMiddleware> > &newMiddlewares)
{
    mMiddlewares = newMiddlewares;
}

void Route::setOwnTimeoutMs(std::optional<int> newOwnTimeoutMs)
{
    mOwnTimeoutMs = newOwnTimeoutMs;
}

void Route::setGroupDefaultTimeoutMs(std::optional<int> newGroupDefaultTimeoutMs)
{
    mGroupDefaultTimeoutMs = newGroupDefaultTimeoutMs;
}

const bool Route::getRunSync() const
{
    return !mRunInWorker;
}
