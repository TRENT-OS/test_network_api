/*
 *  SEOS Network Stack CAmkES App as Server
 *
 *  Copyright (C) 2019, Hensoldt Cyber GmbH
 *
 */

// This example demonstrates a server with an incoming connection. Reads
// incoming data afterconnection is established. Writes or echoes the received
// data back to the client. When the connection with the client is closed a new
// round begins, in an infinitive loop.
// Currently only a single socket is supported per stack instance.
// i.e. no multitasking is supported as of now.

#include <string.h>
#include "stdint.h"
#include "LibDebug/Debug.h"
#include "SeosError.h"
#include "OS_Network.h"
#include "camkes.h"



//------------------------------------------------------------------------------
static seos_err_t
handle_connection(
    OS_socket_handle_t  hSocket)
{
    seos_err_t ret;

    Debug_LOG_INFO("[socket %u] new connection", hSocket);

    // network stack return code behavior
    //
    //  SEOS_SUCCESS
    //    length > 0 indicate data has been read
    //    length = 0 indicates all data has been read and the TCP
    //      connection is still open - unless one has called read() with
    //      length=0, in this case obviously length=0 will also be returned
    //      and there may still be data in the buffer.
    //
    // SEOS_ERROR_CONNECTION_CLOSED indicates there is no more data to read
    //    and the connection has been closed. If there is still unread
    //    data and the connection has been clsoed, then SEOS_SUCCESS with
    //    length > 0 will be returned
    //
    // SEOS_ERROR_GENERIC is an unspecified error, something went wrong,
    //    the TCP connection may still be alive or not.

    // keep calling read until we receive CONNECTION_CLOSED from the stack
    char buffer[4096] = {0};

    for(;;)
    {
        Debug_LOG_TRACE("[socket %u] read ...", hSocket);

        size_t n = 1;
        ret = OS_socket_read(hSocket, buffer, &n);
        if (SEOS_SUCCESS != ret)
        {
            if (SEOS_ERROR_CONNECTION_CLOSED == ret)
            {
                // connection was closed, consider this as successful. The
                // test runner checks for this string
                Debug_LOG_INFO("connection closed by server");
                return SEOS_SUCCESS;
            }

            Debug_LOG_ERROR("[socket %u] socket_read() failed, error %d",
                            hSocket, ret);
            return SEOS_ERROR_GENERIC;
        }

        Debug_ASSERT(n == 1);
        Debug_LOG_TRACE("[socket %u] read 0x%02x, send it back",
                        hSocket, buffer[0]);

        ret = OS_socket_write(hSocket, buffer, &n);
        if (SEOS_SUCCESS != ret)
        {
            // SEOS_ERROR_CONNECTION_CLOSED is not accepted here, because the
            // peer is not expected to close the connection
            Debug_LOG_ERROR("[socket %u] socket_write() failed, error %d",
                            hSocket, ret);
            return SEOS_ERROR_GENERIC;
        }
    }
}


//------------------------------------------------------------------------------
static seos_err_t
run_server(
    OS_server_socket_handle_t  hServerSocket)
{
    Debug_LOG_INFO("launching echo server");

    for (;;)
    {
        seos_err_t ret;

        Debug_LOG_INFO("[socket %u] waiting for connection ...", hServerSocket);
        OS_socket_handle_t  hSocket = 0;
        ret = OS_socket_accept(hServerSocket, &hSocket);
        if (SEOS_SUCCESS != ret)
        {
            Debug_LOG_ERROR("[socket %u] socket_accept() failed, error %d",
                            hServerSocket, ret);
            return SEOS_ERROR_GENERIC;
        }

        ret = handle_connection(hSocket);

        // close the socket, ignore any error
        (void)OS_socket_close(hSocket);

        if (SEOS_SUCCESS != ret)
        {
            Debug_LOG_ERROR("[socket %u] handle_connection() failed for socket %u, error %d",
                            hServerSocket, hSocket, ret);
            return SEOS_ERROR_GENERIC;
        }
    }
}


//------------------------------------------------------------------------------
static seos_err_t
run_app(void)
{
    seos_err_t ret;

    OS_server_socket_params_t params =
    {
        .mode = OS_SOCKET_IPV4 | OS_SOCKET_STREAM,
        .port = 5555,
        .backlog = 1
    };

    OS_server_socket_handle_t hServerSocket = 0;
    ret = OS_server_socket_create(NULL, &params, &hServerSocket);
    if (SEOS_SUCCESS != ret)
    {
        Debug_LOG_ERROR("server_socket_create() failed, code %d", ret);
        return SEOS_ERROR_GENERIC;
    }

    ret = run_server(hServerSocket);
    if (SEOS_SUCCESS != ret)
    {
        Debug_LOG_ERROR("run_server() failed, error %d", ret);
    }

    // close the server socket, ignore any error
    (void)OS_server_socket_close(hServerSocket);

    return ret;
}


//------------------------------------------------------------------------------
int run(void)
{
    Debug_LOG_INFO("starting test_app_server...");

    // can't make this "static const" or even "static" because the data ports
    // are allocated at runtime
    OS_network_client_lib_config_t network_client_lib_cfg = {
        .wait_init_done = event_network_stack_init_done_wait,
        .api = SETUP_SOCKET_SOCKET_API_INSTANCE(network_stack_rpc),
        .port = {
            .buffer = NwAppDataPort,
            .len    = PAGE_SIZE
        }
    };

    OS_network_client_lib_init(&network_client_lib_cfg);

    seos_err_t ret = run_app();
    if (SEOS_SUCCESS != ret)
    {
        Debug_LOG_ERROR("run_app() failed, error %d", ret);
        return -1;
    }

    Debug_LOG_INFO("finished test_app_server");
    return 0;
}
