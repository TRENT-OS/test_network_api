/*
 * TestAppTCPServer
 *
 * Copyright (C) 2019-2021, HENSOLDT Cyber GmbH
 *
 */
#include "OS_Error.h"
#include "lib_compiler/compiler.h"
#include "lib_debug/Debug.h"
#include "lib_macros/Test.h"
#include "lib_macros/Check.h"
#include "stdint.h"
#include <string.h>

#include "OS_Network.h"
#include "interfaces/if_OS_Socket.h"
#include "util/loop_defines.h"

#include "system_config.h"

#include "TimeServer.h"



#include <camkes.h>



typedef struct
{
    event_notify_func_t notify_about_new_events;
    event_wait_func_t   wait_on_new_events;
    mutex_lock_func_t   shared_resource_lock;
    mutex_unlock_func_t shared_resource_unlock;
} nh_helper_sync_func;

static const if_OS_Socket_t network_stack = IF_OS_SOCKET_ASSIGN(networkStack);

static OS_NetworkSocket_Evt_t eventCollection[OS_NETWORK_MAXIMUM_SOCKET_NO] = {0};

static nh_helper_sync_func sync_func;

static const if_OS_Timer_t timer = IF_OS_TIMER_ASSIGN(timeServer_rpc, timeServer_notify);

//------------------------------------------------------------------------------

OS_Error_t
nb_helper_init_local(
    void (*event_notify_func_t)(void),
    void (*event_wait_func_t)(void),
    int (*mutex_lock_func_t)(void),
    int (*mutex_unlock_func_t)(void))
{
    CHECK_PTR_NOT_NULL(event_notify_func_t);
    CHECK_PTR_NOT_NULL(event_wait_func_t);
    CHECK_PTR_NOT_NULL(mutex_lock_func_t);
    CHECK_PTR_NOT_NULL(mutex_unlock_func_t);

    sync_func.notify_about_new_events = event_notify_func_t;
    sync_func.wait_on_new_events = event_wait_func_t;
    sync_func.shared_resource_lock = mutex_lock_func_t;
    sync_func.shared_resource_unlock = mutex_unlock_func_t;

    return OS_SUCCESS;
}


//------------------------------------------------------------------------------

void
nb_helper_collect_pending_ev_handler_local(
    void* ctx)
{
    Debug_ASSERT(NULL != ctx);

    OS_NetworkSocket_Evt_t eventBuffer[OS_NETWORK_MAXIMUM_SOCKET_NO] = {0};
    int numberOfSocketsWithEvents = 0;

    size_t bufferSize = sizeof(eventBuffer);

    OS_Error_t err = OS_NetworkSocket_getPendingEvents(
                         ctx,
                         eventBuffer,
                         bufferSize,
                         &numberOfSocketsWithEvents);
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);

    // Verify that the received number of sockets with events is within expected
    // bounds.
    ASSERT_LE_INT(numberOfSocketsWithEvents, OS_NETWORK_MAXIMUM_SOCKET_NO);
    ASSERT_GT_INT(numberOfSocketsWithEvents, -1);

    for (int i = 0; i < numberOfSocketsWithEvents; i++)
    {
        OS_NetworkSocket_Evt_t event;
        memcpy(&event, &eventBuffer[i], sizeof(event));

        if (event.socketHandle >= 0
            && event.socketHandle < OS_NETWORK_MAXIMUM_SOCKET_NO)
        {
            Debug_ASSERT(NULL != sync_func.shared_resource_lock);
            sync_func.shared_resource_lock();

            eventCollection[event.socketHandle].socketHandle = event.socketHandle;
            eventCollection[event.socketHandle].parentSocketHandle =
                event.parentSocketHandle;
            eventCollection[event.socketHandle].eventMask |= event.eventMask;
            eventCollection[event.socketHandle].currentError = event.currentError;

            Debug_ASSERT(NULL != sync_func.shared_resource_unlock);
            sync_func.shared_resource_unlock();
        }
        else
        {
            Debug_LOG_ERROR("Found invalid socket handle %d for event %d",
                            event.socketHandle, i);
        }
    }

    // Unblock any caller of the functions below waiting for new events to
    // arrive.
    if (numberOfSocketsWithEvents)
    {
        Debug_ASSERT(NULL != sync_func.notify_about_new_events);
        sync_func.notify_about_new_events();
    }

    // Re-register the callback function.
    int ret = networkStack_event_notify_reg_callback(
                  &nb_helper_collect_pending_ev_handler_local,
                  ctx);
    if (ret < 0)
    {
        Debug_LOG_ERROR(
            "networkStack_event_notify_reg_callback() failed, code %d", err);
    }
}

