/**
 * Copyright (C) 2019, Hensoldt Cyber GmbH
 */

#include <string.h>
#include <stdio.h>

#include "SeosConfigBackend.h"
#include "SeosConfigLib.h"



static
void
initializeName(char* buf, size_t bufSize, char const* name)
{
    memset(buf, 0, bufSize);
    strncpy(buf, name, bufSize - 1);
}


static
void
initialzeDomain(SeosConfigLib_Domain* domain, char const* name)
{
    initializeName(domain->name.name, SEOS_CONFIG_LIB_DOMAIN_NAME_LEN, name);
    domain->enumerator.index = 0;
}


int
initializeDomainsAndParameters(SeosConfigLib* configLib, char const* domainName)
{
    int result;

    // Initialize the domains
    SeosConfigLib_Domain domain;
    initialzeDomain(&domain, domainName);
    result = SeosConfigBackend_writeRecord(
                 &configLib->domainBackend,
                 0,
                 &domain,
                 sizeof(domain));
    if (result != 0)
    {
        return result;
    }

    // Initialize the parameters
    SeosConfigLib_Parameter parameter;
    SeosConfigAccessRights_SetAll(&parameter.readAccess);
    SeosConfigAccessRights_SetAll(&parameter.writeAccess);


    char str[SEOS_CONFIG_LIB_PARAMETER_MAX_STRING_LENGTH];

    // Initialize the parameters
    parameter.parameterType = SEOS_CONFIG_LIB_PARAMETER_TYPE_STRING;
    initializeName(parameter.parameterName.name, SEOS_CONFIG_LIB_PARAMETER_NAME_LEN,
                   "SEOS_TAP0_ADDR");
    initializeName(str, sizeof(str), "192.168.82.91");
    parameter.parameterValue.valueString.index = 0;
    parameter.parameterValue.valueString.size = strlen(str) + 1;
    parameter.domain.index = 0;
    result = SeosConfigBackend_writeRecord(
                 &configLib->parameterBackend,
                 0,
                 &parameter,
                 sizeof(parameter));
    if (result != 0)
    {
        return result;
    }
    result = SeosConfigBackend_writeRecord(
                 &configLib->stringBackend,
                 0,
                 str,
                 sizeof(str));
    if (result != 0)
    {
        return result;
    }


    parameter.parameterType = SEOS_CONFIG_LIB_PARAMETER_TYPE_STRING;
    initializeName(parameter.parameterName.name, SEOS_CONFIG_LIB_PARAMETER_NAME_LEN,
                   "SEOS_TAP0_GATEWAY_ADDR");
    initializeName(str, sizeof(str), "192.168.82.1");
    parameter.parameterValue.valueString.index = 1;
    parameter.parameterValue.valueString.size = strlen(str) + 1;
    parameter.domain.index = 0;
    result = SeosConfigBackend_writeRecord(
                 &configLib->parameterBackend,
                 1,
                 &parameter,
                 sizeof(parameter));
    if (result != 0)
    {
        return result;
    }
    result = SeosConfigBackend_writeRecord(
                 &configLib->stringBackend,
                 1,
                 str,
                 sizeof(str));
    if (result != 0)
    {
        return result;
    }


    parameter.parameterType = SEOS_CONFIG_LIB_PARAMETER_TYPE_STRING;
    initializeName(parameter.parameterName.name, SEOS_CONFIG_LIB_PARAMETER_NAME_LEN,
                   "SEOS_TAP0_SUBNET_MASK");
    initializeName(str, sizeof(str), "255.255.255.0");
    parameter.parameterValue.valueString.index = 2;
    parameter.parameterValue.valueString.size = strlen(str) + 1;
    parameter.domain.index = 0;
    result = SeosConfigBackend_writeRecord(
                 &configLib->parameterBackend,
                 2,
                 &parameter,
                 sizeof(parameter));
    if (result != 0)
    {
        return result;
    }
    result = SeosConfigBackend_writeRecord(
                 &configLib->stringBackend,
                 2,
                 str,
                 sizeof(str));
    if (result != 0)
    {
        return result;
    }


    parameter.parameterType = SEOS_CONFIG_LIB_PARAMETER_TYPE_STRING;
    initializeName(parameter.parameterName.name, SEOS_CONFIG_LIB_PARAMETER_NAME_LEN,
                   "SEOS_TAP1_ADDR");
    initializeName(str, sizeof(str), "192.168.82.92");
    parameter.parameterValue.valueString.index = 3;
    parameter.parameterValue.valueString.size = strlen(str) + 1;
    parameter.domain.index = 0;
    result = SeosConfigBackend_writeRecord(
                 &configLib->parameterBackend,
                 3,
                 &parameter,
                 sizeof(parameter));
    if (result != 0)
    {
        return result;
    }
    result = SeosConfigBackend_writeRecord(
                 &configLib->stringBackend,
                 3,
                 str,
                 sizeof(str));
    if (result != 0)
    {
        return result;
    }


    parameter.parameterType = SEOS_CONFIG_LIB_PARAMETER_TYPE_STRING;
    initializeName(parameter.parameterName.name, SEOS_CONFIG_LIB_PARAMETER_NAME_LEN,
                   "SEOS_TAP1_GATEWAY_ADDR");
    initializeName(str, sizeof(str), "192.168.82.1");
    parameter.parameterValue.valueString.index = 4;
    parameter.parameterValue.valueString.size = strlen(str) + 1;
    parameter.domain.index = 0;
    result = SeosConfigBackend_writeRecord(
                 &configLib->parameterBackend,
                 4,
                 &parameter,
                 sizeof(parameter));
    if (result != 0)
    {
        return result;
    }
    result = SeosConfigBackend_writeRecord(
                 &configLib->stringBackend,
                 4,
                 str,
                 sizeof(str));
    if (result != 0)
    {
        return result;
    }


    parameter.parameterType = SEOS_CONFIG_LIB_PARAMETER_TYPE_STRING;
    initializeName(parameter.parameterName.name, SEOS_CONFIG_LIB_PARAMETER_NAME_LEN,
                   "SEOS_TAP1_SUBNET_MASK");
    initializeName(str, sizeof(str), "255.255.255.0");
    parameter.parameterValue.valueString.index = 5;
    parameter.parameterValue.valueString.size = strlen(str) + 1;
    parameter.domain.index = 0;
    result = SeosConfigBackend_writeRecord(
                 &configLib->parameterBackend,
                 5,
                 &parameter,
                 sizeof(parameter));
    if (result != 0)
    {
        return result;
    }
    result = SeosConfigBackend_writeRecord(
                 &configLib->stringBackend,
                 5,
                 str,
                 sizeof(str));
    if (result != 0)
    {
        return result;
    }

    return 0;
}
