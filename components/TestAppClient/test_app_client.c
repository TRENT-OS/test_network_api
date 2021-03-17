/*
 *  OS Network Stack CAmkES App as client
 *
 *  Copyright (C) 2019, Hensoldt Cyber GmbH
 *
 */

#include "lib_debug/Debug.h"
#include "lib_macros/Test.h"
#include "OS_Error.h"
#include "stdint.h"
#include "system_config.h"
#include <string.h>

#include "OS_Network.h"
#include "math.h"

#include "OS_NetworkStackClient.h"
#include "SysLoggerClient.h"
#include "util/loop_defines.h"
#include <camkes.h>

static OS_NetworkStackClient_SocketDataports_t config = {
    .number_of_sockets = OS_NETWORK_MAXIMUM_SOCKET_NO
};

/*
    This example demonstrates reading of a web page example.com using Nw Stack
   API. Currently only a single socket is supported per stack instance. i.e. no
   multitasking is supported as of now.

*/

#define HTTP_PORT 80

void
test_socket_create_neg()
{
    // This test will try to create sockets with invalid parameters. After this
    // it will try to open a socket to a service which is not reachable (closed
    // port) and check that the create() fails
    TEST_START();

    OS_NetworkSocket_Handle_t   handle[OS_NETWORK_MAXIMUM_SOCKET_NO];
    OS_Error_t                  err;
    OS_Network_Socket_t         socket =
    {
        .domain = OS_AF_INET,
        .type   = OS_SOCK_STREAM,
        .name   = CFG_REACHABLE_HOST,
        .port   = CFG_REACHABLE_PORT
    };

    // Test unsupported domain
    socket.domain = 0;
    for (int i = 0; i < OS_NETWORK_MAXIMUM_SOCKET_NO; i++)
    {
        err = OS_NetworkSocket_create(NULL, &socket, &handle[i]);
        ASSERT_EQ_OS_ERR(OS_ERROR_NETWORK_PROTO_NO_SUPPORT, err);
    }
    socket.domain = OS_AF_INET;

    // Test unsupported type
    socket.type = 0;
    for (int i = 0; i < OS_NETWORK_MAXIMUM_SOCKET_NO; i++)
    {
        err = OS_NetworkSocket_create(NULL, &socket, &handle[i]);
        ASSERT_EQ_OS_ERR(OS_ERROR_NETWORK_PROTO_NO_SUPPORT, err);
    }
    socket.type = OS_SOCK_STREAM;

    // Test invalid destination address
    memset(socket.name, 0, sizeof(socket.name));
    for (int i = 0; i < OS_NETWORK_MAXIMUM_SOCKET_NO; i++)
    {
        err = OS_NetworkSocket_create(NULL, &socket, &handle[i]);
        ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_PARAMETER, err);
    }

    // Test unreachable host
    strncpy(socket.name, CFG_UNREACHABLE_HOST, sizeof(socket.name));
    for (int i = 0; i < OS_NETWORK_MAXIMUM_SOCKET_NO; i++)
    {
        err = OS_NetworkSocket_create(NULL, &socket, handle);
        ASSERT_EQ_OS_ERR(OS_ERROR_NETWORK_HOST_UNREACHABLE, err);
    }
    /*
    This test is momentarily suppressed as it seems that picotcp is not
    behaving correctly. There is need of further investigations to establish
    whether is an issue with iptables or with picotcp

    // Test forbidden host (connection reset),
    // firewall is configured to reset TCP for this host
    strncpy(socket.name, CFG_FORBIDDEN_HOST, sizeof(socket.name));
    for (int i = 0; i < OS_NETWORK_MAXIMUM_SOCKET_NO; i++)
    {
        err = OS_NetworkSocket_create(NULL, &socket, handle);
        ASSERT_EQ_OS_ERR(OS_ERROR_NETWORK_CONN_RESET, err);
    }
    */
    // Test connection refused,
    // now with reachable address but port closed. We do not loop this test
    // for convenience: it would take too much time due to the connection
    // timeouts
    strncpy(socket.name, CFG_REACHABLE_HOST, sizeof(socket.name));
    socket.port = CFG_UNREACHABLE_PORT;
    err = OS_NetworkSocket_create(NULL, &socket, handle);
    ASSERT_EQ_OS_ERR(OS_ERROR_NETWORK_CONN_REFUSED, err);

    // test out of resources
    socket.port = CFG_REACHABLE_PORT;
    for (int i = 0; i < OS_NETWORK_MAXIMUM_SOCKET_NO; i++)
    {
        err = OS_NetworkSocket_create(NULL, &socket, &handle[i]);
        ASSERT_EQ_OS_ERR(OS_SUCCESS, err);
    }
    OS_NetworkSocket_Handle_t exceedingHandle;
    err = OS_NetworkSocket_create(NULL, &socket, &exceedingHandle);
    ASSERT_EQ_OS_ERR(OS_ERROR_INSUFFICIENT_SPACE, err);

    // cleanup
    for (int i = 0; i < OS_NETWORK_MAXIMUM_SOCKET_NO; i++)
    {
        OS_Error_t err = OS_NetworkSocket_close(handle[i]);
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
    OS_Network_Socket_t cli_socket  =
    {
        .domain = OS_AF_INET,
        .type   = OS_SOCK_STREAM,
        .name   = "10.0.0.1",
        .port   = 80
    };

    for (int i = 0; i < 2; i++)
    {   // let the following run twice in order to try to catch possible
        // production of RAII garbage

        for (int i = 0; i < OS_NETWORK_MAXIMUM_SOCKET_NO; i++)
        {
            OS_Error_t err = OS_NetworkSocket_create(NULL, &cli_socket, &handle[i]);
            ASSERT_EQ_OS_ERR(OS_SUCCESS, err);
        }

        for (int i = 0; i < OS_NETWORK_MAXIMUM_SOCKET_NO; i++)
        {
            OS_Error_t err = OS_NetworkSocket_close(handle[i]);
            ASSERT_EQ_OS_ERR(OS_SUCCESS, err);
        }
    }

    TEST_FINISH();
}

