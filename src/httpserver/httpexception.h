/******************************************************************************
 * @file        httpexception.h
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-13
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#ifndef HTTPEXCEPTION_H
#define HTTPEXCEPTION_H

#include <QString>
#include <exception>

class HttpException : public std::exception
{
public:
    HttpException(int statusCode, QString message);
    int statusCode() const;
    QString message() const;
    const char *what() const noexcept override;

private:
    int mStatusCode;
    QString mMessage;
    std::string mWhatCache;
};

class BadRequestException : public HttpException
{
public:
    explicit BadRequestException(QString message = "Bad Request")
        : HttpException(400, std::move(message))
    {}
};

class UnauthorizedException : public HttpException
{
public:
    explicit UnauthorizedException(QString message = "Unauthorized")
        : HttpException(401, std::move(message))
    {}
};

class ForbiddenException : public HttpException
{
public:
    explicit ForbiddenException(QString message = "Forbidden")
        : HttpException(403, std::move(message))
    {}
};

class NotFoundException : public HttpException
{
public:
    explicit NotFoundException(QString message = "Not Found")
        : HttpException(404, std::move(message))
    {}
};

class ConflictException : public HttpException
{
public:
    explicit ConflictException(QString message = "Conflict")
        : HttpException(409, std::move(message))
    {}
};

class UnprocessableEntityException : public HttpException
{
public:
    explicit UnprocessableEntityException(QString message = "Unprocessable Entity")
        : HttpException(422, std::move(message))
    {}
};

class TooManyRequestsException : public HttpException
{
public:
    explicit TooManyRequestsException(QString message = "Too Many Requests")
        : HttpException(429, std::move(message))
    {}
};

#endif // HTTPEXCEPTION_H
