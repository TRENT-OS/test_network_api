/*
 * TestAppTCPClient
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
#include "system_config.h"
#include <string.h>

#include "OS_Network.h"
#include "math.h"

#include "SysLoggerClient.h"
#include "interfaces/if_OS_Socket.h"
#include "util/loop_defines.h"
#include "util/non_blocking_helper.h"

#include "TimeServer.h"

#include <camkes.h>

//------------------------------------------------------------------------------
//                            Test params
//------------------------------------------------------------------------------

#define KILOBYTE 1024*5

#define RUNS 10

//------------------------------------------------------------------------------

static const if_OS_Socket_t network_stack =
    IF_OS_SOCKET_ASSIGN(networkStack);

//------------------------------------------------------------------------------
typedef struct
{
    event_notify_func_t notify_about_new_events;
    event_wait_func_t   wait_on_new_events;
    mutex_lock_func_t   shared_resource_lock;
    mutex_unlock_func_t shared_resource_unlock;
} nh_helper_sync_func;

static OS_NetworkSocket_Evt_t eventCollection[OS_NETWORK_MAXIMUM_SOCKET_NO] = {0};

static nh_helper_sync_func sync_func;

static const if_OS_Timer_t timer = IF_OS_TIMER_ASSIGN(timeServer_rpc, timeServer_notify);

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

void
pre_init(void)
{
    OS_Error_t err;
#if defined(Debug_Config_PRINT_TO_LOG_SERVER)
    err = SysLoggerClient_init(sysLogger_Rpc_log);
    Debug_ASSERT(err == OS_SUCCESS);
#endif
    // Initialize the helper lib with the required synchronization mechanisms.
    nb_helper_init(
        event_received_send_ready_emit,
        event_received_recv_ready_wait,
        SharedResourceMutex_lock,
        SharedResourceMutex_unlock);

    nb_helper_init_local(
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

    err = nb_helper_wait_for_network_stack_init(&network_stack);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("nb_helper_wait_for_network_stack_init() failed, code %d", err);
    }
    ASSERT_EQ_OS_ERR(OS_SUCCESS, err);
}
/*
//------------------------------------------------------------------------------
OS_Error_t
nb_helper_wait_for_conn_est_ev_on_socket_debug(
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

        if ((eventMask & OS_SOCK_EV_CONN_EST || eventMask & OS_SOCK_EV_ERROR
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

    if (eventMask & OS_SOCK_EV_CONN_EST)
    {
        eventCollection[handle.handleID].eventMask &= ~OS_SOCK_EV_CONN_EST;
        return OS_SUCCESS;
    }
    else
    {
        eventCollection[handle.handleID].eventMask &= ~OS_SOCK_EV_ERROR;
        return err;
    }
}
*/
OS_Error_t
check_for_socket_event(
    const OS_NetworkSocket_Handle_t handle)
{
    CHECK_VALUE_IN_RANGE(handle.handleID, 0, OS_NETWORK_MAXIMUM_SOCKET_NO);

    OS_Error_t err = OS_SUCCESS;
    uint8_t eventMask = 0;
    //bool foundRelevantEvent = false;

    Debug_ASSERT(NULL != sync_func.shared_resource_lock);
    sync_func.shared_resource_lock();
    eventMask = eventCollection[handle.handleID].eventMask;
    err = eventCollection[handle.handleID].currentError;
    Debug_ASSERT(NULL != sync_func.shared_resource_unlock);
    sync_func.shared_resource_unlock();
    if (eventMask & OS_SOCK_EV_CONN_EST)
    {
        //foundRelevantEvent = true;
        Debug_LOG_DEBUG("OS_SOCK_EV_CONN_EST");
    } else if (eventMask & OS_SOCK_EV_ERROR) {
        Debug_LOG_DEBUG("OS_SOCK_EV_ERROR");
    } else if (eventMask & OS_SOCK_EV_FIN) {
        Debug_LOG_DEBUG("OS_SOCK_EV_FIN");
    }
    return err;
}

