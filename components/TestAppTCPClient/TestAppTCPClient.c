/*
 * TestAppTCPClient
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

IF_OS_SOCKET_DEFINE_CONNECTOR(networkStack_rpc);

static const if_OS_Socket_t network_stack =
    IF_OS_SOCKET_ASSIGN(networkStack_rpc);

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
test_socket_create_neg()
{
    // This test will try to create sockets with invalid parameters. After this
    // it will try to open more sockets than the ressources will allow for to
    // verify that this exceeding creation will fail.
    TEST_START();

    OS_NetworkSocket_Handle_t handle[OS_NETWORK_MAXIMUM_SOCKET_NO];
    OS_Error_t err;

    // Test unsupported domain
    for (int i = 0; i < OS_NETWORK_MAXIMUM_SOCKET_NO; i++)
    {
        err = OS_NetworkSocket_create(
                  &network_stack,
                  &handle[i],
                  0,
                  OS_SOCK_STREAM);
        ASSERT_EQ_OS_ERR(OS_ERROR_NETWORK_PROTO_NO_SUPPORT, err);
    }

    // Test unsupported type
    for (int i = 0; i < OS_NETWORK_MAXIMUM_SOCKET_NO; i++)
    {
        err = OS_NetworkSocket_create(
                  &network_stack,
                  &handle[i],
                  OS_AF_INET,
                  0);
        ASSERT_EQ_OS_ERR(OS_ERROR_NETWORK_PROTO_NO_SUPPORT, err);
    }

    // Test out of resources
    for (int i = 0; i < OS_NETWORK_MAXIMUM_SOCKET_NO; i++)
    {
        err = OS_NetworkSocket_create(
                  &network_stack,
                  &handle[i],
                  OS_AF_INET,
                  OS_SOCK_STREAM);
        ASSERT_EQ_OS_ERR(OS_SUCCESS, err);
    }
    OS_NetworkSocket_Handle_t exceedingHandle;
    err = OS_NetworkSocket_create(
              &network_stack,
              &exceedingHandle,
              OS_AF_INET,
              OS_SOCK_STREAM);
    ASSERT_EQ_OS_ERR(OS_ERROR_INSUFFICIENT_SPACE, err);

    // Cleanup
    for (int i = 0; i < OS_NETWORK_MAXIMUM_SOCKET_NO; i++)
    {
        OS_Error_t err = OS_NetworkSocket_close(handle[i]);
        ASSERT_EQ_OS_ERR(OS_SUCCESS, err);
        err = nb_helper_reset_ev_struct_for_socket(handle[i]);
        ASSERT_EQ_OS_ERR(OS_SUCCESS, err);
    }

    TEST_FINISH();
}

void
test_socket_create_pos()
{
    // This test will test successful creation of an amount of
    // OS_NETWORK_MAXIMUM_SOCKET_NO sockets. Then it will successfully destroy
    // them.
    // That sequence of actions is repeated twice in order to try to detect
    // resources allocation issues.
    // For the purpose of trying to find resources allocation issues it is
    // suggested to run test_socket_create_neg() prior to this
    TEST_START();

    OS_NetworkSocket_Handle_t handle[OS_NETWORK_MAXIMUM_SOCKET_NO];

    // Let the following run twice in order to try to catch possible
    // production of RAII garbage
    for (int i = 0; i < 2; i++)
    {
        for (int i = 0; i < OS_NETWORK_MAXIMUM_SOCKET_NO; i++)
        {
            OS_Error_t err =
                OS_NetworkSocket_create(
                    &network_stack,
                    &handle[i],
                    OS_AF_INET,
                    OS_SOCK_STREAM);
            ASSERT_EQ_OS_ERR(OS_SUCCESS, err);
        }

        for (int i = 0; i < OS_NETWORK_MAXIMUM_SOCKET_NO; i++)
        {
            OS_Error_t err = OS_NetworkSocket_close(handle[i]);
            ASSERT_EQ_OS_ERR(OS_SUCCESS, err);
            err = nb_helper_reset_ev_struct_for_socket(handle[i]);
            ASSERT_EQ_OS_ERR(OS_SUCCESS, err);
        }
    }

    TEST_FINISH();
}

void
test_socket_close_pos()
{
    TEST_START();

    OS_NetworkSocket_Handle_t handle;

    OS_Error_t err = OS_NetworkSocket_create(
                         &network_stack,
                         &handle,
                         OS_AF_INET,
                         OS_SOCK_STREAM);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = OS_NetworkSocket_close(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = nb_helper_reset_ev_struct_for_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    TEST_FINISH();
}

void
test_socket_close_neg()
{
    TEST_START();

    OS_NetworkSocket_Handle_t handle;

    OS_Error_t err = OS_NetworkSocket_create(
                         &network_stack,
                         &handle,
                         OS_AF_INET,
                         OS_SOCK_STREAM);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    OS_NetworkSocket_Handle_t invalid_handle =
    {
        .ctx    = handle.ctx,
        .handleID = -1
    };

    err = OS_NetworkSocket_close(invalid_handle);
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_HANDLE, err);

    err = OS_NetworkSocket_close(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = nb_helper_reset_ev_struct_for_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    TEST_FINISH();
}

void
test_socket_connect_pos()
{
    TEST_START();

    OS_NetworkSocket_Handle_t handle;

    OS_Error_t err = OS_NetworkSocket_create(
                         &network_stack,
                         &handle,
                         OS_AF_INET,
                         OS_SOCK_STREAM);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    const OS_NetworkSocket_Addr_t dstAddr =
    {
        .addr = CFG_REACHABLE_HOST,
        .port = CFG_REACHABLE_PORT
    };

    err = OS_NetworkSocket_connect(handle, &dstAddr);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = nb_helper_wait_for_conn_est_ev_on_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = OS_NetworkSocket_close(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = nb_helper_reset_ev_struct_for_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    TEST_FINISH();
}

void
test_socket_connect_neg()
{
    TEST_START();

    OS_NetworkSocket_Handle_t handle;

    OS_Error_t err = OS_NetworkSocket_create(
                         &network_stack,
                         &handle,
                         OS_AF_INET,
                         OS_SOCK_STREAM);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    OS_NetworkSocket_Addr_t dstAddr =
    {
        .addr = CFG_REACHABLE_HOST,
        .port = CFG_REACHABLE_PORT
    };

    // Test invalid empty destination address.
    memset(dstAddr.addr, 0, sizeof(dstAddr.addr));
    err = OS_NetworkSocket_connect(handle, &dstAddr);
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_PARAMETER, err);

    // Test connecting to an unreachable host.
    strncpy((char*)dstAddr.addr, CFG_UNREACHABLE_HOST, sizeof(dstAddr.addr));
    err = OS_NetworkSocket_connect(handle, &dstAddr);
    ASSERT_EQ_OS_ERR(OS_ERROR_NETWORK_HOST_UNREACHABLE, err);

    // Test connection refused, now with reachable address but port closed.
    strncpy((char*)dstAddr.addr, CFG_REACHABLE_HOST, sizeof(dstAddr.addr));
    dstAddr.port = CFG_UNREACHABLE_PORT;
    err = OS_NetworkSocket_connect(handle, &dstAddr);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    // Wait until we receive the expected event that the connection to the
    // unreachable port failed.
    err = nb_helper_wait_for_conn_est_ev_on_socket(handle);
    ASSERT_EQ_OS_ERR(OS_ERROR_NETWORK_CONN_REFUSED, err);

    // Test forbidden host (connection reset), firewall is configured to reset
    // TCP for this host.
    // This test is momentarily suppressed as it seems that picotcp is not
    // behaving correctly. There is need of further investigations to establish
    // whether is an issue with iptables or with picotcp
    // strncpy((char*)dstAddr.addr, CFG_FORBIDDEN_HOST, sizeof(dstAddr.addr));
    // err = OS_NetworkSocket_connect(handle, &dstAddr);
    // ASSERT_EQ_OS_ERR(OS_ERROR_NETWORK_CONN_RESET, err);

    err = OS_NetworkSocket_close(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = nb_helper_reset_ev_struct_for_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    TEST_FINISH();
}

void
test_tcp_read_pos()
{
    TEST_START();

    OS_NetworkSocket_Handle_t handle;

    OS_Error_t err = OS_NetworkSocket_create(
                         &network_stack,
                         &handle,
                         OS_AF_INET,
                         OS_SOCK_STREAM);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    const OS_NetworkSocket_Addr_t dstAddr =
    {
        .addr = GATEWAY_ADDR,
        .port = CFG_REACHABLE_PORT
    };

    err = OS_NetworkSocket_connect(handle, &dstAddr);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = nb_helper_wait_for_conn_est_ev_on_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    char* request = "GET /network/a.txt "
                    "HTTP/1.0\r\nHost: " CFG_TEST_HTTP_SERVER
                    "\r\nConnection: close\r\n\r\n";

    const size_t len_request = strlen(request);
    size_t       len         = len_request;

    err = OS_NetworkSocket_write(handle, request, len, &len);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    char buffer[2048] = {0};
    len = sizeof(buffer);

    // Wait until we receive a read event for the socket.
    err = nb_helper_wait_for_read_ev_on_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = OS_NetworkSocket_read(handle, buffer, len, &len);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = OS_NetworkSocket_close(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = nb_helper_reset_ev_struct_for_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    TEST_FINISH();
}

void
test_tcp_read_neg()
{
    TEST_START();

    OS_NetworkSocket_Handle_t handle;

    OS_Error_t err = OS_NetworkSocket_create(
                         &network_stack,
                         &handle,
                         OS_AF_INET,
                         OS_SOCK_STREAM);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    char buffer[2048] = {0};
    size_t len = sizeof(buffer);

    // Creates a length guaranteed larger than that of the dataport, which won't
    // fit in the dataport and will generate an error case.
    len = OS_Dataport_getSize(handle.ctx.dataport) + 1;

    err = OS_NetworkSocket_read(handle, buffer, len, &len);
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_PARAMETER, err);

    len = OS_Dataport_getSize(handle.ctx.dataport);

    // Test the call with an invalid handle ID.
    OS_NetworkSocket_Handle_t invalidHandle =
    {
        .ctx = handle.ctx,
        .handleID = -1
    };
    err = OS_NetworkSocket_read(invalidHandle, buffer, len, &len);
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_HANDLE, err);

    err = OS_NetworkSocket_close(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = nb_helper_reset_ev_struct_for_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    TEST_FINISH();
}

void
test_tcp_write_neg()
{
    TEST_START();

    OS_NetworkSocket_Handle_t handle;

    OS_Error_t err = OS_NetworkSocket_create(
                         &network_stack,
                         &handle,
                         OS_AF_INET,
                         OS_SOCK_STREAM);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    const OS_NetworkSocket_Addr_t dstAddr =
    {
        .addr = GATEWAY_ADDR,
        .port = CFG_REACHABLE_PORT
    };

    err = OS_NetworkSocket_connect(handle, &dstAddr);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    char* request = "GET /network/a.txt "
                    "HTTP/1.0\r\nHost: " CFG_TEST_HTTP_SERVER
                    "\r\nConnection: close\r\n\r\n";

    err = nb_helper_wait_for_conn_est_ev_on_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    const size_t len_request = strlen(request);
    size_t       len         = len_request;

    // creates a length guaranteed larger than that of the dataport, which won't
    // fit in the dataport and will generate an error case
    len = OS_Dataport_getSize(handle.ctx.dataport) + 1;

    err = OS_NetworkSocket_write(handle, request, len, &len);
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_PARAMETER, err);

    len = OS_Dataport_getSize(handle.ctx.dataport);

    // Test the call with an invalid handle ID.
    OS_NetworkSocket_Handle_t invalidHandle =
    {
        .ctx = handle.ctx,
        .handleID = -1
    };
    err = OS_NetworkSocket_write(invalidHandle, request, len, &len);
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_HANDLE, err);

    err = OS_NetworkSocket_close(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = nb_helper_reset_ev_struct_for_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    TEST_FINISH();
}

void
test_tcp_write_pos()
{
    TEST_START();

    OS_NetworkSocket_Handle_t handle;

    OS_Error_t err = OS_NetworkSocket_create(
                         &network_stack,
                         &handle,
                         OS_AF_INET,
                         OS_SOCK_STREAM);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    const OS_NetworkSocket_Addr_t dstAddr =
    {
        .addr = GATEWAY_ADDR,
        .port = CFG_REACHABLE_PORT
    };

    err = OS_NetworkSocket_connect(handle, &dstAddr);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = nb_helper_wait_for_conn_est_ev_on_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    char* request = "GET /network/a.txt "
                    "HTTP/1.0\r\nHost: " CFG_TEST_HTTP_SERVER
                    "\r\nConnection: close\r\n\r\n";

    const size_t len_request = strlen(request);
    size_t offs = 0;

    // Loop until all data is written.
    do
    {
        const size_t lenRemaining = len_request - offs;
        size_t       len_io       = lenRemaining;

        err = OS_NetworkSocket_write(
                  handle,
                  &request[offs],
                  len_io,
                  &len_io);
        ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

        /* fatal error, this must not happen. API broken*/
        ASSERT_LE_SZ(len_io, lenRemaining);

        offs += len_io;
    }
    while (offs < len_request);

    err = OS_NetworkSocket_close(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = nb_helper_reset_ev_struct_for_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    TEST_FINISH();
}

