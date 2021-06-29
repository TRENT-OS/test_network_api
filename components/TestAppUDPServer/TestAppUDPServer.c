/*
 * TestAppUDPServer
 *
 * Copyright (C) 2019-2021, HENSOLDT Cyber GmbH
 *
 */

#include "OS_Error.h"
#include "lib_compiler/compiler.h"
#include "lib_debug/Debug.h"
#include "lib_macros/Test.h"
#include "stdint.h"
#include "system_config.h"
#include <string.h>

#include "OS_Network.h"
#include "math.h"

#include "OS_NetworkStackClient.h"
#include "SysLoggerClient.h"
#include "interfaces/if_OS_NetworkStack.h"
#include "util/loop_defines.h"
#include <camkes.h>

static const if_OS_NetworkStack_t network_stack =
    IF_OS_SOCKET_ASSIGN(networkStack_rpc, socket_1_port);

static OS_NetworkStackClient_SocketDataports_t config =
{
    .number_of_sockets = OS_NETWORK_MAXIMUM_SOCKET_NO
};

void
pre_init(void)
{
#if defined(Debug_Config_PRINT_TO_LOG_SERVER)
    DECL_UNUSED_VAR(OS_Error_t err) = SysLoggerClient_init(sysLogger_Rpc_log);
    Debug_ASSERT(err == OS_SUCCESS);
#endif
}

void
init_client_api()
{
    static OS_Dataport_t dataports[OS_NETWORK_MAXIMUM_SOCKET_NO] = { 0 };

    int i = 0;

#define LOOP_COUNT OS_NETWORK_MAXIMUM_SOCKET_NO
#define LOOP_ELEMENT                                                           \
    GEN_ID(OS_Dataport_t t) = OS_DATAPORT_ASSIGN(GEN_PORT(socket_));           \
    dataports[i]            = GEN_ID(t);                                       \
    i++;
#include "util/loop.h"

    config.dataport = dataports;
    OS_NetworkStackClient_init(&config);
}

void
test_udp_recvfrom_pos()
{
    TEST_START();

    // Buffer big enough to hold 2 frames, rounded to the nearest power of 2
    static char buffer[4096];
    size_t      len;

    OS_NetworkSocket_Handle_t handle;

    OS_Network_Socket_t udp_socket = { .domain = OS_AF_INET,
                                       .type   = OS_SOCK_DGRAM,
                                       .name   = DEV_ADDR,
                                       .port   = CFG_UDP_TEST_PORT
                                     };

    OS_Network_Socket_t receive_udp_socket = udp_socket;

    OS_Error_t err =
        OS_NetworkSocket_create(&network_stack, &udp_socket, &handle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("client_socket_create() failed, code %d", err);
        return;
    }

    err = OS_NetworkSocket_bind(handle, udp_socket.port);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("bind() failed, code %d", err);
        return;
    }

    len = sizeof(buffer);
    Debug_LOG_INFO("UDP Receive test handle: %d", handle);

    err = OS_NetworkSocket_recvfrom(
              handle,
              buffer,
              len,
              &len,
              &receive_udp_socket);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("recvfrom() failed, code %d", err);
        return;
    }

    Debug_LOG_INFO(
        "Received %d \"%*s\" : %s %d\n",
        len,
        len,
        buffer,
        receive_udp_socket.name,
        receive_udp_socket.port);

    err = OS_NetworkSocket_close(handle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("close() failed, code %d", err);
        return;
    }

    TEST_FINISH();
}

void
test_udp_sendto_pos()
{
    TEST_START();

    // Buffer big enough to hold 2 frames, rounded to the nearest power of 2
    static char buffer[4096];
    const char  test_message[] = "Hello there";
    size_t      len;

    OS_NetworkSocket_Handle_t handle;

    OS_Network_Socket_t udp_socket = { .domain = OS_AF_INET,
                                       .type   = OS_SOCK_DGRAM,
                                       .name   = DEV_ADDR,
                                       .port   = CFG_UDP_TEST_PORT
                                     };

    OS_Network_Socket_t receive_udp_socket = udp_socket;

    OS_Error_t err =
        OS_NetworkSocket_create(&network_stack, &udp_socket, &handle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("client_socket_create() failed, code %d", err);
        return;
    }

    err = OS_NetworkSocket_bind(handle, udp_socket.port);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("bind() failed, code %d", err);
        return;
    }

    len = sizeof(buffer);
    Debug_LOG_INFO("UDP Send test");
    err = OS_NetworkSocket_recvfrom(
              handle,
              buffer,
              len,
              &len,
              &receive_udp_socket);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("recvfrom() failed, code %d", err);
        return;
    }

    Debug_LOG_INFO(
        "Received %d \"%*s\" : %s %d\n",
        len,
        len,
        buffer,
        receive_udp_socket.name,
        receive_udp_socket.port);

    len = sizeof(test_message);
    err = OS_NetworkSocket_sendto(
              handle,
              test_message,
              len,
              &len,
              receive_udp_socket);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("client_socket_create() failed, code %d", err);
        return;
    }

    err = OS_NetworkSocket_close(handle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("close() failed, code %d", err);
        return;
    }

    TEST_FINISH();
}

