/******************************************************************************
 * @file        httpresponse.cpp
 * @brief
 *
 * @author      diendanx
 * @date        2026-10-27
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#include "httpresponse.h"
#include <QJsonDocument>

bool HttpResponse::isSuccess() const
{
    return statusCode >= 200 && statusCode < 300;
}

bool HttpResponse::isNetworkError() const
{
    return statusCode == 0;
}

QString HttpResponse::text() const
{
    return QString::fromUtf8(body);
}

QJsonDocument HttpResponse::json() const
{
    return QJsonDocument::fromJson(body);
}

QByteArray HttpResponse::header(const QByteArray &name) const
{
    for (const auto &pair : headers) {
        if (pair.first.compare(name, Qt::CaseInsensitive))
            return pair.second;
    }

    return {};
}
