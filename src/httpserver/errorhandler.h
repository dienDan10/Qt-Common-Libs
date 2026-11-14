/******************************************************************************
 * @file        errorhandler.h
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-14
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H
#include "context.h"
#include "httpexception.h"

namespace ErrorHandler {

void handle(std::shared_ptr<Context> ctx, const std::exception &e)
{
    // xử lý lỗi tự throw có chủ đích (HttpException)
    if (const auto *httpEx = dynamic_cast<const HttpException *>(&e)) {
        if (!ctx->hasResponded()) {
            ctx->error(httpEx->statusCode(), httpEx->message());
        }
        return;
    }
    // xử lý lỗi app tự throw
    if (auto logger = ctx->logger()) {
        logger->error(QString("Unhandled exception for %1 %2: %3")
                          .arg(ctx->method(), ctx->path(), QString::fromUtf8(e.what())));
    }

    if (auto callback = ctx->errorCallback()) {
        callback(ctx, e);
    }

    if (!ctx->hasResponded()) {
        ctx->error(500, "Internal Server Error");
    }
}

} // namespace ErrorHandler

#endif // ERRORHANDLER_H
