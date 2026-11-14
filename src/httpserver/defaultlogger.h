/******************************************************************************
 * @file        defaultlogger.h
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-15
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#ifndef DEFAULTLOGGER_H
#define DEFAULTLOGGER_H

#include <QDebug>
#include <QString>
#include "ilogger.h"
class DefaultLogger : public ILogger
{
public:
    void debug(const QString &message) override { qDebug().noquote() << "[DEBUG]" << message; }

    void info(const QString &message) override { qInfo().noquote() << "[INFO]" << message; }

    void warning(const QString &message) override { qWarning().noquote() << "[WARN]" << message; }

    void error(const QString &message) override { qCritical().noquote() << "[ERROR]" << message; }
};

#endif // DEFAULTLOGGER_H
