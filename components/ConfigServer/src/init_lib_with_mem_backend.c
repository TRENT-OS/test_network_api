/**
 * Copyright (C) 2019, Hensoldt Cyber GmbH
 */

#include <string.h>
#include <stdio.h>

#include "LibDebug/Debug.h"

#include "init_lib_with_mem_backend.h"


// Sizes of the buffers are defined to at least cover the total size of the
// parameters added to the backend
static char parameterBuf[12000];
static char domainBuf[2000];
static char stringBuf[2000];
static char blobBuf[12000];

static
OS_Error_t
formatMemoryBackends(OS_ConfigServiceLib_t* configLib)
{
    OS_Error_t result = 0;

    // Create the memory backends.
    result = OS_ConfigServiceBackend_createMemBackend(domainBuf, sizeof(domainBuf),
                                                      4,
                                                      sizeof(OS_ConfigServiceLibTypes_Domain_t));
    if (result != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("createMemBackend failed with: %d", result);
        return result;
    }

    result = OS_ConfigServiceBackend_createMemBackend(parameterBuf,
                                                      sizeof(parameterBuf),
                                                      64, sizeof(OS_ConfigServiceLibTypes_Parameter_t));
    if (result != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("createMemBackend failed with: %d", result);
        return result;
    }

    result = OS_ConfigServiceBackend_createMemBackend(
                 stringBuf,
                 sizeof(stringBuf),
                 16,
                 OS_CONFIG_LIB_PARAMETER_MAX_STRING_LENGTH);
    if (result != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("createMemBackend failed with: %d", result);
        return result;
    }

    result = OS_ConfigServiceBackend_createMemBackend(
                 blobBuf,
                 sizeof(blobBuf),
                 144,
                 OS_CONFIG_LIB_PARAMETER_MAX_BLOB_BLOCK_LENGTH);
    if (result != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("createMemBackend failed with: %d", result);
        return result;
    }
    Debug_LOG_DEBUG("Memory backends formatted");
    return SEOS_SUCCESS;
}

OS_Error_t
initializeWithMemoryBackends(OS_ConfigServiceLib_t* configLib)
{
    OS_Error_t result = 0;
    OS_ConfigServiceBackend_t parameterBackend;
    OS_ConfigServiceBackend_t domainBackend;
    OS_ConfigServiceBackend_t stringBackend;
    OS_ConfigServiceBackend_t blobBackend;

    // Create the memory backends.
    result = formatMemoryBackends(configLib);
    if (result != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("formatMemoryBackends failed with: %d", result);
        return result;
    }

    // Initialize the backends in the config library object.
    result = OS_ConfigServiceBackend_initializeMemBackend(&domainBackend, domainBuf,
                                                          sizeof(domainBuf));
    if (result != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("initializeMemBackend failed with: %d", result);
        return result;
    }

    result = OS_ConfigServiceBackend_initializeMemBackend(&parameterBackend,
                                                          parameterBuf,
                                                          sizeof(parameterBuf));
    if (result != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("initializeMemBackend failed with: %d", result);
        return result;
    }

    result = OS_ConfigServiceBackend_initializeMemBackend(&stringBackend, stringBuf,
                                                          sizeof(stringBuf));
    if (result != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("initializeMemBackend failed with: %d", result);
        return result;
    }

    result = OS_ConfigServiceBackend_initializeMemBackend(&blobBackend, blobBuf,
                                                          sizeof(blobBuf));
    if (result != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("initializeMemBackend failed with: %d", result);
        return result;
    }

    result = OS_ConfigServiceLib_Init(
                 configLib,
                 &parameterBackend,
                 &domainBackend,
                 &stringBackend,
                 &blobBackend);
    if (result != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigServiceLib_Init failed with: %d", result);
        return result;
    }

    return result;
}
