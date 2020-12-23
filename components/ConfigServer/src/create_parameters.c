/**
 * Copyright (C) 2019, Hensoldt Cyber GmbH
 */

#include "system_config.h"

#include "lib_debug/Debug.h"
#include "create_parameters.h"

#include <string.h>
#include <stdio.h>

static
void
initializeName(
    char* buf, size_t bufSize,
    char const* name)
{
    memset(buf, 0, bufSize);
    strncpy(buf, name, bufSize - 1);
}


static
void
initializeDomain(
    OS_ConfigServiceLibTypes_Domain_t* domain,
    char const* name)
{
    initializeName(domain->name.name, OS_CONFIG_LIB_DOMAIN_NAME_LEN, name);
    domain->enumerator.index = 0;
}


OS_Error_t
initializeDomainsAndParameters(
    OS_ConfigServiceLib_t* configLib,
    char const* domainName)
{
    OS_Error_t result;

    if (strcmp(domainName, DOMAIN_NWSTACK) == 0)
    {
        // Initialize the domains
        Debug_LOG_DEBUG("initializing DOMAIN_NWSTACK");
        OS_ConfigServiceLibTypes_Domain_t domain;
        initializeDomain(&domain, domainName);
        result = OS_ConfigServiceBackend_writeRecord(
                     &configLib->domainBackend,
                     0,
                     &domain,
                     sizeof(domain));
        if (result != OS_SUCCESS)
        {
            return result;
        }

        // Initialize the parameters
        OS_ConfigServiceLibTypes_Parameter_t parameter;
        OS_ConfigServiceAccessRights_SetAll(&parameter.readAccess);
        OS_ConfigServiceAccessRights_SetAll(&parameter.writeAccess);

        parameter.parameterType = OS_CONFIG_LIB_PARAMETER_TYPE_STRING;
        initializeName(parameter.parameterName.name, OS_CONFIG_LIB_PARAMETER_NAME_LEN,
                       CFG_ETH_ADDR_CLIENT);

        char str[OS_CONFIG_LIB_PARAMETER_MAX_STRING_LENGTH];
        memset(str, 0, sizeof(str));
        strncpy(str, CFG_ETH_ADDR_CLIENT_VALUE, (sizeof(str) - 1));

        parameter.parameterValue.valueString.index = 0;
        parameter.parameterValue.valueString.size = strlen(str) + 1;
        parameter.domain.index = 0;
        result = OS_ConfigServiceBackend_writeRecord(
                     &configLib->parameterBackend,
                     0,
                     &parameter,
                     sizeof(parameter));
        if (result != OS_SUCCESS)
        {
            return result;
        }
        result = OS_ConfigServiceBackend_writeRecord(
                     &configLib->stringBackend,
                     parameter.parameterValue.valueString.index,
                     str,
                     sizeof(str));
        if (result != OS_SUCCESS)
        {
            return result;
        }

        parameter.parameterType = OS_CONFIG_LIB_PARAMETER_TYPE_STRING;
        initializeName(parameter.parameterName.name, OS_CONFIG_LIB_PARAMETER_NAME_LEN,
                       CFG_ETH_ADDR_SERVER);


        memset(str, 0, sizeof(str));
        strncpy(str, CFG_ETH_ADDR_SERVER_VALUE, (sizeof(str) - 1));

        parameter.parameterValue.valueString.index = 1;
        parameter.parameterValue.valueString.size = strlen(str) + 1;
        parameter.domain.index = 0;
        result = OS_ConfigServiceBackend_writeRecord(
                     &configLib->parameterBackend,
                     1,
                     &parameter,
                     sizeof(parameter));
        if (result != OS_SUCCESS)
        {
            return result;
        }
        result = OS_ConfigServiceBackend_writeRecord(
                     &configLib->stringBackend,
                     parameter.parameterValue.valueString.index,
                     str,
                     sizeof(str));
        if (result != OS_SUCCESS)
        {
            return result;
        }

        parameter.parameterType = OS_CONFIG_LIB_PARAMETER_TYPE_STRING;
        initializeName(parameter.parameterName.name, OS_CONFIG_LIB_PARAMETER_NAME_LEN,
                       CFG_ETH_GATEWAY_ADDR);

        memset(str, 0, sizeof(str));
        strncpy(str, CFG_ETH_GATEWAY_ADDR_VALUE, (sizeof(str) - 1));

        parameter.parameterValue.valueString.index = 2;
        parameter.parameterValue.valueString.size = strlen(str) + 1;
        parameter.domain.index = 0;
        result = OS_ConfigServiceBackend_writeRecord(
                     &configLib->parameterBackend,
                     2,
                     &parameter,
                     sizeof(parameter));
        if (result != OS_SUCCESS)
        {
            return result;
        }
        result = OS_ConfigServiceBackend_writeRecord(
                     &configLib->stringBackend,
                     parameter.parameterValue.valueString.index,
                     str,
                     sizeof(str));
        if (result != OS_SUCCESS)
        {
            return result;
        }


        parameter.parameterType = OS_CONFIG_LIB_PARAMETER_TYPE_STRING;
        initializeName(parameter.parameterName.name, OS_CONFIG_LIB_PARAMETER_NAME_LEN,
                       CFG_ETH_SUBNET_MASK);


        memset(str, 0, sizeof(str));
        strncpy(str, CFG_ETH_SUBNET_MASK_VALUE, (sizeof(str) - 1));

        parameter.parameterValue.valueString.index = 3;
        parameter.parameterValue.valueString.size = strlen(str) + 1;
        parameter.domain.index = 0;
        result = OS_ConfigServiceBackend_writeRecord(
                     &configLib->parameterBackend,
                     3,
                     &parameter,
                     sizeof(parameter));
        if (result != OS_SUCCESS)
        {
            return result;
        }
        result = OS_ConfigServiceBackend_writeRecord(
                     &configLib->stringBackend,
                     parameter.parameterValue.valueString.index,
                     str,
                     sizeof(str));
        if (result != OS_SUCCESS)
        {
            return result;
        }
    }

    return OS_SUCCESS;
}
