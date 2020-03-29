/*
 *  SEOS Network Stack CAmkES App as client
 *
 *  Copyright (C) 2019, Hensoldt Cyber GmbH
 *
 */

#include <string.h>
#include "stdint.h"
#include "LibDebug/Debug.h"
#include "SeosError.h"

#include "OS_Network.h"

extern seos_err_t OS_NetworkAPP_RT(OS_Network_context_t ctx);

/*
    This example demonstrates reading of a web page example.com using Nw Stack API.
    Currently only a single socket is supported per stack instance.
    i.e. no multitasking is supported as of now.

*/

#define HTTP_PORT 80

int run()
{
    Debug_LOG_INFO("Starting test_app_client...");

    char buffer[4096];
    OS_NetworkAPP_RT(NULL);   // Must be actullay called by SEOS Runtime

    OS_NetworkClient_socket_t cli_socket =
    {
        .domain = OS_AF_INET,
        .type   = OS_SOCK_STREAM,
        .name   = CFG_TEST_HTTP_SERVER,
        .port   = HTTP_PORT
    };

    /* This creates a socket API and gives an handle which can be used
       for further communication. */
    OS_NetworkSocket_handle_t handle;
    seos_err_t err = OS_NetworkClientSocket_create(NULL, &cli_socket, &handle);

    if (err != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("client_socket_create() failed, code %d", err);
        return -1;
    }

    Debug_LOG_INFO("Send request to host...");

    const char* request =
        "GET / HTTP/1.0\r\nHost: 192.168.82.12\r\nConnection: close\r\n\r\n";

    const size_t len_request = strlen(request);
    size_t len = len_request;

    /* Send the request to the host */
    size_t offs = 0;
    do
    {
        const size_t lenRemaining = len_request - offs;
        size_t len_io = lenRemaining;


        err = OS_NetworkSocket_write(handle, &request[offs], &len_io);

        if (err != SEOS_SUCCESS)
        {
            Debug_LOG_ERROR("socket_write() failed, code %d", err);
            OS_NetworkSocket_close(handle);
            return -1;
        }

        /* fatal error, this must not happen. API broken*/
        Debug_ASSERT( len_io <= lenRemaining );

        offs += len_io;
    }
    while (offs < len_request);

    Debug_LOG_INFO("read response...");

    /*
    As of now the nw stack behavior is as below:
    Keep reading data until you receive one of the return values:
     a. err = SEOS_ERROR_CONNECTION_CLOSED indicating end of data read
              and connection close
     b. err = SEOS_ERROR_GENERIC  due to error in read
     c. err = SEOS_SUCCESS and length = 0 indicating no data to read but there is still
              connection
     d. err = SEOS_SUCCESS and length >0 , valid data

    Take appropriate actions based on the return value rxd.



    Only a single socket is supported and no multithreading !!!
    Once a webpage is read , display the contents.
    */

    int flag = true;

    do
    {
        len = sizeof(buffer);

        /* Keep calling read until we receive CONNECTION_CLOSED from the stack */
        memset(buffer, 0, sizeof(buffer));

        seos_err_t err = OS_NetworkSocket_read(handle, buffer, &len);

        switch (err)
        {

        /* This means end of read or nothing further to read as socket was closed */
        case SEOS_ERROR_CONNECTION_CLOSED:
            Debug_LOG_INFO("socket_read() reported connection closed");
            flag = false;    /* terminate loop and close handle*/
            break;

        /* Success . continue further reading */
        case SEOS_SUCCESS:
            Debug_LOG_INFO("chunk read, length %d", len);
            continue ;

        /* Error case, break and close the handle */
        default:
            Debug_LOG_INFO("socket_read() failed, error %d", err);
            flag = false;    /* terminate loop and close handle */
            break;
        }// end of switch
    }
    while (flag);
    Debug_LOG_INFO("Test ended");
    /* Close the socket communication */
    err = OS_NetworkSocket_close(handle);
    return 0;
}
