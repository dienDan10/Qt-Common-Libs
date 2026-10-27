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

HttpClient::HttpClient(QNetworkAccessManager *manager, QObject *parent)
    : QObject(parent)
    , mManager(manager)
{}

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
    if (mDefaultHeaders.contains(name)) {
        mDefaultHeaders.remove(name);
    }
}

void HttpClient::setTimeout(std::chrono::milliseconds timeout)
{
    mTimeout = timeout;
}

std::chrono::milliseconds HttpClient::getTimeout() const
{
    return mTimeout;
}

HttpClient::Headers HttpClient::getDefaultHeaders() const
{
    return mDefaultHeaders;
}
