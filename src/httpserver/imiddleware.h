/******************************************************************************
 * @file        imiddleware.h
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-14
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#ifndef IMIDDLEWARE_H
#define IMIDDLEWARE_H
#include "context.h"
#include <functional>

using NextFunction = std::function<void()>;

class IMiddleware
{
public:
    virtual ~IMiddleware() = default;

    virtual void process(std::shared_ptr<Context> ctx, NextFunction next) = 0;
};

#endif // IMIDDLEWARE_H
