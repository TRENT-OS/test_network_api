/*
 *  OS Network Stack CAmkES App as client
 *
 *  Copyright (C) 2019, Hensoldt Cyber GmbH
 *
 */

#include "LibDebug/Debug.h"
#include "OS_Error.h"
#include "stdint.h"
#include <string.h>
#include "system_config.h"

#include "OS_Network.h"
#include "math.h"

#include "OS_NetworkStackClient.h"
#include "util/loop_defines.h"
#include <camkes.h>

extern OS_Error_t
OS_NetworkAPP_RT(OS_Network_Context_t ctx);

static OS_NetworkStackClient_SocketDataports_t config;


/*
    This example demonstrates reading of a web page example.com using Nw Stack
   API. Currently only a single socket is supported per stack instance. i.e. no
   multitasking is supported as of now.

*/

#define HTTP_PORT 80

OS_Error_t
test_tcp_client()
{
    char buffer[2048];
    OS_Network_Socket_t cli_socket = { .domain = OS_AF_INET,
                                       .type   = OS_SOCK_STREAM,
                                       .name   = "10.0.0.1",
                                       .port   = 80 };

    /* This creates a socket API and gives an handle which can be used
       for further communication. */
    OS_NetworkSocket_Handle_t handle[OS_NETWORK_MAXIMUM_SOCKET_NO];
    OS_Error_t err;
    int socket_max = 0;
    int i;
    for (i = 0; i < OS_NETWORK_MAXIMUM_SOCKET_NO; i++)
    {
        err = OS_NetworkSocket_create(NULL, &cli_socket, &handle[i]);

        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("client_socket_create() failed, code %d for %d socket", err,i);
            break;
        }
    }
    socket_max = i;
    Debug_LOG_INFO("Send request to host...");

    char* request = "GET /network/a.txt "
                          "HTTP/1.0\r\nHost: " CFG_TEST_HTTP_SERVER
                          "\r\nConnection: close\r\n\r\n";

    const size_t len_request = strlen(request);
    size_t       len         = len_request;

    /* Send the request to the host */
    for (int i = 0; i < socket_max ; i++)
    {
        size_t offs = 0;
        Debug_LOG_INFO("Writing request to socket %d for %.*s", i, 17, request);
        request[13] = 'a' + i;
        do
        {
            const size_t lenRemaining = len_request - offs;
            size_t       len_io       = lenRemaining;

            err = OS_NetworkSocket_write(handle[i], &request[offs], len_io, &len_io);

            if (err != OS_SUCCESS)
            {
                Debug_LOG_ERROR("socket_write() failed, code %d", err);
                OS_NetworkSocket_close(handle[i]);
                return OS_ERROR_GENERIC;
            }

            /* fatal error, this must not happen. API broken*/
            Debug_ASSERT(len_io <= lenRemaining);

            offs += len_io;
        } while (offs < len_request);
    }
    Debug_LOG_INFO("read response...");

    /*
    As of now the nw stack behavior is as below:
    Keep reading data until you receive one of the return values:
     a. err = OS_ERROR_CONNECTION_CLOSED indicating end of data read
              and connection close
     b. err = OS_ERROR_GENERIC  due to error in read
     c. err = OS_SUCCESS and length = 0 indicating no data to read but there
    is still connection
     d. err = OS_SUCCESS and length >0 , valid data

    Take appropriate actions based on the return value rxd.

    Only a single socket is supported and no multithreading !!!
    Once a webpage is read , display the contents.
    */

    int flag = 0;

    do
    {
        for (int i = 0; i < socket_max ; i++)
        {
            len = sizeof(buffer);
            /* Keep calling read until we receive CONNECTION_CLOSED from the
            stack */
            memset(buffer, 0, sizeof(buffer));
            OS_Error_t err = OS_ERROR_CONNECTION_CLOSED;
            if (!(flag & (1 << i)))
                err = OS_NetworkSocket_read(handle[i], buffer, len, &len);
            switch (err)
            {

            /* This means end of read or nothing further to read as socket was
            * closed */
            case OS_ERROR_CONNECTION_CLOSED:
                Debug_LOG_INFO("socket_read() reported connection closed for handle %d", i);
                flag |= 1<<i; /* terminate loop and close handle*/
                break;

            /* Success . continue further reading */
            case OS_SUCCESS:
                Debug_LOG_INFO("chunk read, length %d, handle %d", len, i);
                continue;

            /* Error case, break and close the handle */
            default:
                Debug_LOG_INFO("socket_read() failed for handle %d, error %d", i, err);
                flag |= 1<<i; /* terminate loop and close handle */
                break;
            } // end of switch
        }
    } while (flag != pow(2,socket_max)-1);
    Debug_LOG_INFO("Test ended");

    for (int i = 0; i < socket_max ; i++)
    {
        /* Close the socket communication */
        err = OS_NetworkSocket_close(handle[i]);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("close() failed for handle %d, code %d", i, err);
            return OS_ERROR_GENERIC;
        }
    }
    return OS_SUCCESS;
}

