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

#include "SysLoggerClient.h"
#include "interfaces/if_OS_Socket.h"
#include "util/loop_defines.h"
#include "util/non_blocking_helper.h"
#include <camkes.h>

static const if_OS_Socket_t network_stack =
    IF_OS_SOCKET_ASSIGN(networkStack);

void
pre_init(void)
{
    OS_Error_t err;
#if defined(Debug_Config_PRINT_TO_LOG_SERVER)
    err = SysLoggerClient_init(sysLogger_Rpc_log);
    Debug_ASSERT(err == OS_SUCCESS);
#endif
    // Initialize the helper lib with the required synchronization mechanisms.
    nb_helper_init(
        event_received_send_ready_emit,
        event_received_recv_ready_wait,
        SharedResourceMutex_lock,
        SharedResourceMutex_unlock);

    // Set up callback for new received socket events.
    int ret = networkStack_event_notify_reg_callback(
                  &nb_helper_collect_pending_ev_handler,
                  (void*) &network_stack);
    if (ret < 0)
    {
        Debug_LOG_ERROR(
            "networkStack_event_notify_reg_callback() failed, code %d", err);
    }

    err = nb_helper_wait_for_network_stack_init(&network_stack);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("nb_helper_wait_for_network_stack_init() failed, code %d", err);
    }
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);
}

void
test_udp_recvfrom_pos()
{
    TEST_START();

    OS_NetworkSocket_Handle_t handle;

    OS_Error_t err = OS_NetworkSocket_create(
                         &network_stack,
                         &handle,
                         OS_AF_INET,
                         OS_SOCK_DGRAM);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkSocket_create() failed, code %d", err);
        return;
    }

    const OS_NetworkSocket_Addr_t dstAddr =
    {
        .addr = OS_INADDR_ANY_STR,
        .port = CFG_UDP_TEST_PORT
    };

    err = OS_NetworkSocket_bind(handle, &dstAddr);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkSocket_bind() failed, code %d", err);

        err = OS_NetworkSocket_close(handle);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_NetworkSocket_close() failed, code %d", err);
        }
        err = nb_helper_reset_ev_struct_for_socket(handle);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("nb_helper_reset_ev_struct_for_socket() failed, code %d", err);
        }

        return;
    }

    Debug_LOG_INFO("UDP Receive test handle: %d", handle);

    // Buffer big enough to hold 2 frames, rounded to the nearest power of 2
    static char buffer[4096];
    size_t len = sizeof(buffer);

    OS_NetworkSocket_Addr_t srcAddr = {0};

    // Wait until we get a read event for the bound socket.
    nb_helper_wait_for_read_ev_on_socket(handle);

    // Try to read some data.
    err = OS_NetworkSocket_recvfrom(
              handle,
              buffer,
              len,
              &len,
              &srcAddr);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkSocket_recvfrom() failed, code %d", err);

        err = OS_NetworkSocket_close(handle);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_NetworkSocket_close() failed, code %d", err);
        }
        err = nb_helper_reset_ev_struct_for_socket(handle);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("nb_helper_reset_ev_struct_for_socket() failed, code %d", err);
        }

        return;
    }

    Debug_LOG_INFO(
        "Received %d \"%*s\" : %s %d",
        len,
        len,
        buffer,
        srcAddr.addr,
        srcAddr.port);

    err = OS_NetworkSocket_close(handle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkSocket_close() failed, code %d", err);
        return;
    }

    err = nb_helper_reset_ev_struct_for_socket(handle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("nb_helper_reset_ev_struct_for_socket() failed, code %d", err);
        return;
    }

    TEST_FINISH();
}

