/**
 * Copyright (C) 2019, Hensoldt Cyber GmbH
 */

#pragma once

#include "SeosError.h"
#include "OS_ConfigService.h"

seos_err_t initializeDomainsAndParameters(
    OS_ConfigServiceLib_t *configLib,
    char const *domainName);
