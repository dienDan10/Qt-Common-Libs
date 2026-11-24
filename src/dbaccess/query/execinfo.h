/******************************************************************************
 * @file        execinfo.h
 * @brief
 *
 * @author      diendanx
 * @date        2026-11-24
 *
 * @copyright   Copyright (c) 2026 diendanx.
 *              All rights reserved.
 ******************************************************************************/
#ifndef EXECINFO_H
#define EXECINFO_H
#include <QVariant>

namespace QtCommon::dbaccess {

struct ExecInfo
{
    int rowsAffected = -1; // -1 = driver không xác định được (theo quy ước của Qt)
    QVariant lastInsertId; // không hợp lệ nếu driver không hỗ trợ
};

} // namespace QtCommon::dbaccess

#endif // EXECINFO_H
