/******************************************************************************
 * @file        httpresponse.h
 * @brief
 *
 * @author      diendanx
 * @date        2026-10-27
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <QByteArray>
#include <QList>
#include <QNetworkReply>
#include <QUrl>

struct HttpResponse
{
    int statusCode = 0;
    QByteArray body;
    QList<QNetworkReply::RawHeaderPair> headers;
    QNetworkReply::NetworkError error = QNetworkReply::NoError;
    QString errorString;
    QUrl url;

    bool isSuccess() const;

    // Lỗi tầng mạng: timeout, DNS, không có kết nối,...
    bool isNetworkError() const;

    QString text() const;

    QJsonDocument json() const;

    // lấy header không phân biệt hoa thường
    QByteArray header(const QByteArray &name) const;
};

#endif // HTTPRESPONSE_H
