/**
 * Copyright (C) 2019, Hensoldt Cyber GmbH
 */

#include <string.h>
#include <stdio.h>

#include "seos_config_client.h"


static
void
dumpFrame(const uint8_t* buff, size_t len)
{
    size_t i;
    for (i = 0; i < len; i++)
    {
        printf("%3u %02X", buff[i], buff[i]);
        if (buff[i] >= 0x20 && buff[i] < 127)
        {
            printf(" %c", buff[i]);
        }
        else
        {
            printf(" \033[31m.\033[0m");
        }
        printf("\n");
    }
    printf("--- end of frame ---\n\n");
}


static
void
dumpParamter(SeosConfigLib_Parameter const* parameter,
             SeosConfigHandle configurationLib)
{
    SeosConfigLib_ParameterType parameterType;
    char* parameterTypeName;

    seos_configuration_parameterGetType(parameter, &parameterType);

    if (parameterType == SEOS_CONFIG_LIB_PARAMETER_TYPE_INTEGER32)
    {
        parameterTypeName = "SEOS_CONFIG_LIB_PARAMETER_TYPE_INTEGER32";
    }
    else if (parameterType == SEOS_CONFIG_LIB_PARAMETER_TYPE_INTEGER64)
    {
        parameterTypeName = "SEOS_CONFIG_LIB_PARAMETER_TYPE_INTEGER64";
    }
    else if (parameterType == SEOS_CONFIG_LIB_PARAMETER_TYPE_STRING)
    {
        parameterTypeName = "SEOS_CONFIG_LIB_PARAMETER_TYPE_STRING";
    }
    else if (parameterType == SEOS_CONFIG_LIB_PARAMETER_TYPE_BLOB)
    {
        parameterTypeName = "SEOS_CONFIG_LIB_PARAMETER_TYPE_BLOB";
    }
    else
    {
        parameterTypeName = "unknown";
    }

    printf("parameterType: %s\n", parameterTypeName);


    SeosConfigLib_ParameterName parameterName;
    seos_configuration_parameterGetName(parameter, &parameterName);
    printf("parameterName: %s\n", parameter->parameterName.name);

    // We are test code: ok to access private members.
    printf("parameterDomain: %u\n", parameter->domain.index);


    if (parameter->parameterType == SEOS_CONFIG_LIB_PARAMETER_TYPE_INTEGER32)
    {
        uint32_t value;
        int intResult = seos_configuration_parameterGetValueAsU32(configurationLib,
                                                                  parameter, &value);

        if (0 == intResult)
        {
            printf("parameterValue: %x\n", value);
        }
        else
        {
            printf("parameterValue: error: %d\n", intResult);
        }
    }
    else if (parameter->parameterType == SEOS_CONFIG_LIB_PARAMETER_TYPE_INTEGER64)
    {
        uint64_t value;
        int intResult = seos_configuration_parameterGetValueAsU64(configurationLib,
                                                                  parameter, &value);

        if (0 == intResult)
        {
            printf("parameterValue: %llx\n", value);
        }
        else
        {
            printf("parameterValue: error: %d\n", intResult);
        }
    }
    else if (parameter->parameterType == SEOS_CONFIG_LIB_PARAMETER_TYPE_STRING)
    {
        char buf[SEOS_CONFIG_LIB_PARAMETER_MAX_STRING_LENGTH];
        size_t length = seos_configuration_parameterGetSize(parameter);

        int intResult = seos_configuration_parameterGetValueAsString(
                            configurationLib,
                            parameter,
                            buf,
                            SEOS_CONFIG_LIB_PARAMETER_MAX_STRING_LENGTH);

        if (0 == intResult)
        {
            printf("parameterValue length: %d\n", length);
            dumpFrame((unsigned char*)buf, length);
        }
        else
        {
            printf("parameterValue: error: %d\n", intResult);
        }
    }
    else if (parameter->parameterType == SEOS_CONFIG_LIB_PARAMETER_TYPE_BLOB)
    {
        char buf[SEOS_CONFIG_LIB_PARAMETER_MAX_BLOB_BLOCK_LENGTH];
        size_t length = seos_configuration_parameterGetSize(parameter);

        int intResult = seos_configuration_parameterGetValueAsBlob(
                            configurationLib,
                            parameter,
                            buf,
                            SEOS_CONFIG_LIB_PARAMETER_MAX_BLOB_BLOCK_LENGTH);

        if (0 == intResult)
        {
            printf("parameterValue length: %d\n", length);
            dumpFrame((unsigned char*)buf, length);
        }
        else
        {
            printf("parameterValue: error: %d\n", intResult);
        }
    }
    else
    {
        printf("parameterValue: unknown type\n");
    }
}


static
void
enumerateDomainParameters(SeosConfigHandle configLib,
                          SeosConfigLib_DomainEnumerator const* domainEnumerator)
{
    SeosConfigLib_ParameterEnumerator enumerator;
    int intResult;

    intResult = seos_configuration_parameterEnumeratorInit(
                    configLib,
                    domainEnumerator,
                    &enumerator);

    if (0 != intResult)
    {
        return;
    }

    printf("enumerating domain parameters\n");

    do
    {
        SeosConfigLib_Parameter parameter;

        memset(&parameter, 0, sizeof(parameter));

        intResult = seos_configuration_parameterEnumeratorGetElement(configLib,
                    &enumerator,
                    &parameter);
        if (0 == intResult)
        {
            dumpParamter(&parameter, configLib);

            intResult = seos_configuration_parameterEnumeratorIncrement(configLib,
                                                                        &enumerator);
        }
        else
        {
            printf("Error from SeosConfigLib_parameterEnumeratorGetElement: %d\n",
                   intResult);
        }
    }
    while (0 == intResult);
}


void
enumerateDomains(SeosConfigHandle configLib)
{
    SeosConfigLib_DomainEnumerator enumerator;
    SeosConfigLib_Domain domain;

    seos_configuration_domainEnumeratorInit(configLib, &enumerator);

    printf("enumerating domains\n");

    int intResult;
    do
    {
        intResult = seos_configuration_domainEnumeratorGetElement(configLib,
                                                                  &enumerator,
                                                                  &domain);
        if (0 == intResult)
        {
            SeosConfigLib_DomainName domainName;

            seos_configuration_domainGetName(&domain, &domainName);
            printf("domain name: %s\n", domainName.name);

            enumerateDomainParameters(configLib, &enumerator);

            intResult = seos_configuration_domainEnumeratorIncrement(configLib,
                                                                     &enumerator);
        }
        else
        {
        }
    }
    while (0 == intResult);
}