//------------------------------------------------------------------------------

OS_Error_t
nb_helper_wait_for_network_stack_init_local(
    const if_OS_Socket_t* const ctx)
{
    OS_NetworkStack_State_t networkStackState;

    do
    {
        networkStackState = OS_NetworkSocket_getStatus(ctx);
        if (networkStackState == UNINITIALIZED || networkStackState == INITIALIZED)
        {
            // just yield to wait until the stack is up and running
            seL4_Yield();
        }
        if (networkStackState == FATAL_ERROR)
        {
            // NetworkStack will not come up.
            Debug_LOG_ERROR("A FATAL_ERROR occurred in the Network Stack component.")
            return OS_ERROR_ABORTED;
        }
    }
    while (networkStackState != RUNNING);

    return OS_SUCCESS;
}

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
    OS_Error_t err;
#if defined(Debug_Config_PRINT_TO_LOG_SERVER)
    err = SysLoggerClient_init(sysLogger_Rpc_log);
    Debug_ASSERT(err == OS_SUCCESS);
#endif
    // Initialize the helper lib with the required synchronization mechanisms.
    nb_helper_init_local(
        event_received_send_ready_emit,
        event_received_recv_ready_wait,
        SharedResourceMutex_lock,
        SharedResourceMutex_unlock);

    // Set up callback for new received socket events.
    int ret = networkStack_event_notify_reg_callback(
                  &nb_helper_collect_pending_ev_handler_local,
                  (void*) &network_stack);
    if (ret < 0)
    {
        Debug_LOG_ERROR(
            "networkStack_event_notify_reg_callback() failed, code %d", err);
    }

    err = nb_helper_wait_for_network_stack_init_local(&network_stack);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("nb_helper_wait_for_network_stack_init() failed, code %d", err);
    }
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);
}

//------------------------------------------------------------------------------

OS_Error_t
nb_helper_reset_ev_struct_for_socket_local(
    const OS_NetworkSocket_Handle_t handle)
{
    CHECK_VALUE_IN_RANGE(handle.handleID, 0, OS_NETWORK_MAXIMUM_SOCKET_NO);

    Debug_ASSERT(NULL != sync_func.shared_resource_lock);
    sync_func.shared_resource_lock();

    memset(&eventCollection[handle.handleID], 0, sizeof(OS_NetworkSocket_Evt_t));

    Debug_ASSERT(NULL != sync_func.shared_resource_unlock);
    sync_func.shared_resource_unlock();

    return OS_SUCCESS;
}

//------------------------------------------------------------------------------

OS_Error_t
nb_helper_wait_for_conn_acpt_ev_on_socket_local(
    const OS_NetworkSocket_Handle_t handle)
{
    CHECK_VALUE_IN_RANGE(handle.handleID, 0, OS_NETWORK_MAXIMUM_SOCKET_NO);

    OS_Error_t err;
    uint8_t eventMask = 0;
    bool foundRelevantEvent = false;

    do
    {
        Debug_ASSERT(NULL != sync_func.shared_resource_lock);
        sync_func.shared_resource_lock();

        eventMask = eventCollection[handle.handleID].eventMask;
        err = eventCollection[handle.handleID].currentError;

        Debug_ASSERT(NULL != sync_func.shared_resource_unlock);
        sync_func.shared_resource_unlock();

        if ((eventMask & OS_SOCK_EV_CONN_ACPT || eventMask & OS_SOCK_EV_ERROR
             || eventMask & OS_SOCK_EV_FIN))
        {
            foundRelevantEvent = true;
        }
        else
        {
            // Wait for the arrival of new events.
            Debug_ASSERT(NULL != sync_func.wait_on_new_events);
            sync_func.wait_on_new_events();
        }
    }
    while (!foundRelevantEvent);

    if (eventMask & OS_SOCK_EV_CONN_ACPT)
    {
        eventCollection[handle.handleID].eventMask &= ~OS_SOCK_EV_CONN_ACPT;
        return OS_SUCCESS;
    }
    else
    {
        eventCollection[handle.handleID].eventMask &= ~OS_SOCK_EV_ERROR;
        return err;
    }
}