void
test_tcp_client()
{
    TEST_START();

    const OS_NetworkSocket_Addr_t dstAddr =
    {
        .addr = GATEWAY_ADDR,
        .port = CFG_REACHABLE_PORT
    };

    OS_NetworkSocket_Handle_t handle[OS_NETWORK_MAXIMUM_SOCKET_NO];
    OS_Error_t err;
    int i;

    for (i = 0; i < OS_NETWORK_MAXIMUM_SOCKET_NO; i++)
    {
        err = OS_NetworkSocket_create(
                  &network_stack,
                  &handle[i],
                  OS_AF_INET,
                  OS_SOCK_STREAM);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR(
                "OS_NetworkSocket_create() failed, code %d for %d socket",
                err,
                i);
            break;
        }

        err = OS_NetworkSocket_connect(handle[i], &dstAddr);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR(
                "OS_NetworkSocket_connect() failed, code %d for %d socket",
                err,
                i);
            break;
        }

        err = nb_helper_wait_for_conn_est_ev_on_socket(handle[i]);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR(
                "nb_helper_wait_for_conn_est_ev_on_socket() failed, code %d for %d socket",
                err,
                i);
            break;
        }
    }

    int socket_max = i;
    Debug_LOG_INFO("Send request to host...");

    char* request = "GET /network/a.txt "
                    "HTTP/1.0\r\nHost: " CFG_TEST_HTTP_SERVER
                    "\r\nConnection: close\r\n\r\n";

    const size_t len_request = strlen(request);
    size_t       len         = len_request;

    /* Send the request to the host */
    for (i = 0; i < socket_max; i++)
    {
        size_t offs = 0;
        Debug_LOG_INFO("Writing request to socket %d for %.*s", i, 17, request);
        request[13] = 'a' + i;
        do
        {
            const size_t lenRemaining = len_request - offs;
            size_t       len_io       = lenRemaining;

            err = OS_NetworkSocket_write(
                      handle[i],
                      &request[offs],
                      len_io,
                      &len_io);

            if (err != OS_SUCCESS)
            {
                Debug_LOG_ERROR("OS_NetworkSocket_write() failed, code %d", err);
                OS_NetworkSocket_close(handle[i]);
                nb_helper_reset_ev_struct_for_socket(handle[i]);
                return;
            }

            /* fatal error, this must not happen. API broken*/
            ASSERT_LE_SZ(len_io, lenRemaining);

            offs += len_io;
        }
        while (offs < len_request);
    }
    Debug_LOG_INFO("read response...");

    /*
    As of now the nw stack behavior is as below:
    Keep reading data until you receive one of the return values:
     a. err = OS_ERROR_NETWORK_CONN_SHUTDOWN indicating end of data read
              and connection close
     b. err = OS_ERROR_GENERIC  due to error in read
     c. err = OS_SUCCESS and length = 0 indicating no data to read but there
    is still connection
     d. err = OS_SUCCESS and length >0 , valid data

    Take appropriate actions based on the return value rxd.

    Once a webpage is read , display the contents.
    */

    int flag = 0;
    char buffer[2048] = {0};

    do
    {
        for (i = 0; i < socket_max; i++)
        {
            len = sizeof(buffer);
            /* Keep calling read until we receive OS_ERROR_NETWORK_CONN_SHUTDOWN
            from the stack */
            OS_Error_t err = OS_ERROR_NETWORK_CONN_SHUTDOWN;

            // Wait until we receive a read event for the socket.
            err = nb_helper_wait_for_read_ev_on_socket(handle[i]);

            if (!(flag & (1 << i)) && (err == OS_SUCCESS))
            {
                err = OS_NetworkSocket_read(handle[i], buffer, len, &len);
            }
            switch (err)
            {

            /* This means end of read or nothing further to read as socket was
             * closed */
            case OS_ERROR_NETWORK_CONN_SHUTDOWN:
                Debug_LOG_INFO(
                    "OS_NetworkSocket_read() reported connection closed for handle %d",
                    i);
                flag |= 1 << i; /* terminate loop and close handle*/
                break;

            /* Success . continue further reading */
            case OS_SUCCESS:
                Debug_LOG_INFO("chunk read, length %d, handle %d", len, i);
                break;

            /* Error case, break and close the handle */
            default:
                Debug_LOG_INFO(
                    "OS_NetworkSocket_read() failed for handle %d, error %d",
                    i,
                    err);
                flag |= 1 << i; /* terminate loop and close handle */
                break;
            } // end of switch
        }
    }
    while (flag != pow(2, socket_max) - 1);
    Debug_LOG_INFO("Test ended");

    for (i = 0; i < socket_max; i++)
    {
        /* Close the socket communication */
        err = OS_NetworkSocket_close(handle[i]);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR(
                "OS_NetworkSocket_close() failed for handle %d, code %d", i,
                err);
            return;
        }
        nb_helper_reset_ev_struct_for_socket(handle[i]);
    }

    TEST_FINISH();
}

