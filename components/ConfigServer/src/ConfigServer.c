/*
   *  Main file of the ConfigServer component.
   *
   *  Copyright (C) 2020, Hensoldt Cyber GmbH
*/


#include "seos_system_config.h"

#include "LibDebug/Debug.h"
#include "OS_ConfigService.h"

#include <camkes.h>

#include "init_lib_with_mem_backend.h"
#include "create_parameters.h"


static bool
initializeConfigBackend(void)
{
    seos_err_t ret;

    OS_ConfigServiceInstanceStore_t* serverInstanceStore =
        OS_ConfigService_getInstances();
    OS_ConfigServiceLib_t* configLib =
        OS_ConfigServiceInstanceStore_getInstance(serverInstanceStore, 0);

    // Create the backends in the instance.
    Debug_LOG_INFO("ConfigServer: Initializing with mem backend...");
    ret = initializeWithMemoryBackends(configLib);
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("initializeWithMemoryBackends failed with: %d", ret);
        return false;
    }

    // Create the parameters in the instance.
    Debug_LOG_DEBUG("Enumerating %s", DOMAIN_NWSTACK);
    ret = initializeDomainsAndParameters(configLib, DOMAIN_NWSTACK);
    if (ret != SEOS_SUCCESS)
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