void
test_tcp_client_send_only()
{
    TEST_START();
    int m = 0;
    const OS_NetworkSocket_Addr_t dstAddr =
        {
            .addr = GATEWAY_ADDR,
            .port = 5555
        };

    for (m = 0; m < RUNS; m++) {
        printf("\nRUN: %i of %i", (m+1), RUNS);
        uint64_t timestamp_setup = 0;
        TimeServer_getTime(&timer, 1, &timestamp_setup); // 0 -> Time in seconds
        //Debug_LOG_DEBUG("Reset timestamp to: %u", timestamp_setup);


        

        OS_NetworkSocket_Handle_t handle[OS_NETWORK_MAXIMUM_SOCKET_NO];
        OS_Error_t err;
        int i=0;

        //No loop -> one socket is enough
        err = OS_NetworkSocket_create(
                  &network_stack,
                  &handle[i],
                  OS_AF_INET,
                  OS_SOCK_STREAM);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR(
                "OS_NetworkSocket_create() failed, code %d for %d socket",
                err,
                i);
                return;
        }

        err = OS_NetworkSocket_connect(handle[i], &dstAddr);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR(
                "OS_NetworkSocket_connect() failed, code %d for %d socket",
                err,
                i);
                return;
        }
        err = nb_helper_wait_for_conn_est_ev_on_socket(handle[i]);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR(
                "nb_helper_wait_for_conn_est_ev_on_socket() failed, code %d for %d socket",
                err,
                i);
                return;
        }

        //Debug_LOG_INFO("Send request to host...");


        char data[1024];

        size_t datalen = sizeof(data);
        for (int i=0; i<datalen;i++) {data[i] = 'a';} 
        size_t kilobytes = datalen * KILOBYTE;

        //Debug_LOG_INFO("\nWriting data to socket: Data: %i BYTE... \n", kilobytes);

        uint64_t timestamp = 0;
        TimeServer_getTime(&timer, 1, &timestamp); // 0 -> Time in seconds
        //Debug_LOG_DEBUG("Reset timestamp to: %u", timestamp);

        size_t bytes_written = 0;
        do
        {
            size_t datalen = sizeof(data);

            err = OS_NetworkSocket_write(
                      handle[i],
                      &data[0],
                      datalen,
                      &datalen);
            if (err != OS_SUCCESS)
            {
                Debug_LOG_ERROR("OS_NetworkSocket_write() failed, code %d", err);
                OS_NetworkSocket_close(handle[i]);
                nb_helper_reset_ev_struct_for_socket(handle[i]);
                return;
            }

            check_for_socket_event(handle[i]);

            bytes_written += datalen;

            //Debug_LOG_DEBUG("Sent %i BYTES of %i BYTES\n", bytes_written, kilobytes);
        }
        while (bytes_written < kilobytes);

        /* Close the socket communication */
        err = OS_NetworkSocket_close(handle[i]);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR(
                "OS_NetworkSocket_close() failed for handle %d, code %d", i,
                err);
            return;
        }
        nb_helper_reset_ev_struct_for_socket(handle[i]);
        uint64_t timestamp_after = 0;
        TimeServer_getTime(&timer, 1, &timestamp_after);
        //Debug_LOG_DEBUG("Received SOCK_EV_CLOSE -> closing socket \n Received a total of: %i BYTES", total_bytes);
        uint64_t delta = timestamp_after - timestamp_setup;
        uint64_t kbps = (bytes_written/1024)/(delta/1000);
        //Debug_LOG_DEBUG("duration from setup: %"PRIu64"ms speed: %"PRIu64" kb/s", delta, kbps);
        printf("\ndata: %iMB\n", ((bytes_written/1024)/1024));
        printf("setup: %"PRIu64"ms speed: %"PRIu64" kb/s\n", delta, kbps);
        delta = timestamp_after - timestamp;
        kbps = (bytes_written/1024)/(delta/1000);
        //Debug_LOG_DEBUG("duration: %"PRIu64"ms speed: %"PRIu64" kb/s", delta, kbps);
        printf("sending: %"PRIu64"ms speed: %"PRIu64" kb/s\n", delta, kbps);
    }
    TEST_FINISH();
    Debug_LOG_DEBUG("Tests finished!");
}
    

//------------------------------------------------------------------------------
int
run()
{
    Debug_LOG_INFO("Starting TestAppTCPClient %s...", get_instance_name());

    test_tcp_client_send_only();

    return 0;
}
