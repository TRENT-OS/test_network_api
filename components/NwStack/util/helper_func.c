/**
 * Collection of helper functions used by several component.
 *
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include <string.h>

#include "helper_func.h"

// -----------------------------------------------------------------------------
void initializeName(
    char* buf, size_t bufSize,
    char const* name)
{
    memset(buf, 0, bufSize);
    strncpy(buf, name, bufSize - 1);
}

//------------------------------------------------------------------------------
static
seos_err_t
compareDomainName(
    OS_ConfigServiceLibTypes_DomainName_t const* a,
    OS_ConfigServiceLibTypes_DomainName_t const* b)
{
    for (unsigned int k = 0; k < OS_CONFIG_LIB_DOMAIN_NAME_LEN; ++k)
    {
        if (a->name[k] != b->name[k])
        {
            Debug_LOG_DEBUG("Error: function: %s - line: %d\n", __FUNCTION__, __LINE__);
            return SEOS_ERROR_GENERIC;
        }
    }

    return SEOS_SUCCESS;
}

//------------------------------------------------------------------------------
seos_err_t
initializeDomainName(
    OS_ConfigServiceLibTypes_DomainName_t* domainName,
    char const* name)
{
    initializeName(domainName->name, OS_CONFIG_LIB_DOMAIN_NAME_LEN, name);

    return SEOS_SUCCESS;
}

seos_err_t
initializeParameterName(
    OS_ConfigServiceLibTypes_ParameterName_t* parameterName,
    char const* name)
{
    initializeName(parameterName->name, OS_CONFIG_LIB_PARAMETER_NAME_LEN, name);

    return SEOS_SUCCESS;
}

//------------------------------------------------------------------------------
static
seos_err_t
find_domain(
    OS_ConfigServiceHandle_t handle,
    OS_ConfigServiceLibTypes_DomainEnumerator_t* enumerator,
    OS_ConfigServiceLibTypes_DomainName_t const* domainName,
    OS_ConfigServiceLibTypes_Domain_t* domain)
{
    seos_err_t ret;

    OS_ConfigService_domainEnumeratorInit(handle, enumerator);
    for (;;)
    {
        ret = OS_ConfigService_domainEnumeratorGetElement(
                  handle,
                  enumerator,
                  domain);
        if (SEOS_SUCCESS != ret)
        {
            Debug_LOG_ERROR("OS_ConfigService_domainEnumeratorGetElement() failed, ret %d",
                            ret);
            return SEOS_ERROR_GENERIC;
        }

        OS_ConfigServiceLibTypes_DomainName_t domainNameTmp;
        OS_ConfigService_domainGetName(domain, &domainNameTmp);
        if (SEOS_SUCCESS == compareDomainName(&domainNameTmp, domainName))
        {
            // enumerator holds the right domain
            return SEOS_SUCCESS;
        }

        ret = OS_ConfigService_domainEnumeratorIncrement(handle, enumerator);
        if (SEOS_SUCCESS != ret)
        {
            Debug_LOG_ERROR("OS_ConfigService_domainEnumeratorIncrement() failed, ret %d",
                            ret);
            return SEOS_ERROR_GENERIC;
        }
    } // end for(;;)
}

//------------------------------------------------------------------------------
static
seos_err_t
get_parameter_enumerator(
    OS_ConfigServiceHandle_t handle,
    const char* DomainName,
    const char* ParameterName,
    OS_ConfigServiceLibTypes_ParameterEnumerator_t* parameterEnumerator)
{
    seos_err_t ret;

    OS_ConfigServiceLibTypes_DomainEnumerator_t domainEnumerator = {0};
    OS_ConfigServiceLibTypes_DomainName_t domainName;
    OS_ConfigServiceLibTypes_ParameterName_t parameterName;
    OS_ConfigServiceLibTypes_Domain_t domain = {0};

    initializeDomainName(&domainName, DomainName);

    initializeParameterName(&parameterName, ParameterName);

    ret = find_domain(handle, &domainEnumerator, &domainName, &domain);
    if (SEOS_SUCCESS != ret)
    {
        Debug_LOG_ERROR("find_domain() failed, ret %d", ret);
        return SEOS_ERROR_CONFIG_DOMAIN_NOT_FOUND;
    }

    ret = OS_ConfigService_domainEnumeratorGetElement(handle, &domainEnumerator,
                                                      &domain);
    if (SEOS_SUCCESS != ret)
    {
        Debug_LOG_ERROR("OS_ConfigServiceLibTypes_DomainEnumerator_tGetElement() failed, ret %d",
                        ret);
        return SEOS_ERROR_GENERIC;
    }

    ret = OS_ConfigService_domainCreateParameterEnumerator(handle, &domain,
                                                           &parameterName, parameterEnumerator);
    if (SEOS_SUCCESS != ret)
    {
        Debug_LOG_ERROR("OS_ConfigServiceLibTypes_Domain_tCreateParameterEnumerator() failed, ret %d",
                        ret);
        return SEOS_ERROR_GENERIC;
    }

    return SEOS_SUCCESS;
}

//------------------------------------------------------------------------------
static
seos_err_t
get_parameter_element(
    OS_ConfigServiceHandle_t handle,
    const char* DomainName,
    const char* ParameterName,
    OS_ConfigServiceLibTypes_DomainName_t* domainName,
    OS_ConfigServiceLibTypes_ParameterName_t* parameterName,
    OS_ConfigServiceLibTypes_Parameter_t* parameter)
{
    seos_err_t ret;
    OS_ConfigServiceLibTypes_Domain_t domain;
    OS_ConfigServiceLibTypes_DomainEnumerator_t domainEnumerator = {0};

    initializeDomainName(domainName, DomainName);
    initializeParameterName(parameterName, ParameterName);

    ret = find_domain(handle, &domainEnumerator, domainName, &domain);
    if (SEOS_SUCCESS != ret)
    {
        Debug_LOG_ERROR("find_domain() failed, ret %d", ret);
        return SEOS_ERROR_CONFIG_DOMAIN_NOT_FOUND;
    }

    ret = OS_ConfigService_domainGetElement(handle, &domain, parameterName,
                                            parameter);
    if (SEOS_SUCCESS != ret)
    {
        Debug_LOG_ERROR("domainGetElement() failed, ret %d", ret);
        return SEOS_ERROR_CONFIG_PARAMETER_NOT_FOUND;
    }

    return SEOS_SUCCESS;
}

//------------------------------------------------------------------------------
seos_err_t
helper_func_getConfigParameter(
    OS_ConfigServiceHandle_t* handle,
    const char* DomainName,
    const char* ParameterName,
    void* parameterBuffer,
    size_t parameterLength)
{
    seos_err_t ret;
    size_t bytesCopied;
    OS_ConfigServiceLibTypes_DomainName_t domainName;
    OS_ConfigServiceLibTypes_ParameterName_t parameterName;
    OS_ConfigServiceLibTypes_Parameter_t parameter;
    OS_ConfigServiceHandle_t configHandle = *handle;

    ret = get_parameter_element(configHandle, DomainName, ParameterName,
                                &domainName, &parameterName, &parameter);
    if (SEOS_SUCCESS != ret)
    {
        Debug_LOG_ERROR("get_parameter_element() failed, ret %d", ret);
        return ret;
    }

    OS_ConfigServiceLibTypes_ParameterType_t parameterType;
    OS_ConfigService_parameterGetType(&parameter, &parameterType);

    ret = OS_ConfigService_parameterGetValue(configHandle,
                                             &parameter,
                                             parameterBuffer,
                                             parameterLength,
                                             &bytesCopied);
    if (SEOS_SUCCESS != ret)
    {
        Debug_LOG_ERROR("parameterGetValue() failed, ret %d", ret);
        return ret;
    }

    return SEOS_SUCCESS;
}

//------------------------------------------------------------------------------
seos_err_t helper_func_setConfigParameter(
    OS_ConfigServiceHandle_t* handle,
    const char* DomainName,
    const char* ParameterName,
    const void* parameterValue,
    size_t parameterLength)
{
    seos_err_t ret;
    OS_ConfigServiceHandle_t configHandle = *handle;
    OS_ConfigServiceLibTypes_ParameterEnumerator_t parameterEnumerator = {0};
    OS_ConfigServiceLibTypes_DomainName_t domainName;
    OS_ConfigServiceLibTypes_ParameterName_t parameterName;
    OS_ConfigServiceLibTypes_Parameter_t parameter;

    ret = get_parameter_element(configHandle, DomainName, ParameterName,
                                &domainName, &parameterName, &parameter);
    if (SEOS_SUCCESS != ret)
    {
        Debug_LOG_ERROR("get_parameter_element() failed, ret %d", ret);
        return ret;
    }

    ret = get_parameter_enumerator(configHandle,
                                   DomainName,
                                   ParameterName,
                                   &parameterEnumerator);
    if (SEOS_SUCCESS != ret)
    {
        Debug_LOG_ERROR("get_parameter_enumerator() failed, ret %d", ret);
        return SEOS_ERROR_GENERIC;
    }

    OS_ConfigServiceLibTypes_ParameterType_t parameterType;
    OS_ConfigService_parameterGetType(&parameter, &parameterType);

    ret = OS_ConfigService_parameterSetValue(configHandle,
                                             &parameterEnumerator,
                                             parameterType,
                                             parameterValue,
                                             parameterLength);
    if (SEOS_SUCCESS != ret)
    {
        Debug_LOG_ERROR("OS_ConfigService_parameterSetValue() failed, ret %d", ret);
        return ret;
    }

    return SEOS_SUCCESS;
}