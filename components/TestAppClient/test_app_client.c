/*
 *  SEOS Network Stack CAmkES App as client
 *
 *  Copyright (C) 2019, Hensoldt Cyber GmbH
 *
 */

// This example demonstrates reading of a web page example.com using Nw Stack
// API. Currently only a single socket is supported per stack instance i.e. no
// multitasking is supported as of now.

#include <string.h>
#include "stdint.h"
#include "LibDebug/Debug.h"
#include "SeosError.h"
#include "OS_Network.h"
#include "camkes.h"


#define HTTP_PORT   80


//------------------------------------------------------------------------------
static seos_err_t
do_request(
    OS_socket_handle_t  hSocket)
{
    Debug_LOG_INFO("[socket %u] send request to host...", hSocket);

    const char* request =
        "GET / HTTP/1.0\r\nHost: 192.168.82.12\r\nConnection: close\r\n\r\n";

    const size_t len_request = strlen(request);
    size_t offs = 0;
    do
    {
        const size_t lenRemaining = len_request - offs;
        size_t len_io = lenRemaining;

        seos_err_t ret = OS_socket_write(hSocket, &request[offs], &len_io);
        if (SEOS_SUCCESS != ret)
        {
            Debug_LOG_ERROR("[socket %u] socket_write() failed, code %d",
                            hSocket, ret);
            return SEOS_ERROR_GENERIC;
        }

        // sanity check if API is broken.
        Debug_ASSERT( len_io > lenRemaining );

        offs += len_io;
    }
    while (offs < len_request);

    Debug_LOG_INFO("[socket %u] read response ...", hSocket);

    char buffer[4096];

    for(;;)
    {
        memset(buffer, 0, sizeof(buffer));
        size_t len = sizeof(buffer);

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

        seos_err_t ret = OS_socket_read(hSocket, buffer, &len);
        if (SEOS_SUCCESS != ret)
        {
            if (SEOS_ERROR_CONNECTION_CLOSED == ret)
            {
                Debug_LOG_INFO("[socket %u] socket_read() reported connection closed",
                               hSocket);
                return SEOS_SUCCESS;
            }

            Debug_LOG_ERROR("[socket %u] socket_read() failed, error %d",
                            hSocket, ret);
            return SEOS_ERROR_GENERIC;
        }

        Debug_LOG_DEBUG("[socket %u] chunk of %zu byte read",
                        hSocket, len);
    }

}


//------------------------------------------------------------------------------
static seos_err_t
run_app(void)
{
    seos_err_t ret;

    OS_client_socket_params_t params =
    {
        .mode = OS_SOCKET_IPV4 | OS_SOCKET_STREAM,
        .name = CFG_TEST_HTTP_SERVER,
        .port = HTTP_PORT
    };

    OS_socket_handle_t hSocket;
    ret = OS_client_socket_create(NULL, &params, &hSocket);
    if (SEOS_SUCCESS != ret)
    {
        Debug_LOG_ERROR("client_socket_create() failed, code %d", ret);
        return SEOS_ERROR_GENERIC;
    }

    ret = do_request(hSocket);
    if (SEOS_SUCCESS != ret)
    {
        Debug_LOG_ERROR("[socket %u] do_request() failed, error %d",
                        hSocket, ret);
    }

    // close the socket, ignore any error
    (void)OS_socket_close(hSocket);

    return ret;
}


//------------------------------------------------------------------------------
int run(void)
{
    Debug_LOG_INFO("starting test_app_client...");

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

    Debug_LOG_INFO("finished test_app_client");
    return 0;
}