void
test_dataport_size_check_client_functions()
{
    TEST_START();

    OS_NetworkSocket_Handle_t handle;

    OS_Error_t err = OS_NetworkSocket_create(
                         &network_stack,
                         &handle,
                         OS_AF_INET,
                         OS_SOCK_STREAM);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    // creates a length guaranteed larger than that of the dataport, which won't
    // fit in the dataport and will generate an error case
    size_t len = OS_Dataport_getSize(handle.ctx.dataport) + 1;

    // Buffer big enough to hold 2 frames, rounded to the nearest power of 2
    static char buffer[4096];

    err = OS_NetworkSocket_read(handle, buffer, len, NULL);
    if (err != OS_ERROR_INVALID_PARAMETER)
    {
        Debug_LOG_ERROR(
            "Client socket read with invalid dataport size failed, error %d",
            err);
    }
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_PARAMETER, err);

    OS_NetworkSocket_Addr_t srcAddr = {0};

    err = OS_NetworkSocket_recvfrom(
              handle,
              buffer,
              len,
              NULL,
              &srcAddr);
    if (err != OS_ERROR_INVALID_PARAMETER)
    {
        Debug_LOG_ERROR(
            "Client socket recvfrom with invalid dataport size failed, "
            "error "
            "%d",
            err);
    }
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_PARAMETER, err);

    err = OS_NetworkSocket_write(handle, buffer, len, NULL);
    if (err != OS_ERROR_INVALID_PARAMETER)
    {
        Debug_LOG_ERROR(
            "Client socket write with invalid dataport size failed, error "
            "%d",
            err);
    }
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_PARAMETER, err);

    err = OS_NetworkSocket_sendto(handle, buffer, len, NULL, &srcAddr);
    if (err != OS_ERROR_INVALID_PARAMETER)
    {
        Debug_LOG_ERROR(
            "Client socket sendto with invalid dataport size failed, error "
            "%d",
            err);
    }
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_PARAMETER, err);

    err = OS_NetworkSocket_close(handle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("close() failed, code %d", err);
    }

    nb_helper_reset_ev_struct_for_socket(handle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("nb_helper_reset_ev_struct_for_socket() failed, code %d", err);
    }

    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    TEST_FINISH();
}