void
test_udp_recvfrom_neg()
{
    // This test will try to create sockets with invalid parameters. After this
    // it will try to open a socket to a service which is not reachable (closed
    // port) and check that the create() fails
    TEST_START();

    OS_NetworkSocket_Handle_t handle;
    OS_NetworkSocket_Handle_t invalid_handle = OS_NetworkServer_Handle_INVALID;
    OS_Network_Socket_t       udp_socket     = { .domain = OS_AF_INET,
                                                 .type   = OS_SOCK_DGRAM,
                                                 .name   = DEV_ADDR,
                                                 .port   = CFG_UNREACHABLE_PORT
                                               };

    OS_Error_t err =
        OS_NetworkSocket_create(&network_stack, &udp_socket, &handle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkSocket_create() failed, code %d", err);
    }
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    const OS_Dataport_t dp  = config.dataport[handle.handleID];
    size_t              len = OS_Dataport_getSize(dp);

    if_OS_NetworkStack_t* vtable = (if_OS_NetworkStack_t*)handle.ctx;

    err = vtable->socket_recvfrom(invalid_handle.handleID, &len, &udp_socket);
    if (err != OS_ERROR_INVALID_HANDLE)
    {
        Debug_LOG_ERROR(
            "UDP recvfrom() with invalid handle failed, error %d",
            err);
    }
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_HANDLE, err);

    // creates a length guaranteed larger than that of the dataport, which won't
    // fit in the dataport and will generate an error case
    len = OS_Dataport_getSize(dp) + 1;

    err = vtable->socket_recvfrom(handle.handleID, &len, &udp_socket);
    if (err != OS_ERROR_INVALID_PARAMETER)
    {
        Debug_LOG_ERROR(
            "UDP recvfrom() with invalid dataport size failed, error %d",
            err);
    }
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_PARAMETER, err);

    err = OS_NetworkSocket_close(handle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("close() failed, code %d", err);
    }
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    TEST_FINISH();
}

void
test_udp_sendto_neg()
{
    // This test will try to create sockets with invalid parameters. After this
    // it will try to open a socket to a service which is not reachable (closed
    // port) and check that the create() fails
    TEST_START();

    OS_NetworkSocket_Handle_t handle;
    OS_NetworkSocket_Handle_t invalid_handle = OS_NetworkServer_Handle_INVALID;
    OS_Network_Socket_t       udp_socket     = { .domain = OS_AF_INET,
                                                 .type   = OS_SOCK_DGRAM,
                                                 .name   = DEV_ADDR,
                                                 .port   = 24242
                                               };

    OS_Error_t err =
        OS_NetworkSocket_create(&network_stack, &udp_socket, &handle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkSocket_create() failed, code %d", err);
    }
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    const OS_Dataport_t dp  = config.dataport[handle.handleID];
    size_t              len = OS_Dataport_getSize(dp);

    if_OS_NetworkStack_t* vtable = (if_OS_NetworkStack_t*)handle.ctx;

    err = vtable->socket_sendto(invalid_handle.handleID, &len, udp_socket);
    if (err != OS_ERROR_INVALID_HANDLE)
    {
        Debug_LOG_ERROR(
            "UDP sendto() with invalid handle failed, error %d",
            err);
    }
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_HANDLE, err);

    // creates a length guaranteed larger than that of the dataport, which won't
    // fit in the dataport and will generate an error case
    len = OS_Dataport_getSize(dp) + 1;

    err = vtable->socket_sendto(handle.handleID, &len, udp_socket);
    if (err != OS_ERROR_INVALID_PARAMETER)
    {
        Debug_LOG_ERROR(
            "UDP sendto() with invalid dataport size failed, error %d",
            err);
    }
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_PARAMETER, err);

    err = OS_NetworkSocket_close(handle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("close() failed, code %d", err);
    }
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    TEST_FINISH();
}

void
test_udp_echo()
{
    TEST_START();

    // Buffer big enough to hold 2 frames, rounded to the nearest power of 2
    static char buffer[4096];
    size_t      len;

    OS_NetworkSocket_Handle_t handle;

    OS_Network_Socket_t udp_socket = { .domain = OS_AF_INET,
                                       .type   = OS_SOCK_DGRAM,
                                       .name   = DEV_ADDR,
                                       .port   = CFG_UDP_TEST_PORT
                                     };

    OS_Network_Socket_t receive_udp_socket = udp_socket;

    OS_Error_t err =
        OS_NetworkSocket_create(&network_stack, &udp_socket, &handle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("client_socket_create() failed, code %d", err);
        return;
    }
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = OS_NetworkSocket_bind(handle, udp_socket.port);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("bind() failed, code %d", err);
        return;
    }
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    while (1)
    {
        len = sizeof(buffer);

        err = OS_NetworkSocket_recvfrom(
                  handle,
                  buffer,
                  len,
                  &len,
                  &receive_udp_socket);

        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("recvfrom() failed, code %d", err);
            return;
        }
        ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

        err = OS_NetworkSocket_sendto(
                  handle,
                  buffer,
                  len,
                  &len,
                  receive_udp_socket);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("client_socket_create() failed, code %d", err);
            return;
        }
        ASSERT_EQ_OS_ERR(OS_SUCCESS, err);
    }
    err = OS_NetworkSocket_close(handle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("close() failed, code %d", err);
        return;
    }
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    TEST_FINISH();
}

//------------------------------------------------------------------------------
int
run()
{
    init_client_api();
    Debug_LOG_INFO("Starting TestAppUDPServer %s...", get_instance_name());

    test_udp_recvfrom_pos();
    test_udp_sendto_pos();
    test_udp_recvfrom_neg();
    test_udp_sendto_neg();
    test_udp_echo();

    return 0;
}
