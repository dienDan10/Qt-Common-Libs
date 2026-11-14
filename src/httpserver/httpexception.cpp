/******************************************************************************
 * @file        httpexception.cpp
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-13
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#include "httpexception.h"

HttpException::HttpException(int statusCode, QString message)
    : mStatusCode(statusCode)
    , mMessage(std::move(message))
    , mWhatCache(mMessage.toStdString())
{}

int HttpException::statusCode() const
{
    return mStatusCode;
}

QString HttpException::message() const
{
    return mMessage;
}

const char *HttpException::what() const noexcept
{
    return mWhatCache.c_str();
}
