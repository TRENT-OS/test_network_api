/**
 * Copyright (C) 2019, Hensoldt Cyber GmbH
 */

#pragma once

#include "OS_Error.h"
#include "OS_ConfigService.h"

OS_Error_t initializeDomainsAndParameters(
    OS_ConfigServiceLib_t *configLib,
    char const *domainName);