void
test_dataport_size_check_lib_functions()
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
    }
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    // creates a length guaranteed larger than that of the dataport, which won't
    // fit in the dataport and will generate an error case
    size_t len = OS_Dataport_getSize(handle.ctx.dataport) + 1;

    err = handle.ctx.socket_read(handle.handleID, &len);
    if (err != OS_ERROR_INVALID_PARAMETER)
    {
        Debug_LOG_ERROR(
            "Lib socket read with invalid dataport size failed, error %d",
            err);
    }
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_PARAMETER, err);

    OS_NetworkSocket_Addr_t srcAddr = {0};

    err = handle.ctx.socket_recvfrom(handle.handleID, &len, &srcAddr);
    if (err != OS_ERROR_INVALID_PARAMETER)
    {
        Debug_LOG_ERROR(
            "Lib socket recvfrom with invalid dataport size failed, error %d",
            err);
    }
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_PARAMETER, err);

    err = handle.ctx.socket_write(handle.handleID, &len);
    if (err != OS_ERROR_INVALID_PARAMETER)
    {
        Debug_LOG_ERROR(
            "Lib socket write with invalid dataport size failed, error %d",
            err);
    }
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_PARAMETER, err);

    err = handle.ctx.socket_sendto(handle.handleID, &len, &srcAddr);
    if (err != OS_ERROR_INVALID_PARAMETER)
    {
        Debug_LOG_ERROR(
            "Lib socket sendto with invalid dataport size failed, error %d",
            err);
    }
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_PARAMETER, err);

    err = OS_NetworkSocket_close(handle);

    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("close() failed, code %d", err);
    }

    nb_helper_reset_ev_struct_for_socket(handle);
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
    Debug_LOG_INFO("Starting TestAppTCPClient %s...", get_instance_name());

#ifdef TCP_CLIENT
    if ((0 == strcmp(get_instance_name(), "testAppTCPClient_singleSocket")) ||
        (0 == strcmp(get_instance_name(), "testAppTCPClient_client1")))
    {
        // The following API tests do not need to be executed in parallel
        // therefore only testAppTCPClient_client1 will execute them.
        test_dataport_size_check_client_functions();
        test_dataport_size_check_lib_functions();
        test_socket_create_neg();
        test_socket_create_pos();
        test_socket_close_pos();
        test_socket_close_neg();
        test_socket_connect_pos();
        test_socket_connect_neg();
        test_tcp_read_pos();
        test_tcp_read_neg();
        test_tcp_write_pos();
        test_tcp_write_neg();
    }
#endif

#ifdef TCP_CLIENT_MULTIPLE_CLIENTS
    // synchronise the two clients
    multiple_client_sync_send_ready_emit();
    multiple_client_sync_recv_ready_wait();
#endif

    test_tcp_client();

    return 0;
}