OS_Error_t
test_udp_recvfrom()
{
    char buffer[4096];
    size_t len;

    OS_NetworkSocket_Handle_t handle;


    OS_Network_Socket_t udp_socket = { .domain = OS_AF_INET,
                                       .type   = OS_SOCK_DGRAM,
                                       .name   = "10.0.0.10",
                                       .port   = 8888 };

    OS_Network_Socket_t receive_udp_socket = udp_socket;

    OS_Error_t err = OS_NetworkSocket_create(NULL, &udp_socket, &handle);

    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("client_socket_create() failed, code %d", err);
        return OS_ERROR_GENERIC;
    }

    err = OS_NetworkSocket_bind(handle, udp_socket.port);

    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("bind() failed, code %d", err);
        return OS_ERROR_GENERIC;
    }

    len = sizeof(buffer);
    Debug_LOG_INFO("UDP Receive test handle: %d", handle);
    err = OS_NetworkSocket_recvfrom(handle, buffer, len, &len, &receive_udp_socket);

    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("recvfrom() failed, code %d", err);
        return OS_ERROR_GENERIC;
    }

    Debug_LOG_INFO("Received %d \"%*s\" : %s %d\n", len, len, buffer,
                        receive_udp_socket.name, receive_udp_socket.port);

    err = OS_NetworkSocket_close(handle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("close() failed, code %d", err);
        return OS_ERROR_GENERIC;
    }
    return OS_SUCCESS;
}

OS_Error_t
test_udp_sendto()
{
    char buffer[4096];
    char test_message[] = "Hello there";
    size_t len;

    OS_NetworkSocket_Handle_t handle;


    OS_Network_Socket_t udp_socket = { .domain = OS_AF_INET,
                                       .type   = OS_SOCK_DGRAM,
                                       .name   = "10.0.0.10",
                                       .port   = 8888 };

    OS_Network_Socket_t receive_udp_socket = udp_socket;

    OS_Error_t err = OS_NetworkSocket_create(NULL, &udp_socket, &handle);

    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("client_socket_create() failed, code %d", err);
        return OS_ERROR_GENERIC;
    }

    err = OS_NetworkSocket_bind(handle, udp_socket.port);

    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("bind() failed, code %d", err);
        return OS_ERROR_GENERIC;
    }

    len = sizeof(buffer);
    Debug_LOG_INFO("UDP Send test");
    err = OS_NetworkSocket_recvfrom(handle, buffer,len, &len, &receive_udp_socket);

    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("recvfrom() failed, code %d", err);
        return OS_ERROR_GENERIC;
    }

    Debug_LOG_INFO("Received %d \"%*s\" : %s %d\n", len, len, buffer,
                         receive_udp_socket.name, receive_udp_socket.port);

    len = sizeof(test_message);
    err = OS_NetworkSocket_sendto(handle, test_message, len, &len, receive_udp_socket);

    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("client_socket_create() failed, code %d", err);
        return OS_ERROR_GENERIC;
    }

    err = OS_NetworkSocket_close(handle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("close() failed, code %d", err);
        return OS_ERROR_GENERIC;
    }

    return OS_SUCCESS;
}

void init_client_api()
{
    config.number_of_sockets = OS_NETWORK_MAXIMUM_SOCKET_NO;
    static OS_Dataport_t dataports[OS_NETWORK_MAXIMUM_SOCKET_NO] = {0};


    int i = 0;

#define LOOP_COUNT OS_NETWORK_MAXIMUM_SOCKET_NO
#define LOOP_ELEMENT                                                     \
    GEN_ID(OS_Dataport_t t) = OS_DATAPORT_ASSIGN(GEN_ID(NwAppDataPort));         \
    dataports[i] = GEN_ID(t);                                            \
    i++;
#include "util/loop.h"

    config.dataport = dataports;
    OS_NetworkStackClient_init(&config);
}