//------------------------------------------------------------------------------

int
run()
{
    Debug_LOG_INFO("Starting TestAppTCPServer ...");

    OS_NetworkSocket_Handle_t srvHandle;

    OS_Error_t err = OS_NetworkSocket_create(
                         &network_stack,
                         &srvHandle,
                         OS_AF_INET,
                         OS_SOCK_STREAM);
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
        nb_helper_reset_ev_struct_for_socket_local(srvHandle); //shouldnt inflict latency
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
        nb_helper_reset_ev_struct_for_socket_local(srvHandle); //shouldnt inflict latency
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
            nb_helper_wait_for_conn_acpt_ev_on_socket_local(srvHandle);

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
            nb_helper_reset_ev_struct_for_socket_local(srvHandle);
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
        // Keep calling read until we receive CONNECTION_CLOSED from the stack
         
        uint64_t timestamp = 0;
        TimeServer_getTime(&timer, 0, &timestamp); // 0 -> Time in seconds
        Debug_LOG_DEBUG("Reset timestamp to: %u", timestamp);     
        
        size_t total_bytes = 0;
        for (;;) //read loop for socket
        {
            CHECK_VALUE_IN_RANGE(clientHandle.handleID, 0, OS_NETWORK_MAXIMUM_SOCKET_NO);

            size_t n = 0;
            uint8_t eventMask = 0;
            bool foundRelevantEvent = false;
            

            do {
                Debug_ASSERT(NULL != sync_func.shared_resource_lock); //-> Helper init
                sync_func.shared_resource_lock();

                eventMask = eventCollection[clientHandle.handleID].eventMask;
                err = eventCollection[clientHandle.handleID].currentError;

                Debug_ASSERT(NULL != sync_func.shared_resource_unlock);
                sync_func.shared_resource_unlock();

                if ((eventMask & OS_SOCK_EV_READ || eventMask & OS_SOCK_EV_CLOSE
                     || eventMask & OS_SOCK_EV_ERROR || eventMask & OS_SOCK_EV_FIN))
                {
                    foundRelevantEvent = true;
                }
                else
                {
                    // wait for arrival of new event -> this is likely where the high waiting times get introduced 
                    Debug_ASSERT(NULL != sync_func.wait_on_new_events);
                    sync_func.wait_on_new_events();
                }
            } while (!foundRelevantEvent);

            if (eventMask & OS_SOCK_EV_FIN)
            {
                Debug_LOG_INFO("connection closed by server");
                OS_NetworkSocket_close(clientHandle);
                nb_helper_reset_ev_struct_for_socket_local(clientHandle);
                break;
            }
            else if (eventMask & OS_SOCK_EV_READ)
            {
                eventCollection[clientHandle.handleID].eventMask &= ~OS_SOCK_EV_READ;
                do {
                    err = OS_NetworkSocket_read(
                            clientHandle,
                            buffer,
                            sizeof(buffer),
                            &n);
                } while (err == OS_ERROR_TRY_AGAIN);
                Debug_LOG_DEBUG("Received %i bytes", n);
                total_bytes += n;
            }
            else if (eventMask & OS_SOCK_EV_CLOSE)
            {
                eventCollection[clientHandle.handleID].eventMask = 0;
                OS_NetworkSocket_close(clientHandle);
                nb_helper_reset_ev_struct_for_socket_local(clientHandle);
                uint64_t timestamp_after = 0;
                TimeServer_getTime(&timer, 0, &timestamp_after);
                Debug_LOG_DEBUG("Received SOCK_EV_CLOSE -> closing socket \n Received a total of: %i BYTES", total_bytes);
                // xgetTime does not work as expected -> results seem to be somehow random in nature
                //Debug_LOG_ERROR("timestamp:%u, timestamp_after: %u", timestamp, timestamp_after);
                break;
            }
            else
            {
                eventCollection[clientHandle.handleID].eventMask &= ~OS_SOCK_EV_ERROR;
                Debug_LOG_ERROR("Received not handled ERROR: %d", err);
            }           
        } 
    }
    return -1;
}