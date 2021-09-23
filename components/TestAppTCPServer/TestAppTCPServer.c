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
#include "interfaces/if_OS_Socket.h"
#include "if_NetworkStack_PicoTcp_Config.h"
#include "util/loop_defines.h"
#include "util/non_blocking_helper.h"
#include <camkes.h>

IF_OS_SOCKET_DEFINE_CONNECTOR(networkStack_rpc);

static const if_OS_Socket_t network_stack =
    IF_OS_SOCKET_ASSIGN(networkStack_rpc);

static const if_NetworkStack_PicoTcp_Config_t networkStackConfig =
    if_NetworkStack_PicoTcp_Config_ASSIGN(networkStack_PicoTcp_Config_rpc);

/*
 * This example demonstrates a server with an incoming connection. Reads
 *  incoming data after a connection is established. Writes or echoes the
 * received data back to the client. When the connection with the client is
 * closed a new round begins, in an infinitive loop. Currently only a single
 * socket is supported per stack instance. i.e. no multitasking is supported as
 * of now.
 */

//------------------------------------------------------------------------------
void
pre_init(void)
{
#if defined(Debug_Config_PRINT_TO_LOG_SERVER)
    DECL_UNUSED_VAR(OS_Error_t err) = SysLoggerClient_init(sysLogger_Rpc_log);
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
}

//------------------------------------------------------------------------------
int
run()
{
    Debug_LOG_INFO("Starting TestAppTCPServer ...");

    OS_NetworkSocket_Handle_t srvHandle;
    OS_Error_t err = OS_ERROR_GENERIC;

#if !defined(NetworkStack_PicoTcp_USE_HARDCODED_IPADDR)
    static const OS_NetworkStack_AddressConfig_t ipAddrConfig =
    {
        .dev_addr       = "10.0.0.11",
        .gateway_addr   = "10.0.0.1",
        .subnet_mask    = "255.255.255.0"
    };

    err = networkStackConfig.configIpAddr(&ipAddrConfig);
    Debug_ASSERT(err == OS_SUCCESS);
#endif
    do
    {
        err = OS_NetworkSocket_create(
                  &network_stack,
                  &srvHandle,
                  OS_AF_INET,
                  OS_SOCK_STREAM);
        if (OS_ERROR_NOT_INITIALIZED == err)
        {
            // just yield to wait until the stack is up and running
            seL4_Yield();
        }
    }
    while (OS_ERROR_NOT_INITIALIZED == err);

    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkSocket_create() failed, code %d", err);
        return -1;
    }

    const OS_NetworkSocket_Addr_t dstAddr =
    {
        .addr = OS_INADDR_ANY_STR,
        .port = CFG_TCP_SERVER_PORT
    };

    err = OS_NetworkSocket_bind(
              srvHandle,
              &dstAddr);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkSocket_bind() failed, code %d", err);
        OS_NetworkSocket_close(srvHandle);
        nb_helper_reset_ev_struct_for_socket(srvHandle);
        return -1;
    }

    int backlog = 10;

    err = OS_NetworkSocket_listen(
              srvHandle,
              backlog);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkSocket_listen() failed, code %d", err);
        OS_NetworkSocket_close(srvHandle);
        nb_helper_reset_ev_struct_for_socket(srvHandle);
        return -1;
    }

    Debug_LOG_INFO("launching echo server");

    /* Gets filled when accept is called */
    OS_NetworkSocket_Handle_t clientHandle;
    OS_NetworkSocket_Addr_t srcAddr = {0};

    static char buffer[4096];

    for (;;)
    {
        do
        {
            // Wait until we get an conn acpt event for the listening socket.
            nb_helper_wait_for_conn_acpt_ev_on_socket(srvHandle);

            err = OS_NetworkSocket_accept(
                      srvHandle,
                      &clientHandle,
                      &srcAddr);
        }
        while (err == OS_ERROR_TRY_AGAIN);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_NetworkSocket_accept() failed, error %d", err);
            OS_NetworkSocket_close(srvHandle);
            nb_helper_reset_ev_struct_for_socket(srvHandle);
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
            do
            {
                // Wait until we receive a read event for the socket.
                nb_helper_wait_for_read_ev_on_socket(clientHandle);

                DECL_UNUSED_VAR(int t1) = SharedResourceMutex_lock();
                err = OS_NetworkSocket_read(
                          clientHandle,
                          buffer,
                          sizeof(buffer),
                          &n);
                DECL_UNUSED_VAR(int t2) = SharedResourceMutex_unlock();
            }
            while (err == OS_ERROR_TRY_AGAIN);

            if (OS_SUCCESS != err)
            {
                Debug_LOG_ERROR("OS_NetworkSocket_read() failed, error %d", err);
                break;
            }

            size_t bytesWritten      = 0;
            size_t totalBytesWritten = 0;

            while (totalBytesWritten < n)
            {
                do
                {
                    DECL_UNUSED_VAR(int t3) =  SharedResourceMutex_lock();
                    err = OS_NetworkSocket_write(
                              clientHandle,
                              &buffer[totalBytesWritten],
                              n - totalBytesWritten,
                              &bytesWritten);
                    DECL_UNUSED_VAR(int t4) = SharedResourceMutex_unlock();
                }
                while (err == OS_ERROR_TRY_AGAIN);
                if (err != OS_SUCCESS)
                {
                    Debug_LOG_ERROR("OS_NetworkSocket_write() failed, error %d", err);
                    break;
                }
                totalBytesWritten = totalBytesWritten + bytesWritten;
            }
        }

        switch (err)
        {
        /* This means end of read as socket was closed. Exit now and close
         * handle*/
        case OS_ERROR_NETWORK_CONN_SHUTDOWN:
            // the test runner checks for this string
            Debug_LOG_INFO("connection closed by server");
            OS_NetworkSocket_close(clientHandle);
            nb_helper_reset_ev_struct_for_socket(clientHandle);
            continue;
        /* Any other value is a failure in read, hence exit and close handle  */
        default:
            Debug_LOG_ERROR("server socket failure, error %d", err);
            OS_NetworkSocket_close(clientHandle);
            nb_helper_reset_ev_struct_for_socket(clientHandle);
            continue;
        } // end of switch
    }
    return -1;
}