void
test_udp_sendto_pos()
{
    TEST_START();

    OS_NetworkSocket_Handle_t handle;

    OS_Error_t err = OS_NetworkSocket_create(
                         &network_stack,
                         &handle,
                         OS_AF_INET,
                         OS_SOCK_DGRAM);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkSocket_create() failed, code %d", err);
        return;
    }

    const OS_NetworkSocket_Addr_t dstAddr =
    {
        .addr = OS_INADDR_ANY_STR,
        .port = CFG_UDP_TEST_PORT
    };

    err = OS_NetworkSocket_bind(handle, &dstAddr);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkSocket_bind() failed, code %d", err);

        err = OS_NetworkSocket_close(handle);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_NetworkSocket_close() failed, code %d", err);
        }
        err = nb_helper_reset_ev_struct_for_socket(handle);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("nb_helper_reset_ev_struct_for_socket() failed, code %d", err);
        }

        return;
    }

    // Buffer big enough to hold 2 frames, rounded to the nearest power of 2
    static char buffer[4096] = {0};
    size_t len = sizeof(buffer);

    OS_NetworkSocket_Addr_t srcAddr = {0};

    Debug_LOG_INFO("UDP Send test");

    // Wait until we get a read event for the bound socket.
    nb_helper_wait_for_read_ev_on_socket(handle);

    // Try to read some data.
    err = OS_NetworkSocket_recvfrom(
              handle,
              buffer,
              len,
              &len,
              &srcAddr);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkSocket_recvfrom() failed, code %d", err);

        err = OS_NetworkSocket_close(handle);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_NetworkSocket_close() failed, code %d", err);
        }
        err = nb_helper_reset_ev_struct_for_socket(handle);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("nb_helper_reset_ev_struct_for_socket() failed, code %d", err);
        }

        return;
    }

    Debug_LOG_INFO(
        "Received %d \"%*s\" : %s %d",
        len,
        len,
        buffer,
        srcAddr.addr,
        srcAddr.port);

    const char test_message[] = "Hello there";
    len = sizeof(test_message);
    err = OS_NetworkSocket_sendto(
              handle,
              test_message,
              len,
              &len,
              &srcAddr);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkSocket_sendto() failed, code %d", err);

        err = OS_NetworkSocket_close(handle);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_NetworkSocket_close() failed, code %d", err);
        }
        err = nb_helper_reset_ev_struct_for_socket(handle);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("nb_helper_reset_ev_struct_for_socket() failed, code %d", err);
        }

        return;
    }

    err = OS_NetworkSocket_close(handle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkSocket_close() failed, code %d", err);
        return;
    }
    err = nb_helper_reset_ev_struct_for_socket(handle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("nb_helper_reset_ev_struct_for_socket() failed, code %d", err);
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
    OS_NetworkSocket_Handle_t invalid_handle = OS_NetworkSocket_Handle_INVALID;

    OS_Error_t err = OS_NetworkSocket_create(
                         &network_stack,
                         &handle,
                         OS_AF_INET,
                         OS_SOCK_DGRAM);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkSocket_create() failed, code %d", err);
    }
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    size_t len = OS_Dataport_getSize(handle.ctx.dataport);

    OS_NetworkSocket_Addr_t srcAddr = {0};

    err = handle.ctx.socket_recvfrom(
              invalid_handle.handleID,
              &len,
              &srcAddr);
    if (err != OS_ERROR_INVALID_HANDLE)
    {
        Debug_LOG_ERROR(
            "UDP recvfrom() with invalid handle failed, error %d",
            err);
    }
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_HANDLE, err);

    // creates a length guaranteed larger than that of the dataport, which won't
    // fit in the dataport and will generate an error case
    len = OS_Dataport_getSize(handle.ctx.dataport) + 1;

    err = handle.ctx.socket_recvfrom(
              handle.handleID,
              &len,
              &srcAddr);
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
        Debug_LOG_ERROR("OS_NetworkSocket_close() failed, code %d", err);
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
    OS_NetworkSocket_Handle_t invalid_handle = OS_NetworkSocket_Handle_INVALID;

    OS_Error_t err = OS_NetworkSocket_create(
                         &network_stack,
                         &handle,
                         OS_AF_INET,
                         OS_SOCK_DGRAM);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkSocket_create() failed, code %d", err);
    }
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    size_t len = OS_Dataport_getSize(handle.ctx.dataport);

    const OS_NetworkSocket_Addr_t dstAddr =
    {
        .addr = DEV_ADDR,
        .port = 24242
    };

    err = handle.ctx.socket_sendto(
              invalid_handle.handleID,
              &len,
              &dstAddr);
    if (err != OS_ERROR_INVALID_HANDLE)
    {
        Debug_LOG_ERROR(
            "UDP sendto() with invalid handle failed, error %d",
            err);
    }
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_HANDLE, err);

    // creates a length guaranteed larger than that of the dataport, which won't
    // fit in the dataport and will generate an error case
    len = OS_Dataport_getSize(handle.ctx.dataport) + 1;

    err = handle.ctx.socket_sendto(handle.handleID, &len, &dstAddr);
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
        Debug_LOG_ERROR("OS_NetworkSocket_close() failed, code %d", err);
    }
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    TEST_FINISH();
}

