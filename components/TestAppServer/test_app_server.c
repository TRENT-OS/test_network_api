/*
 *  OS Network Stack CAmkES App as Server
 *
 *  Copyright (C) 2019, Hensoldt Cyber GmbH
 *
 */
#include <string.h>
#include "stdint.h"
#include "LibDebug/Debug.h"
#include "OS_Error.h"

#include "OS_Network.h"
#include "OS_NetworkStackClient.h"
#include "util/loop_defines.h"
#include <camkes.h>

/*
 * This example demonstrates a server with an incoming connection. Reads
 *  incoming data afterconnection is established. Writes or echoes the received
 *  data back to the client. When the connection with the client is closed a new
 *  round begins, in an infinitive loop.
 *  Currently only a single socket is supported per stack instance.
 *  i.e. no multitasking is supported as of now.
*/


extern OS_Error_t OS_NetworkAPP_RT(OS_Network_Context_t ctx);

void init_client_api()
{
    static os_network_dataports_socket_t config;

    config.number_of_sockets = OS_NETWORK_MAXIMUM_SOCKET_NO;
    static OS_Dataport_t dataports[OS_NETWORK_MAXIMUM_SOCKET_NO] = {0};


    int i = 0;

#define LOOP_COUNT OS_NETWORK_MAXIMUM_SOCKET_NO
#define LOOP_ELEMENT                                                     \
    GEN_ID(OS_Dataport_t t) = OS_DATAPORT_ASSIGN(GEN_ID(NwAppDataPort)); \
    dataports[i] = GEN_ID(t);                                            \
    i++;
#include "util/loop.h"

    config.dataport = dataports;
    OS_NetworkStackClient_init(&config);
}

int
run()
{
    init_client_api();

    Debug_LOG_INFO("Starting test_app_server...");

    char buffer[4096];
    OS_NetworkAPP_RT(NULL);    /* Must be actually called by OS Runtime */

    OS_NetworkServer_Socket_t  srv_socket =
    {
        .domain = OS_AF_INET,
        .type   = OS_SOCK_STREAM,
        .listen_port = 5555,
        .backlog = 1,
    };

    /* Gets filled when accept is called */
    OS_NetworkSocket_Handle_t  seos_socket_handle ;
    /* Gets filled when server socket create is called */
    OS_NetworkServer_Handle_t  seos_nw_server_handle ;

    OS_Error_t err = OS_NetworkServerSocket_create(NULL, &srv_socket,
                                                   &seos_nw_server_handle);

    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("server_socket_create() failed, code %d", err);
        return -1;
    }

    Debug_LOG_INFO("launching echo server");

    for (;;)
    {
        err = OS_NetworkServerSocket_accept(seos_nw_server_handle, &seos_socket_handle);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("socket_accept() failed, error %d", err);
            return -1;
        }

        /*
            As of now the nw stack behavior is as below:
            Keep reading data until you receive one of the return values:
             a. err = OS_ERROR_CONNECTION_CLOSED and length = 0 indicating end of data read
                      and connection close
             b. err = OS_ERROR_GENERIC  due to error in read
             c. err = OS_SUCCESS and length = 0 indicating no data to read but there is still
                      connection
             d. err = OS_SUCCESS and length >0 , valid data

            Take appropriate actions based on the return value rxd.


            Only a single socket is supported and no multithreading !!!
            Once we accept an incoming connection, start reading data from the client and echo back
            the data rxd.
        */
        memset(buffer, 0, sizeof(buffer));

        Debug_LOG_INFO("starting server read loop");
        /* Keep calling read until we receive CONNECTION_CLOSED from the stack */
        for (;;)
        {
            Debug_LOG_DEBUG("read...");
            size_t n = 1;
            err = OS_NetworkSocket_read(seos_socket_handle, buffer, &n);
            if (OS_SUCCESS != err)
            {
                Debug_LOG_ERROR("socket_read() failed, error %d", err);
                break;
            }

            Debug_ASSERT(n == 1);
            Debug_LOG_DEBUG("Got a byte %02x, send it back", buffer[0]);

            err = OS_NetworkSocket_write(seos_socket_handle, buffer, &n);
            if (err != OS_SUCCESS)
            {
                Debug_LOG_ERROR("socket_write() failed, error %d", err);
                break;
            }
        }

        switch (err)
        {
        /* This means end of read as socket was closed. Exit now and close handle*/
        case OS_ERROR_CONNECTION_CLOSED:
            // the test runner checks for this string
            Debug_LOG_INFO("connection closed by server");
            OS_NetworkSocket_close(seos_socket_handle);
            break;
        /* Any other value is a failure in read, hence exit and close handle  */
        default :
            Debug_LOG_ERROR("server socket failure, error %d", err);
            break;
        } //end of switch
    }
    return -1;
}
