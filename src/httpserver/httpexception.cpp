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
    : m_statusCode(statusCode)
    , m_message(std::move(message))
{}

int HttpException::statusCode() const
{
    return m_statusCode;
}

QString HttpException::message() const
{
    return m_message;
}

const char *HttpException::what() const noexcept
{
    m_whatCache = m_message.toStdString();
    return m_whatCache.c_str();
}