OS_Error_t
test_dataport_size_check_client_functions()
{
    char buffer[4096];
    OS_Network_Socket_t udp_socket;
    OS_NetworkSocket_Handle_t handle = 0;


    const OS_Dataport_t dp = config.dataport[handle];
    size_t len = OS_Dataport_getSize(dp) + 1;

    if(OS_NetworkSocket_read(handle, buffer, len, NULL) != OS_ERROR_INVALID_PARAMETER )
    {
        Debug_LOG_ERROR("Client socket read with invalid dataport size failed");
        return OS_ERROR_GENERIC;
    }

    if(OS_NetworkSocket_recvfrom(handle, buffer, len, NULL, &udp_socket) != OS_ERROR_INVALID_PARAMETER )
    {
        Debug_LOG_ERROR("Client socket recvfrom with invalid dataport size failed");
        return OS_ERROR_GENERIC;
    }

    if(OS_NetworkSocket_write(handle, buffer, len, NULL) != OS_ERROR_INVALID_PARAMETER)
    {
        Debug_LOG_ERROR("Client socket write with invalid dataport size failed");
        return OS_ERROR_GENERIC;
    }

    if(OS_NetworkSocket_sendto(handle, buffer, len, NULL, udp_socket) != OS_ERROR_INVALID_PARAMETER)
    {
        Debug_LOG_ERROR("Client socket sendto with invalid dataport size failed");
        return OS_ERROR_GENERIC;
    }

    return OS_SUCCESS;
}

OS_Error_t
test_dataport_size_check_lib_functions()
{
    OS_NetworkSocket_Handle_t handle;
    OS_Network_Socket_t udp_socket = { .domain = OS_AF_INET,
                                       .type   = OS_SOCK_DGRAM,
                                       .name   = "10.0.0.10",
                                       .port   = 8888 };

    OS_Error_t err = OS_NetworkSocket_create(NULL, &udp_socket, &handle);

    const OS_Dataport_t dp = config.dataport[handle];
    size_t len = OS_Dataport_getSize(dp) + 1;


    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("client_socket_create() failed, code %d", err);
        return OS_ERROR_GENERIC;
    }

    if(network_stack_rpc_socket_read(handle, &len) != OS_ERROR_INVALID_PARAMETER )
    {
        Debug_LOG_ERROR("Lib socket read with invalid dataport size failed");
        return OS_ERROR_GENERIC;
    }
    if(network_stack_rpc_socket_recvfrom(handle, &len, &udp_socket) != OS_ERROR_INVALID_PARAMETER )
    {
        Debug_LOG_ERROR("Lib socket recvfrom with invalid dataport size failed");
        return OS_ERROR_GENERIC;
    }
    if(network_stack_rpc_socket_write(handle, &len) != OS_ERROR_INVALID_PARAMETER )
    {
        Debug_LOG_ERROR("Lib socket write with invalid dataport size failed");
        return OS_ERROR_GENERIC;
    }
    if(network_stack_rpc_socket_sendto(handle, &len, udp_socket) != OS_ERROR_INVALID_PARAMETER )
    {
        Debug_LOG_ERROR("Lib socket sendto with invalid dataport size failed");
        return OS_ERROR_GENERIC;
    }

    err = OS_NetworkSocket_close(handle);

    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("close() failed, code %d", err);
        return OS_ERROR_GENERIC;
    }

    return OS_SUCCESS;
}

int
run()
{
    init_client_api();
    Debug_LOG_INFO("Starting test_app_client...");

    OS_NetworkAPP_RT(NULL); // Must be actually called by OS Runtime

    Debug_LOG_INFO("TCP client test");
    if(test_tcp_client() == OS_SUCCESS)
    {
        Debug_LOG_INFO("TCP client test successful.");
    }
    else
    {
        Debug_LOG_ERROR("TCP client test failed.");
    }

    if(test_udp_recvfrom() == OS_SUCCESS)
    {
        Debug_LOG_INFO("UDP recvfrom test successful.");
    }
    else
    {
        Debug_LOG_ERROR("UDP recvfrom test failed.");
    }

    if(test_udp_sendto() == OS_SUCCESS)
    {
        Debug_LOG_INFO("UDP sendto test successful.");
    }
    else
    {
        Debug_LOG_ERROR("UDP sendto test failed.");
    }

    if(test_dataport_size_check_client_functions() == OS_SUCCESS)
    {
        Debug_LOG_INFO("Client dataport test successful.");
    }
    else
    {
        Debug_LOG_ERROR("Client dataport test failed.");
    }

    if(test_dataport_size_check_lib_functions() == OS_SUCCESS)
    {
        Debug_LOG_INFO("Lib dataport test successful.");
    }
    else
    {
        Debug_LOG_ERROR("Lib dataport test failed.");
    }

    return 0;
}