void
test_tcp_client()
{
    TEST_START();

    char                buffer[2048];
    OS_Network_Socket_t cli_socket = { .domain = OS_AF_INET,
                                       .type   = OS_SOCK_STREAM,
                                       .name   = "10.0.0.1",
                                       .port   = 80 };

    /* This creates a socket API and gives an handle which can be used
       for further communication. */
    OS_NetworkSocket_Handle_t handle[OS_NETWORK_MAXIMUM_SOCKET_NO];
    OS_Error_t                err;
    int                       socket_max = 0;
    int                       i;
    for (i = 0; i < OS_NETWORK_MAXIMUM_SOCKET_NO; i++)
    {
        err = OS_NetworkSocket_create(NULL, &cli_socket, &handle[i]);

        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR(
                "client_socket_create() failed, code %d for %d socket",
                err,
                i);
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
    for (int i = 0; i < socket_max; i++)
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
                Debug_LOG_ERROR("socket_write() failed, code %d", err);
                OS_NetworkSocket_close(handle[i]);
                return;
            }

            /* fatal error, this must not happen. API broken*/
            ASSERT_LE_SZ(len_io, lenRemaining);

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
        for (int i = 0; i < socket_max; i++)
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
                Debug_LOG_INFO(
                    "socket_read() reported connection closed for handle %d",
                    i);
                flag |= 1 << i; /* terminate loop and close handle*/
                break;

            /* Success . continue further reading */
            case OS_SUCCESS:
                Debug_LOG_INFO("chunk read, length %d, handle %d", len, i);
                continue;

            /* Error case, break and close the handle */
            default:
                Debug_LOG_INFO(
                    "socket_read() failed for handle %d, error %d",
                    i,
                    err);
                flag |= 1 << i; /* terminate loop and close handle */
                break;
            } // end of switch
        }
    } while (flag != pow(2, socket_max) - 1);
    Debug_LOG_INFO("Test ended");

    for (int i = 0; i < socket_max; i++)
    {
        /* Close the socket communication */
        err = OS_NetworkSocket_close(handle[i]);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("close() failed for handle %d, code %d", i, err);
            return;
        }
    }

    TEST_FINISH();
}

void
test_udp_recvfrom()
{
    TEST_START();

    // Buffer big enough to hold 2 frames, rounded to the nearest power of 2
    static char buffer[4096];
    size_t      len;

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
        return;
    }

    err = OS_NetworkSocket_bind(handle, udp_socket.port);

    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("bind() failed, code %d", err);
        return;
    }

    len = sizeof(buffer);
    Debug_LOG_INFO("UDP Receive test handle: %d", handle);
    err = OS_NetworkSocket_recvfrom(
        handle,
        buffer,
        len,
        &len,
        &receive_udp_socket);

    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("recvfrom() failed, code %d", err);
        return;
    }

    Debug_LOG_INFO(
        "Received %d \"%*s\" : %s %d\n",
        len,
        len,
        buffer,
        receive_udp_socket.name,
        receive_udp_socket.port);

    err = OS_NetworkSocket_close(handle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("close() failed, code %d", err);
        return;
    }

    TEST_FINISH();
}

void
test_udp_sendto()
{
    TEST_START();

    // Buffer big enough to hold 2 frames, rounded to the nearest power of 2
    static char buffer[4096];
    char        test_message[] = "Hello there";
    size_t      len;

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
        return;
    }

    err = OS_NetworkSocket_bind(handle, udp_socket.port);

    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("bind() failed, code %d", err);
        return;
    }

    len = sizeof(buffer);
    Debug_LOG_INFO("UDP Send test");
    err = OS_NetworkSocket_recvfrom(
        handle,
        buffer,
        len,
        &len,
        &receive_udp_socket);

    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("recvfrom() failed, code %d", err);
        return;
    }

    Debug_LOG_INFO(
        "Received %d \"%*s\" : %s %d\n",
        len,
        len,
        buffer,
        receive_udp_socket.name,
        receive_udp_socket.port);

    len = sizeof(test_message);
    err = OS_NetworkSocket_sendto(
        handle,
        test_message,
        len,
        &len,
        receive_udp_socket);

    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("client_socket_create() failed, code %d", err);
        return;
    }

    err = OS_NetworkSocket_close(handle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("close() failed, code %d", err);
        return;
    }

    TEST_FINISH();
}

