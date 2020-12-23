/*
   *  Main file of the ConfigServer component.
   *
   *  Copyright (C) 2020, Hensoldt Cyber GmbH
*/


#include "system_config.h"

#include "lib_debug/Debug.h"
#include "OS_ConfigService.h"

#include <camkes.h>

#include "init_lib_with_mem_backend.h"
#include "create_parameters.h"


static bool
initializeConfigBackend(void)
{
    OS_Error_t ret;
    OS_ConfigServiceLib_t* configLib =
        OS_ConfigService_getInstance();

    // Create the backends in the instance.
    Debug_LOG_INFO("ConfigServer: Initializing with mem backend...");
    ret = initializeWithMemoryBackends(configLib);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("initializeWithMemoryBackends failed with: %d", ret);
        return false;
    }

    // Create the parameters in the instance.
    Debug_LOG_DEBUG("Enumerating %s", DOMAIN_NWSTACK);
    ret = initializeDomainsAndParameters(configLib, DOMAIN_NWSTACK);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("initializeDomainAndParameters for %s failed with: %d",
                        DOMAIN_NWSTACK, ret);
        return false;
    }

    return true;
}


void pre_init(void)
{
    Debug_LOG_INFO("Starting ConfigServer...");

    if (!initializeConfigBackend())
    {
        Debug_LOG_ERROR("initializeConfigBackend failed!");
    }

    Debug_LOG_INFO("Config Server initialized.");
}
