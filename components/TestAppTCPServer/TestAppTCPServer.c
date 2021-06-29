/*
 * TestAppTCPServer
 *
 * Copyright (C) 2019-2021, HENSOLDT Cyber GmbH
 *
 */
#include "OS_Error.h"
#include "lib_compiler/compiler.h"
#include "lib_debug/Debug.h"
#include "stdint.h"
#include <string.h>

#include "OS_Network.h"
#include "OS_NetworkStackClient.h"
#include "interfaces/if_OS_NetworkStack.h"
#include "util/loop_defines.h"
#include <camkes.h>

static const if_OS_NetworkStack_t network_stack =
    IF_OS_SOCKET_ASSIGN(networkStack_rpc, socket_1_port);

/*
 * This example demonstrates a server with an incoming connection. Reads
 *  incoming data after a connection is established. Writes or echoes the
 * received data back to the client. When the connection with the client is
 * closed a new round begins, in an infinitive loop. Currently only a single
 * socket is supported per stack instance. i.e. no multitasking is supported as
 * of now.
 */

void
init_client_api()
{
    static OS_NetworkStackClient_SocketDataports_t config;

    config.number_of_sockets = OS_NETWORK_MAXIMUM_SOCKET_NO;
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

//------------------------------------------------------------------------------
void
pre_init(void)
{
#if defined(Debug_Config_PRINT_TO_LOG_SERVER)
    DECL_UNUSED_VAR(OS_Error_t err) = SysLoggerClient_init(sysLogger_Rpc_log);
    Debug_ASSERT(err == OS_SUCCESS);
#endif
}

//------------------------------------------------------------------------------
int
run()
{
    init_client_api();

    Debug_LOG_INFO("Starting TestAppTCPServer ...");

    char buffer[4096];

    OS_NetworkServer_Socket_t srv_socket =
    {
        .domain      = OS_AF_INET,
        .type        = OS_SOCK_STREAM,
        .listen_port = CFG_TCP_SERVER_PORT,
        .backlog     = 1,
    };

    /* Gets filled when accept is called */
    OS_NetworkSocket_Handle_t os_socket_handle;
    /* Gets filled when server socket create is called */
    OS_NetworkServer_Handle_t os_nw_server_handle;

    OS_Error_t err = OS_NetworkServerSocket_create(
                         &network_stack,
                         &srv_socket,
                         &os_nw_server_handle);

    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("server_socket_create() failed, code %d", err);
        return -1;
    }

    Debug_LOG_INFO("launching echo server");

    for (;;)
    {
        err = OS_NetworkServerSocket_accept(
                  os_nw_server_handle,
                  &os_socket_handle);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("socket_accept() failed, error %d", err);
            return -1;
        }

        /*
            As of now the nw stack behavior is as below:
            Keep reading data until you receive one of the return values:
             a. err = OS_ERROR_CONNECTION_CLOSED and length = 0 indicating end
           of data read and connection close b. err = OS_ERROR_GENERIC  due to
           error in read c. err = OS_SUCCESS and length = 0 indicating no data
           to read but there is still connection d. err = OS_SUCCESS and length
           >0 , valid data

            Take appropriate actions based on the return value rxd.


            Only a single socket is supported and no multithreading !!!
            Once we accept an incoming connection, start reading data from the
           client and echo back the data rxd.
        */
        memset(buffer, 0, sizeof(buffer));

        Debug_LOG_INFO("starting server read loop");
        /* Keep calling read until we receive CONNECTION_CLOSED from the stack
         */
        for (;;)
        {
            Debug_LOG_DEBUG("read...");
            size_t n = 0;
            // Try to read as much as fits into the buffer
            err = OS_NetworkSocket_read(
                      os_socket_handle,
                      buffer,
                      sizeof(buffer),
                      &n);
            if (OS_SUCCESS != err)
            {
                Debug_LOG_ERROR("socket_read() failed, error %d", err);
                break;
            }

            size_t bytesWritten      = 0;
            size_t totalBytesWritten = 0;

            while (totalBytesWritten < n)
            {
                err = OS_NetworkSocket_write(
                          os_socket_handle,
                          &buffer[totalBytesWritten],
                          n - totalBytesWritten,
                          &bytesWritten);
                if (err != OS_SUCCESS)
                {
                    Debug_LOG_ERROR("socket_write() failed, error %d", err);
                    break;
                }
                totalBytesWritten = totalBytesWritten + bytesWritten;
            }
        }

        switch (err)
        {
        /* This means end of read as socket was closed. Exit now and close
         * handle*/
        case OS_ERROR_CONNECTION_CLOSED:
            // the test runner checks for this string
            Debug_LOG_INFO("connection closed by server");
            OS_NetworkSocket_close(os_socket_handle);
            continue;
        /* Any other value is a failure in read, hence exit and close handle  */
        default:
            Debug_LOG_ERROR("server socket failure, error %d", err);
            OS_NetworkSocket_close(os_socket_handle);
            continue;
        } // end of switch
    }
    return -1;
}
