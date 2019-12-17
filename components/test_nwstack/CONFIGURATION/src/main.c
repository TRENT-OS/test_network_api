/*
   *  configuration server startup.
   *
   *  Copyright (C) 2019, Hensoldt Cyber GmbH
*/


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "seos.h"
#include "seos_config_server.h"

#include "init_lib_with_mem_backend.h"
#include "create_parameters.h"
#include "dump_config_lib.h"

bool isInitialized = false;

bool server_seos_configuration_IsUpAndRunning()
{
    return isInitialized;
}

int initializeLocalLibraryInstances()
{
    int result;

    // Initialize configuration library instance 0 with a memory backends.

    // Fetch the pointer to instance 0.
    SeosConfigInstanceStore* serverInstanceStore =
        server_seos_configuration_getInstances();
    SeosConfigLib* configLib =
        seos_configuration_instance_store_getInstance(serverInstanceStore, 0);

    // Create the backends in the instance.
    result = initializeWithMemoryBackends(configLib);
    if (result != SEOS_SUCCESS)
    {
        return result;
    }

    // Create the parameters in the instance.
    result = initializeDomainsAndParameters(configLib, "Domain-Network Stack");
    if (result != SEOS_SUCCESS)
    {
        return result;
    }

    printf("initializeLocalLibraryInstances ok\n");

    return SEOS_SUCCESS;
}


int run(void)
{
    printf("#########\n# start Network Config server #\n#########\n");

    // Initialize the configuration libraries.
    initializeLocalLibraryInstances();

    // Indicate the local instances are initialized.
    isInitialized = true;

    // The code below demonstrates how the server can access its local library instances.
    bool dumpLibrary = false;
    if (dumpLibrary)
    {
        // Create a fake remote handle to the first library.
        SeosConfigHandle handle;
        seos_configuration_handle_initRemoteHandle(
            0,
            &handle);

        // Dump this library.
        enumerateDomains(handle);
    }

    while (1)
        ;

    printf("#######\n# end #\n#######\n");

    return 0;
}
