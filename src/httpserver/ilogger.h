/******************************************************************************
 * @file        ilogger.h
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-13
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#ifndef ILOGGER_H
#define ILOGGER_H
#include <QString>

class ILogger
{
public:
    virtual ~ILogger() = default;

    virtual void debug(const QString &message) = 0;
    virtual void info(const QString &message) = 0;
    virtual void warning(const QString &message) = 0;
    virtual void error(const QString &message) = 0;
};

#endif // ILOGGER_H