void
init_client_api()
{
    static OS_Dataport_t dataports[OS_NETWORK_MAXIMUM_SOCKET_NO] = { 0 };

    int i = 0;

#define LOOP_COUNT OS_NETWORK_MAXIMUM_SOCKET_NO
#define LOOP_ELEMENT                                                           \
    GEN_ID(OS_Dataport_t t) = OS_DATAPORT_ASSIGN(GEN_ID(NwAppDataPort));       \
    dataports[i]            = GEN_ID(t);                                       \
    i++;
#include "util/loop.h"

    config.dataport = dataports;
    OS_NetworkStackClient_init(&config);
}

void
test_dataport_size_check_client_functions()
{
    TEST_START();

    // Buffer big enough to hold 2 frames, rounded to the nearest power of 2
    static char               buffer[4096];
    OS_Network_Socket_t       udp_socket;
    OS_NetworkSocket_Handle_t handle = 0;
    OS_Error_t                err;

    const OS_Dataport_t dp = config.dataport[handle];
    // creates a length guaranteed larger than that of the dataport, which won't
    // fit in the dataport and will generate an error case
    size_t len = OS_Dataport_getSize(dp) + 1;

    err = OS_NetworkSocket_read(handle, buffer, len, NULL);
    if (err != OS_ERROR_INVALID_PARAMETER)
    {
        Debug_LOG_ERROR(
            "Client socket read with invalid dataport size failed, error %d",
            err);
    }
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_PARAMETER, err);

    err = OS_NetworkSocket_recvfrom(handle, buffer, len, NULL, &udp_socket);
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

    err = OS_NetworkSocket_sendto(handle, buffer, len, NULL, udp_socket);
    if (err != OS_ERROR_INVALID_PARAMETER)
    {
        Debug_LOG_ERROR(
            "Client socket sendto with invalid dataport size failed, error "
            "%d",
            err);
    }
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_PARAMETER, err);

    TEST_FINISH();
}

void
test_dataport_size_check_lib_functions()
{
    TEST_START();

    OS_NetworkSocket_Handle_t handle;
    OS_Network_Socket_t       udp_socket = { .domain = OS_AF_INET,
                                       .type   = OS_SOCK_DGRAM,
                                       .name   = "10.0.0.10",
                                       .port   = 8888 };

    OS_Error_t err = OS_NetworkSocket_create(NULL, &udp_socket, &handle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("client_socket_create() failed, code %d", err);
    }
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    const OS_Dataport_t dp = config.dataport[handle];
    // creates a length guaranteed larger than that of the dataport, which won't
    // fit in the dataport and will generate an error case
    size_t len = OS_Dataport_getSize(dp) + 1;

    err = network_stack_rpc_socket_read(handle, &len);
    if (err != OS_ERROR_INVALID_PARAMETER)
    {
        Debug_LOG_ERROR(
            "Lib socket read with invalid dataport size failed, error %d",
            err);
    }
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_PARAMETER, err);

    err = network_stack_rpc_socket_recvfrom(handle, &len, &udp_socket);
    if (err != OS_ERROR_INVALID_PARAMETER)
    {
        Debug_LOG_ERROR(
            "Lib socket recvfrom with invalid dataport size failed, error %d",
            err);
    }
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_PARAMETER, err);

    err = network_stack_rpc_socket_write(handle, &len);
    if (err != OS_ERROR_INVALID_PARAMETER)
    {
        Debug_LOG_ERROR(
            "Lib socket write with invalid dataport size failed, error %d",
            err);
    }
    ASSERT_EQ_OS_ERR(OS_ERROR_INVALID_PARAMETER, err);

    err = network_stack_rpc_socket_sendto(handle, &len, udp_socket);
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
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    TEST_FINISH();
}

//------------------------------------------------------------------------------
void
pre_init(void)
{
#if defined(Debug_Config_PRINT_TO_LOG_SERVER)
    OS_Error_t err = SysLoggerClient_init(sysLogger_Rpc_log);
    Debug_ASSERT(err == OS_SUCCESS);
#endif
}

//------------------------------------------------------------------------------
int
run()
{
    init_client_api();
    Debug_LOG_INFO("Starting test_app_client %s...", get_instance_name());

    if (!strcmp(get_instance_name(), "nwApp1"))
    {
        // The following API tests do not need to be executed in parallel
        // therefore only nwApp1 will execute them.
        test_dataport_size_check_client_functions();
        test_dataport_size_check_lib_functions();
        test_socket_create_neg();
        test_socket_create_pos();
    }
    // synchronise the nwApp1 and nwApp1_2
    event_network_app_send_ready_emit();
    event_network_app_recv_ready_wait();

    test_tcp_client();
    test_udp_recvfrom();
    test_udp_sendto();

    return 0;
}
