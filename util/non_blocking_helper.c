/*
 * Implementation of the helper functions for the non-blocking scheme.
 *
 * Copyright (C) 2021, HENSOLDT Cyber GmbH
 */

#include <stdint.h>
#include <string.h>
#include <camkes.h>

#include "OS_Error.h"
#include "OS_Network.h"
#include "OS_Types.h"

#include "lib_debug/Debug.h"
#include "lib_macros/Check.h"
#include "lib_macros/Test.h"
#include "interfaces/if_OS_Socket.h"

#include "non_blocking_helper.h"
#include "system_config.h"

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

//------------------------------------------------------------------------------
OS_Error_t
nb_helper_init(
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
nb_helper_collect_pending_ev_handler(
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
                  &nb_helper_collect_pending_ev_handler,
                  ctx);
    if (ret < 0)
    {
        Debug_LOG_ERROR(
            "networkStack_event_notify_reg_callback() failed, code %d", err);
    }
}

//------------------------------------------------------------------------------
OS_Error_t
nb_helper_wait_for_network_stack_init(
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

//------------------------------------------------------------------------------
OS_Error_t
nb_helper_wait_for_read_ev_on_socket(
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

        if ((eventMask & OS_SOCK_EV_READ || eventMask & OS_SOCK_EV_CLOSE
             || eventMask & OS_SOCK_EV_ERROR || eventMask & OS_SOCK_EV_FIN))
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

    if (eventMask & OS_SOCK_EV_READ)
    {
        eventCollection[handle.handleID].eventMask &= ~OS_SOCK_EV_READ;
        return OS_SUCCESS;
    }
    else if (eventMask & OS_SOCK_EV_CLOSE)
    {
        eventCollection[handle.handleID].eventMask = 0;
        return OS_ERROR_NETWORK_CONN_SHUTDOWN;
    }
    else
    {
        eventCollection[handle.handleID].eventMask &= ~OS_SOCK_EV_ERROR;
        return err;
    }
}

//------------------------------------------------------------------------------
OS_Error_t
nb_helper_wait_for_conn_est_ev_on_socket(
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

//------------------------------------------------------------------------------
OS_Error_t
nb_helper_wait_for_conn_acpt_ev_on_socket(
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
OS_Error_t
nb_helper_reset_ev_struct_for_socket(
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
