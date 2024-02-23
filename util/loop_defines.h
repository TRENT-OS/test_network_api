/*
 * Copyright (C) 2020-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

#define CONCAT_IMPL(X, Y) X##Y

#define MACRO_CONCAT(X, Y) CONCAT_IMPL(X, Y)

#define GEN_ID(X) MACRO_CONCAT(X, __INCLUDE_LEVEL__)

#define GEN_EMIT(X) MACRO_CONCAT(MACRO_CONCAT(X, __INCLUDE_LEVEL__), _emit)

#define GEN_WAIT(X) MACRO_CONCAT(MACRO_CONCAT(X, __INCLUDE_LEVEL__), _wait)

#define GEN_PORT(X) MACRO_CONCAT(MACRO_CONCAT(X, __INCLUDE_LEVEL__), _port)

#define GEN_NAME(X, Y) MACRO_CONCAT(MACRO_CONCAT(X, Y), __INCLUDE_LEVEL__)
