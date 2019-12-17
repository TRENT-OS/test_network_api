/**
 * Copyright (C) 2019, Hensoldt Cyber GmbH
 */

#include <string.h>

#include "SeosConfigBackend.h"
#include "SeosConfigLib.h"

#define PARAMETER_BUF_SIZE 2000*5
#define DOMAIN_BUF_SIZE 2000
#define STRING_BUF_SIZE 2000*5
#define BLOB_BUF_SIZE 2000

char parameterBuf[PARAMETER_BUF_SIZE];
char domainBuf[DOMAIN_BUF_SIZE];
char stringBuf[STRING_BUF_SIZE];
char blobBuf[BLOB_BUF_SIZE];

static
seos_err_t
formatMemoryBackends(SeosConfigLib* configLib)
{
    seos_err_t result = 0;

    // Create the memory backends.
    result = SeosConfigBackend_createMemBackend(domainBuf, sizeof(domainBuf), 1,
                                                sizeof(SeosConfigLib_Domain));
    if (result != SEOS_SUCCESS)
    {
        return result;
    }

    result = SeosConfigBackend_createMemBackend(parameterBuf, sizeof(parameterBuf),
                                                6, sizeof(SeosConfigLib_Parameter));
    if (result != SEOS_SUCCESS)
    {
        return result;
    }

    result = SeosConfigBackend_createMemBackend(
                 stringBuf,
                 sizeof(stringBuf),
                 6,
                 SEOS_CONFIG_LIB_PARAMETER_MAX_STRING_LENGTH);
    if (result != SEOS_SUCCESS)
    {
        return result;
    }

    result = SeosConfigBackend_createMemBackend(
                 blobBuf,
                 sizeof(blobBuf),
                 1,
                 SEOS_CONFIG_LIB_PARAMETER_MAX_BLOB_BLOCK_LENGTH);
    if (result != SEOS_SUCCESS)
    {
        return result;
    }

    return SEOS_SUCCESS;
}

seos_err_t
initializeWithMemoryBackends(SeosConfigLib* configLib)
{
    seos_err_t result = 0;
    SeosConfigBackend parameterBackend;
    SeosConfigBackend domainBackend;
    SeosConfigBackend stringBackend;
    SeosConfigBackend blobBackend;

    // Create the memory backends.
    result = formatMemoryBackends(configLib);
    if (result != SEOS_SUCCESS)
    {
        return result;
    }

    // Initialize the backends in the config library object.
    result = SeosConfigBackend_initializeMemBackend(&domainBackend, domainBuf,
                                                    sizeof(domainBuf));
    if (result != SEOS_SUCCESS)
    {
        return result;
    }

    result = SeosConfigBackend_initializeMemBackend(&parameterBackend, parameterBuf,
                                                    sizeof(parameterBuf));
    if (result != SEOS_SUCCESS)
    {
        return result;
    }

    result = SeosConfigBackend_initializeMemBackend(&stringBackend, stringBuf,
                                                    sizeof(stringBuf));
    if (result != SEOS_SUCCESS)
    {
        return result;
    }

    result = SeosConfigBackend_initializeMemBackend(&blobBackend, blobBuf,
                                                    sizeof(blobBuf));
    if (result != SEOS_SUCCESS)
    {
        return result;
    }

    result = SeosConfigLib_Init(
                 configLib,
                 &parameterBackend,
                 &domainBackend,
                 &stringBackend,
                 &blobBackend);
    if (result != SEOS_SUCCESS)
    {
        return result;
    }

    return result;
}

