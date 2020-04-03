/**
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#pragma once

#include "LibDebug/Debug.h"

#include "OS_ConfigService.h"

//------------------------------------------------------------------------------
seos_err_t
helper_func_getConfigParameter(
    OS_ConfigServiceHandle_t *handle,
    const char *DomainName,
    const char *ParameterName,
    void *parameterBuffer,
    size_t parameterLength);

seos_err_t
helper_func_setConfigParameter(
    OS_ConfigServiceHandle_t *handle,
    const char *DomainName,
    const char *ParameterName,
    const void *parameterValue,
    size_t parameterLength);