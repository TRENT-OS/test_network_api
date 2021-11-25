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

#include "OS_Socket.h"
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
    err = OS_Socket_regCallback(
              &network_stack,
              &nb_helper_collect_pending_ev_handler,
              (void*) &network_stack);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR(
            "OS_Socket_regCallback() failed, code %d", err);
    }
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

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

    OS_Socket_Handle_t handle[OS_NETWORK_MAXIMUM_SOCKET_NO];
    OS_Error_t err;

    // Test unsupported domain
    for (int i = 0; i < OS_NETWORK_MAXIMUM_SOCKET_NO; i++)
    {
        err = OS_Socket_create(
                  &network_stack,
                  &handle[i],
                  0,
                  OS_SOCK_STREAM);
        ASSERT_EQ_OS_ERR(OS_ERROR_NETWORK_PROTO_NO_SUPPORT, err);
    }

    // Test unsupported type
    for (int i = 0; i < OS_NETWORK_MAXIMUM_SOCKET_NO; i++)
    {
        err = OS_Socket_create(
                  &network_stack,
                  &handle[i],
                  OS_AF_INET,
                  0);
        ASSERT_EQ_OS_ERR(OS_ERROR_NETWORK_PROTO_NO_SUPPORT, err);
    }

    // Test out of resources
    for (int i = 0; i < OS_NETWORK_MAXIMUM_SOCKET_NO; i++)
    {
        err = OS_Socket_create(
                  &network_stack,
                  &handle[i],
                  OS_AF_INET,
                  OS_SOCK_STREAM);
        ASSERT_EQ_OS_ERR(OS_SUCCESS, err);
    }
    OS_Socket_Handle_t exceedingHandle;
    err = OS_Socket_create(
              &network_stack,
              &exceedingHandle,
              OS_AF_INET,
              OS_SOCK_STREAM);
    ASSERT_EQ_OS_ERR(OS_ERROR_INSUFFICIENT_SPACE, err);

    // Cleanup
    for (int i = 0; i < OS_NETWORK_MAXIMUM_SOCKET_NO; i++)
    {
        OS_Error_t err = OS_Socket_close(handle[i]);
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

    OS_Socket_Handle_t handle[OS_NETWORK_MAXIMUM_SOCKET_NO];

    // Let the following run twice in order to try to catch possible
    // production of RAII garbage
    for (int i = 0; i < 2; i++)
    {
        for (int i = 0; i < OS_NETWORK_MAXIMUM_SOCKET_NO; i++)
        {
            OS_Error_t err =
                OS_Socket_create(
                    &network_stack,
                    &handle[i],
                    OS_AF_INET,
                    OS_SOCK_STREAM);
            ASSERT_EQ_OS_ERR(OS_SUCCESS, err);
        }

        for (int i = 0; i < OS_NETWORK_MAXIMUM_SOCKET_NO; i++)
        {
            OS_Error_t err = OS_Socket_close(handle[i]);
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

    OS_Socket_Handle_t handle;

    OS_Error_t err = OS_Socket_create(
                         &network_stack,
                         &handle,
                         OS_AF_INET,
                         OS_SOCK_STREAM);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = OS_Socket_close(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = nb_helper_reset_ev_struct_for_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    TEST_FINISH();
}

void
test_socket_close_neg()
{
    TEST_START();

    OS_Socket_Handle_t handle;

    OS_Error_t err = OS_Socket_create(
                         &network_stack,
                         &handle,
                         OS_AF_INET,
                         OS_SOCK_STREAM);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    OS_Socket_Handle_t invalid_handle =
    {
        .ctx    = handle.ctx,
        .handleID = -1
    };

    err = OS_Socket_close(invalid_handle);
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_HANDLE, err);

    err = OS_Socket_close(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = nb_helper_reset_ev_struct_for_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    TEST_FINISH();
}

void
test_socket_connect_pos()
{
    TEST_START();

    OS_Socket_Handle_t handle;

    OS_Error_t err = OS_Socket_create(
                         &network_stack,
                         &handle,
                         OS_AF_INET,
                         OS_SOCK_STREAM);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    const OS_Socket_Addr_t dstAddr =
    {
        .addr = CFG_REACHABLE_HOST,
        .port = CFG_REACHABLE_PORT
    };

    err = OS_Socket_connect(handle, &dstAddr);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = nb_helper_wait_for_conn_est_ev_on_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = OS_Socket_close(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = nb_helper_reset_ev_struct_for_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    TEST_FINISH();
}

void
test_socket_connect_neg()
{
    TEST_START();

    OS_Socket_Handle_t handle;

    OS_Error_t err;

    OS_Socket_Addr_t dstAddr =
    {
        .addr = CFG_REACHABLE_HOST,
        .port = CFG_REACHABLE_PORT
    };

    // Test invalid empty destination address.
    err = OS_Socket_create(
              &network_stack,
              &handle,
              OS_AF_INET,
              OS_SOCK_STREAM);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    memset(dstAddr.addr, 0, sizeof(dstAddr.addr));
    err = OS_Socket_connect(handle, &dstAddr);
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_PARAMETER, err);

    err = OS_Socket_close(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    // Test connecting to an unreachable host.
    err = OS_Socket_create(
              &network_stack,
              &handle,
              OS_AF_INET,
              OS_SOCK_STREAM);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    strncpy((char*)dstAddr.addr, CFG_UNREACHABLE_HOST, sizeof(dstAddr.addr));
    err = OS_Socket_connect(handle, &dstAddr);
    ASSERT_EQ_OS_ERR(OS_ERROR_NETWORK_HOST_UNREACHABLE, err);

    err = OS_Socket_close(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    // Test connection refused, now with reachable address but port closed.
    err = OS_Socket_create(
              &network_stack,
              &handle,
              OS_AF_INET,
              OS_SOCK_STREAM);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    strncpy((char*)dstAddr.addr, CFG_REACHABLE_HOST, sizeof(dstAddr.addr));
    dstAddr.port = CFG_UNREACHABLE_PORT;
    err = OS_Socket_connect(handle, &dstAddr);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    // Wait until we receive the expected event that the connection to the
    // unreachable port failed.
    err = nb_helper_wait_for_conn_est_ev_on_socket(handle);
    ASSERT_EQ_OS_ERR(OS_ERROR_NETWORK_CONN_REFUSED, err);

    err = OS_Socket_close(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    // Test forbidden host (connection reset), firewall is configured to send a
    // TCP reset for packets with source and destination port 88.
    err = OS_Socket_create(
              &network_stack,
              &handle,
              OS_AF_INET,
              OS_SOCK_STREAM);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    const OS_Socket_Addr_t srcAddr =
    {
        .addr = CFG_ETH_ADDR_CLIENT_VALUE,
        .port = CFG_FORBIDDEN_PORT
    };

    err = OS_Socket_bind(handle, &srcAddr);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    dstAddr.port = CFG_FORBIDDEN_PORT;
    strncpy((char*)dstAddr.addr, CFG_FORBIDDEN_HOST, sizeof(dstAddr.addr));
    err = OS_Socket_connect(handle, &dstAddr);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = nb_helper_wait_for_conn_est_ev_on_socket(handle);
    ASSERT_EQ_OS_ERR(OS_ERROR_NETWORK_CONN_REFUSED, err);

    err = OS_Socket_close(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = nb_helper_reset_ev_struct_for_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    TEST_FINISH();
}

void
test_socket_non_blocking_neg()
{
    TEST_START();

    char* request = "GET /network/a.txt "
                    "HTTP/1.0\r\nHost: " CFG_TEST_HTTP_SERVER
                    "\r\nConnection: close\r\n\r\n";

    OS_Socket_Handle_t handle;

    OS_Error_t err = OS_Socket_create(
                         &network_stack,
                         &handle,
                         OS_AF_INET,
                         OS_SOCK_STREAM);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = nb_helper_reset_ev_struct_for_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    OS_Socket_Addr_t dstAddr =
    {
        .addr = CFG_REACHABLE_HOST,
        .port = CFG_UNREACHABLE_PORT
    };

    OS_Socket_Addr_t srcAddr = { 0 };

    char   buffer[2048] = { 0 };
    size_t len          = sizeof(buffer);

    // Try to connect to a host that will let the connection timeout. While
    // the connect is pending, try the other socket functions.
    err = OS_Socket_connect(handle, &dstAddr);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = OS_Socket_read(handle, buffer, len, &len);
    ASSERT_EQ_OS_ERR(OS_ERROR_NETWORK_CONN_NONE, err);

    err = OS_Socket_write(handle, request, len, &len);
    ASSERT_EQ_OS_ERR(OS_ERROR_NETWORK_CONN_NONE, err);

    err = OS_Socket_recvfrom(handle, buffer, len, &len, &srcAddr);
    ASSERT_EQ_OS_ERR(OS_ERROR_NETWORK_PROTO, err);

    err = OS_Socket_sendto(handle, buffer, len, &len, &srcAddr);
    ASSERT_EQ_OS_ERR(OS_ERROR_NETWORK_PROTO, err);

    err = OS_Socket_close(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    // Try to connect to a host that will let the connection timeout. After the
    // connection timedout, try the other socket functions.

    OS_Socket_Handle_t handle_connection_timedout;

    err = OS_Socket_create(
              &network_stack,
              &handle_connection_timedout,
              OS_AF_INET,
              OS_SOCK_STREAM);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = nb_helper_reset_ev_struct_for_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = OS_Socket_connect(handle_connection_timedout, &dstAddr);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    // Wait until we receive the expected event that the connection to the
    // unreachable port failed.
    err = nb_helper_wait_for_conn_est_ev_on_socket(handle_connection_timedout);
    ASSERT_EQ_OS_ERR(OS_ERROR_NETWORK_CONN_REFUSED, err);

    err = OS_Socket_read(handle_connection_timedout, buffer, len, &len);
    ASSERT_EQ_OS_ERR(OS_ERROR_NETWORK_CONN_NONE, err);

    err = OS_Socket_write(handle_connection_timedout, request, len, &len);
    ASSERT_EQ_OS_ERR(OS_ERROR_NETWORK_CONN_NONE, err);

    err = OS_Socket_recvfrom(handle, buffer, len, &len, &srcAddr);
    ASSERT_EQ_OS_ERR(OS_ERROR_NETWORK_PROTO, err);

    err = OS_Socket_sendto(handle, buffer, len, &len, &srcAddr);
    ASSERT_EQ_OS_ERR(OS_ERROR_NETWORK_PROTO, err);

    err = OS_Socket_close(handle_connection_timedout);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = nb_helper_reset_ev_struct_for_socket(handle_connection_timedout);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    TEST_FINISH();
}

void
test_tcp_read_pos()
{
    TEST_START();

    OS_Socket_Handle_t handle;

    OS_Error_t err = OS_Socket_create(
                         &network_stack,
                         &handle,
                         OS_AF_INET,
                         OS_SOCK_STREAM);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    const OS_Socket_Addr_t dstAddr =
    {
        .addr = GATEWAY_ADDR,
        .port = CFG_REACHABLE_PORT
    };

    err = nb_helper_reset_ev_struct_for_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = OS_Socket_connect(handle, &dstAddr);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = nb_helper_wait_for_conn_est_ev_on_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    char* request = "GET /network/a.txt "
                    "HTTP/1.0\r\nHost: " CFG_TEST_HTTP_SERVER
                    "\r\nConnection: close\r\n\r\n";

    size_t len_request = strlen(request);
    size_t len_actual = 0;

    err = OS_Socket_write(handle, request, len_request, &len_actual);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    // Intentionally choose a buffer size larger than the underlying dataport.
    static char buffer[OS_DATAPORT_DEFAULT_SIZE + 1] = {0};

    // Wait until we receive a read event for the socket.
    err = nb_helper_wait_for_read_ev_on_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    // Read a small chunk that should fit into the underlying dataport.
    len_request = 256;
    err = OS_Socket_read(handle, buffer, len_request, &len_actual);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);
    // Verify that we read some bytes.
    ASSERT_GT_SZ(len_actual, 0);

    // Read an amount that should not fit into the underlying dataport. The API
    // should adjust this size to the max size of the underlying dataport.
    len_request = sizeof(buffer);

    // Wait until we receive a read event for the socket.
    err = nb_helper_wait_for_read_ev_on_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = OS_Socket_read(handle, buffer, len_request, &len_actual);

    // Verify that we were only able to read at most the size of the underlying
    // dataport and not the requested size.
    ASSERT_LE_SZ(len_actual, networkStack_rpc_get_size());

    err = OS_Socket_close(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = nb_helper_reset_ev_struct_for_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    TEST_FINISH();
}

void
test_tcp_read_neg()
{
    TEST_START();

    OS_Socket_Handle_t handle;

    OS_Error_t err = OS_Socket_create(
                         &network_stack,
                         &handle,
                         OS_AF_INET,
                         OS_SOCK_STREAM);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    char buffer[2048] = {0};
    size_t len = sizeof(buffer);

    // Pass a null pointer for the buffer.
    err = OS_Socket_read(handle, NULL, len, &len);
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_PARAMETER, err);

    // Pass a null pointer for the actual length return parameter.
    err = OS_Socket_read(handle, buffer, len, NULL);
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_PARAMETER, err);

    len = OS_Dataport_getSize(handle.ctx.dataport);

    // Test the call with an invalid handle ID.
    OS_Socket_Handle_t invalidHandle =
    {
        .ctx = handle.ctx,
        .handleID = -1
    };
    err = OS_Socket_read(invalidHandle, buffer, len, &len);
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_HANDLE, err);

    err = OS_Socket_close(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = nb_helper_reset_ev_struct_for_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    TEST_FINISH();
}

void
test_tcp_write_neg()
{
    TEST_START();

    OS_Socket_Handle_t handle;

    OS_Error_t err = OS_Socket_create(
                         &network_stack,
                         &handle,
                         OS_AF_INET,
                         OS_SOCK_STREAM);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    const OS_Socket_Addr_t dstAddr =
    {
        .addr = GATEWAY_ADDR,
        .port = CFG_REACHABLE_PORT
    };

    err = OS_Socket_connect(handle, &dstAddr);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    char* request = "GET /network/a.txt "
                    "HTTP/1.0\r\nHost: " CFG_TEST_HTTP_SERVER
                    "\r\nConnection: close\r\n\r\n";

    err = nb_helper_wait_for_conn_est_ev_on_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    const size_t len_request = strlen(request);
    size_t len_actual = 0;

    // Pass a null pointer for the buffer.
    err = OS_Socket_write(handle, NULL, len_request, &len_actual);
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_PARAMETER, err);

    // Pass a null pointer for the actual length return parameter.
    err = OS_Socket_write(handle, request, len_request, NULL);
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_PARAMETER, err);

    // Test the call with an invalid handle ID.
    OS_Socket_Handle_t invalidHandle =
    {
        .ctx = handle.ctx,
        .handleID = -1
    };
    err = OS_Socket_write(invalidHandle, request, len_request, &len_actual);
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_HANDLE, err);

    err = OS_Socket_close(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = nb_helper_reset_ev_struct_for_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    TEST_FINISH();
}

void
test_tcp_write_pos()
{
    TEST_START();

    OS_Socket_Handle_t handle;

    OS_Error_t err = OS_Socket_create(
                         &network_stack,
                         &handle,
                         OS_AF_INET,
                         OS_SOCK_STREAM);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    const OS_Socket_Addr_t dstAddr =
    {
        .addr = GATEWAY_ADDR,
        .port = CFG_REACHABLE_PORT
    };

    err = OS_Socket_connect(handle, &dstAddr);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = nb_helper_wait_for_conn_est_ev_on_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    char* request = "GET /network/a.txt "
                    "HTTP/1.0\r\nHost: " CFG_TEST_HTTP_SERVER
                    "\r\nConnection: close\r\n\r\n";

    size_t len_request = strlen(request);
    size_t offs = 0;

    // Loop until all data is written.
    do
    {
        const size_t lenRemaining = len_request - offs;
        size_t       len_io       = lenRemaining;

        err = OS_Socket_write(
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

    // Intentionally choose a buffer size larger than the underlying dataport.
    static char buffer[OS_DATAPORT_DEFAULT_SIZE + 1] = {0};
    len_request = sizeof(buffer);
    size_t len_actual = 0;

    err = OS_Socket_write(
              handle,
              &request[offs],
              len_request,
              &len_actual);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    // Verify that we were only able to write at most the size of the underlying
    // dataport and not the requested size.
    ASSERT_LE_SZ(len_actual, networkStack_rpc_get_size());

    err = OS_Socket_close(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    err = nb_helper_reset_ev_struct_for_socket(handle);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    TEST_FINISH();
}

void
test_tcp_client()
{
    TEST_START();

    const OS_Socket_Addr_t dstAddr =
    {
        .addr = GATEWAY_ADDR,
        .port = CFG_REACHABLE_PORT
    };

    OS_Socket_Handle_t handle[OS_NETWORK_MAXIMUM_SOCKET_NO];
    OS_Error_t err;
    int i;

    for (i = 0; i < OS_NETWORK_MAXIMUM_SOCKET_NO; i++)
    {
        err = OS_Socket_create(
                  &network_stack,
                  &handle[i],
                  OS_AF_INET,
                  OS_SOCK_STREAM);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR(
                "OS_Socket_create() failed, code %d for %d socket",
                err,
                i);
            break;
        }

        err = OS_Socket_connect(handle[i], &dstAddr);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR(
                "OS_Socket_connect() failed, code %d for %d socket",
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

            err = OS_Socket_write(
                      handle[i],
                      &request[offs],
                      len_io,
                      &len_io);

            if (err != OS_SUCCESS)
            {
                Debug_LOG_ERROR("OS_Socket_write() failed, code %d", err);
                OS_Socket_close(handle[i]);
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
                err = OS_Socket_read(handle[i], buffer, len, &len);
            }
            switch (err)
            {

            /* This means end of read or nothing further to read as socket was
             * closed */
            case OS_ERROR_NETWORK_CONN_SHUTDOWN:
                Debug_LOG_INFO(
                    "OS_Socket_read() reported connection closed for handle %d",
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
                    "OS_Socket_read() failed for handle %d, error %d",
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
        err = OS_Socket_close(handle[i]);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR(
                "OS_Socket_close() failed for handle %d, code %d", i,
                err);
            return;
        }
        nb_helper_reset_ev_struct_for_socket(handle[i]);
    }

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
        test_socket_create_neg();
        test_socket_create_pos();
        test_socket_close_pos();
        test_socket_close_neg();
        test_socket_connect_pos();
        test_socket_connect_neg();
        test_socket_non_blocking_neg();
        test_tcp_write_pos();
        test_tcp_write_neg();
        test_tcp_read_pos();
        test_tcp_read_neg();
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