void
test_udp_echo()
{
    TEST_START();

    OS_NetworkSocket_Handle_t handle;

    OS_Error_t err = OS_NetworkSocket_create(
                         &network_stack,
                         &handle,
                         OS_AF_INET,
                         OS_SOCK_DGRAM);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkSocket_create() failed, code %d", err);
        return;
    }
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    const OS_NetworkSocket_Addr_t dstAddr =
    {
        .addr = OS_INADDR_ANY_STR,
        .port = CFG_UDP_TEST_PORT
    };

    err = OS_NetworkSocket_bind(handle, &dstAddr);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkSocket_bind() failed, code %d", err);

        err = OS_NetworkSocket_close(handle);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_NetworkSocket_close() failed, code %d", err);
        }
        err = nb_helper_reset_ev_struct_for_socket(handle);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("nb_helper_reset_ev_struct_for_socket() failed, code %d", err);
        }
    }
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    OS_NetworkSocket_Addr_t srcAddr = {0};

    // Buffer big enough to hold 2 frames, rounded to the nearest power of 2
    static char buffer[4096] = {0};

    while (1)
    {
        size_t len = sizeof(buffer);

        do
        {
            // Wait until we get an event for the bound socket.
            nb_helper_wait_for_read_ev_on_socket(handle);

            // Try to read some data.
            err = OS_NetworkSocket_recvfrom(
                      handle,
                      buffer,
                      len,
                      &len,
                      &srcAddr);
        }
        while (err == OS_ERROR_TRY_AGAIN);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_NetworkSocket_recvfrom() failed, code %d", err);

            err = OS_NetworkSocket_close(handle);
            if (err != OS_SUCCESS)
            {
                Debug_LOG_ERROR("OS_NetworkSocket_close() failed, code %d", err);
            }
            err = nb_helper_reset_ev_struct_for_socket(handle);
            if (err != OS_SUCCESS)
            {
                Debug_LOG_ERROR("nb_helper_reset_ev_struct_for_socket() failed, code %d", err);
            }
            return;
        }

        err = OS_NetworkSocket_sendto(
                  handle,
                  buffer,
                  len,
                  &len,
                  &srcAddr);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_NetworkSocket_sendto() failed, code %d", err);

            err = OS_NetworkSocket_close(handle);
            if (err != OS_SUCCESS)
            {
                Debug_LOG_ERROR("OS_NetworkSocket_close() failed, code %d", err);
            }
            err = nb_helper_reset_ev_struct_for_socket(handle);
            if (err != OS_SUCCESS)
            {
                Debug_LOG_ERROR("nb_helper_reset_ev_struct_for_socket() failed, code %d", err);
            }
            return;
        }
    }
    err = OS_NetworkSocket_close(handle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkSocket_close() failed, code %d", err);
        return;
    }
    err = nb_helper_reset_ev_struct_for_socket(handle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("nb_helper_reset_ev_struct_for_socket() failed, code %d", err);
    }
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    TEST_FINISH();
}

//------------------------------------------------------------------------------
int
run()
{
    Debug_LOG_INFO("Starting TestAppUDPServer %s...", get_instance_name());

    test_udp_recvfrom_pos();
    test_udp_sendto_pos();
    test_udp_recvfrom_neg();
    test_udp_sendto_neg();
    test_udp_echo();

    return 0;
}
