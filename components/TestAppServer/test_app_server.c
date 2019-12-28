/*
 *  SEOS Network Stack CAmkES App as Server
 *
 *  Copyright (C) 2019, Hensoldt Cyber GmbH
 *
 */
#include <string.h>
#include "stdint.h"
#include "LibDebug/Debug.h"
#include "SeosError.h"

#include "seos_nw_api.h"

/*
 * This example demonstrates a server with an incoming connection. Reads
 *  incoming data afterconnection is established. Writes or echoes the received
 *  data back to the client. When the connection with the client is closed a new
 *  round begins, in an infinitive loop.
 *  Currently only a single socket is supported per stack instance.
 *  i.e. no multitasking is supported as of now.
*/


extern seos_err_t Seos_NwAPP_RT(Seos_nw_context ctx);


int run()
{
    Debug_LOG_INFO("Starting test_app_server...");

    char buffer[4096];
    Seos_NwAPP_RT(NULL);    /* Must be actullay called by SEOS Runtime */

    seos_nw_server_struct  srv_socket =
    {
        .domain = SEOS_AF_INET,
        .type   = SEOS_SOCK_STREAM,
        .listen_port = 5555,
        .backlog = 1,
    };

    /* Gets filled when accept is called */
    seos_socket_handle_t  seos_socket_handle ;
    /* Gets filled when server socket create is called */
    seos_nw_server_handle_t  seos_nw_server_handle ;

    seos_err_t err = Seos_server_socket_create(NULL, &srv_socket,
                                               &seos_nw_server_handle);

    if (err != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("server_socket_create() failed, code %d", err);
        return -1;
    }

    Debug_LOG_INFO("launching echo server");

    for(;;)
    {
        err = Seos_socket_accept(seos_nw_server_handle, &seos_socket_handle);
        if (err != SEOS_SUCCESS)
        {
            Debug_LOG_ERROR("socket_accept() failed, error %d", err);
            return -1;
        }

        /*
            As of now the nw stack behavior is as below:
            Keep reading data until you receive one of the return values:
             a. err = SEOS_ERROR_CONNECTION_CLOSED and length = 0 indicating end of data read
                      and connection close
             b. err = SEOS_ERROR_GENERIC  due to error in read
             c. err = SEOS_SUCCESS and length = 0 indicating no data to read but there is still
                      connection
             d. err = SEOS_SUCCESS and length >0 , valid data

            Take appropriate actions based on the return value rxd.


            Only a single socket is supported and no multithreading !!!
            Once we accept an incoming connection, start reading data from the client and echo back
            the data rxd.
        */
        memset(buffer, 0, sizeof(buffer));

        Debug_LOG_INFO("starting server read loop");
        /* Keep calling read until we receive CONNECTION_CLOSED from the stack */
        for(;;)
        {
            Debug_LOG_DEBUG("read...");
            size_t n = 1;
            err = Seos_socket_read(seos_socket_handle, buffer, &n);
            if (SEOS_SUCCESS != err)
            {
                Debug_LOG_ERROR("socket_read() failed, error %d", err);
                break;
            }

            Debug_ASSERT(n == 1);
            Debug_LOG_DEBUG("Got a byte %02x, send it back", buffer[0]);

            err = Seos_socket_write(seos_socket_handle, buffer, &n);
            if (err != SEOS_SUCCESS)
            {
                Debug_LOG_ERROR("socket_write() failed, error %d", err);
                break;
            }
        }

        switch (err)
        {
        /* This means end of read as socket was closed. Exit now and close handle*/
        case SEOS_ERROR_CONNECTION_CLOSED:
            // the test runner checks for this string
            Debug_LOG_INFO("connection closed by server");
            break;
        /* Any other value is a failure in read, hence exit and close handle  */
        default :
            Debug_LOG_ERROR("server socket failure, error %d", err);
            break;
        } //end of switch
    }
    return -1;
}
