/**
 * Copyright (C) 2019, Hensoldt Cyber GmbH
 */

#pragma once

#include "SeosError.h"
#include "OS_ConfigService.h"

seos_err_t
initializeWithMemoryBackends(OS_ConfigServiceLib_t* configLib);
