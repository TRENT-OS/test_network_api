/*
 *  OS Network Stack CAmkES wrapper
 *
 * Copyright (C) 2021, HENSOLDT Cyber GmbH
 *
 */

#include "system_config.h"

#include "OS_Dataport.h"
#include "OS_Error.h"
#include "OS_Network.h"
#include "OS_NetworkStack.h"
#include "TimeServer.h"
#include "lib_compiler/compiler.h"
#include "lib_debug/Debug.h"
#include "util/loop_defines.h"
#include <camkes.h>

#define NUMBER_OF_CLIENTS 8

static const OS_NetworkStack_AddressConfig_t config =
{
    .dev_addr     = DEV_ADDR,
    .gateway_addr = GATEWAY_ADDR,
    .subnet_mask  = SUBNET_MASK
};

static const if_OS_Timer_t timer =
    IF_OS_TIMER_ASSIGN(timeServer_rpc, timeServer_notify);

static bool initSuccessfullyCompleted = false;

unsigned int
networkStack_rpc_num_badges(void);

seL4_Word
networkStack_rpc_enumerate_badge(unsigned int i);

unsigned int
networkStack_rpc_socket_quota(seL4_Word client_id);

seL4_Word
networkStack_rpc_get_sender_id(void);

void*
networkStack_rpc_buf(seL4_Word client_id);

size_t
networkStack_rpc_buf_size(seL4_Word client_id);

int
get_client_id(void)
{
    return networkStack_rpc_get_sender_id() - networkStack_rpc_enumerate_badge(0);
}

uint8_t*
get_client_id_buf(void)
{
    return networkStack_rpc_buf(networkStack_rpc_get_sender_id());
}

int
get_client_id_buf_size(void)
{
    return networkStack_rpc_buf_size(networkStack_rpc_get_sender_id());
}

int
get_client_socket_quota(int clientId)
{
    return networkStack_rpc_socket_quota(networkStack_rpc_enumerate_badge(clientId));
}

//------------------------------------------------------------------------------
// network stack's PicTCP OS adaption layer calls this.
uint64_t
Timer_getTimeMs(void)
{
    OS_Error_t err;
    uint64_t   ms;

    if ((err = TimeServer_getTime(&timer, TimeServer_PRECISION_MSEC, &ms)) !=
        OS_SUCCESS)
    {
        Debug_LOG_ERROR("TimeServer_getTime() failed with %d", err);
        ms = 0;
    }

    return ms;
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
void
post_init(void)
{
    Debug_LOG_INFO("[NwStack '%s'] starting", get_instance_name());

    // attribute int maximum_number_of_client        = 8;
    // attribute int maximum_number_of_total_sockets = 0;

    if (maximum_number_of_client < networkStack_rpc_num_badges())
    {
        Debug_LOG_FATAL(
            "[NwStack '%s'] is configured for %d clients, but %d clients are "
            "connected",
            get_instance_name(),
            maximum_number_of_client,
            networkStack_rpc_num_badges());
        return;
    }

    int* max_clients = calloc(sizeof(int), networkStack_rpc_num_badges());
    if (max_clients == NULL)
    {
        Debug_LOG_FATAL("[NwStack '%s'] Could not allocate resources.");
        return;
    }

    int totalSocketsNeeded = 0;

    for (int i = 0; i < networkStack_rpc_num_badges(); i++)
    {
        max_clients[i] = get_client_socket_quota(i);
        Debug_LOG_INFO(
            "[NwStack '%s'] Client %d requires %d sockets",
            get_instance_name(),
            i,
            max_clients[i]);
        totalSocketsNeeded += max_clients[i]; // sockets needed for client i
    }

    Debug_LOG_INFO(
        "[NwStack '%s'] %d sockets required in total",
        get_instance_name(),
        totalSocketsNeeded);

    // maximum_number_of_total_sockets = 0 (default) means that the need
    // is set by the connected clients.
    if (maximum_number_of_total_sockets)
        if (totalSocketsNeeded > maximum_number_of_total_sockets)
        {
            Debug_LOG_FATAL(
                "[NwStack '%s'] The connected clients require %d, but only %d "
                "are "
                "configured .",
                totalSocketsNeeded,
                maximum_number_of_total_sockets);
            return;
        }

// will be removed with the event rework
#define LOOP_ELEMENT                                                           \
    {                                                                          \
        .notify_write      = GEN_EMIT(e_write_),                               \
        .wait_write        = GEN_WAIT(c_write_),                               \
        .notify_read       = GEN_EMIT(e_read_),                                \
        .wait_read         = GEN_WAIT(c_read_),                                \
        .notify_connection = GEN_EMIT(e_conn_),                                \
        .wait_connection   = GEN_WAIT(c_conn_),                                \
        .accepted_handle   = -1,                                               \
    },

    // will be removed with the event rework
    static OS_NetworkStack_SocketResources_t socks[OS_NETWORK_MAXIMUM_SOCKET_NO] = {
#define LOOP_COUNT OS_NETWORK_MAXIMUM_SOCKET_NO
#include "util/loop.h" // places LOOP_ELEMENT here for LOOP_COUNT times
    };

    const OS_NetworkStack_CamkesConfig_t camkes_config =
    {
        .wait_loop_event         = event_tick_or_data_wait,

        .internal =
        {
            .notify_loop        = event_internal_emit,

            .allocator_lock     = allocatorMutex_lock,
            .allocator_unlock   = allocatorMutex_unlock,

            .nwStack_lock       = nwstackMutex_lock,
            .nwStack_unlock     = nwstackMutex_unlock,

            .socketCB_lock      = socketControlBlockMutex_lock,
            .socketCB_unlock    = socketControlBlockMutex_unlock,

            .stackTS_lock       = stackThreadSafeMutex_lock,
            .stackTS_unlock     = stackThreadSafeMutex_unlock,

            .number_of_clients      = networkStack_rpc_num_badges(),
            .number_of_sockets      = totalSocketsNeeded,
            .client_sockets_quota   = max_clients,
            .sockets                = socks,
        },

        .drv_nic =
        {
            .from = OS_DATAPORT_ASSIGN_SIZE(nic_from_port, NIC_DRIVER_RINGBUFFER_NUMBER_ELEMENTS),
            .to   = OS_DATAPORT_ASSIGN(nic_to_port),

            .rpc =
            {
                .dev_read       = nic_rpc_rx_data,
                .dev_write      = nic_rpc_tx_data,
                .get_mac        = nic_rpc_get_mac_address,
            }
        }
    };

    OS_NetworkStack_CamkesConfig_t* p_camkes_config =
        malloc(sizeof(OS_NetworkStack_CamkesConfig_t));
    if (p_camkes_config == NULL)
    {
        Debug_LOG_ERROR("[NwStack '%s'] Could not allocate resources.");
        return;
    }

    *p_camkes_config = camkes_config;

    Debug_LOG_INFO("Clients connected %d", networkStack_rpc_num_badges());

    for (int i = 0; i < networkStack_rpc_num_badges(); i++)
        Debug_LOG_INFO("Client badge #%d", networkStack_rpc_enumerate_badge(i));

    OS_Error_t ret;

    Debug_LOG_INFO("[NwStack '%s'] IP ADDR: %s", get_instance_name(), DEV_ADDR);
    Debug_LOG_INFO(
        "[NwStack '%s'] GATEWAY ADDR: %s",
        get_instance_name(),
        GATEWAY_ADDR);
    Debug_LOG_INFO(
        "[NwStack '%s'] SUBNETMASK: %s",
        get_instance_name(),
        SUBNET_MASK);

    ret = OS_NetworkStack_init(p_camkes_config, &config);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_FATAL(
            "[NwStack '%s'] OS_NetworkStack_init() failed, error %d",
            get_instance_name(),
            ret);
        return;
    }
    initSuccessfullyCompleted = true;
}

//------------------------------------------------------------------------------
int
run(void)
{
    if (!initSuccessfullyCompleted)
    {
        Debug_LOG_FATAL(
            "[NwStack '%s'] initialization failed",
            get_instance_name());
        return -1;
    }
    void networkStack_rpc_emit(seL4_Word client_id);

    void networkStack_rpc_notify_socket(
        seL4_Word    client_id,
        unsigned int socket);

    networkStack_rpc_emit(networkStack_rpc_enumerate_badge(0));

    // The Ticker component sends us a tick every second. Currently there is
    // no dedicated interface to enable and disable the tick. because we
    // don't need this. OS_NetworkStack_run() is not supposed to return.

    OS_Error_t ret = OS_NetworkStack_run();
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_FATAL(
            "[NwStack '%s'] OS_NetworkStack_run() failed, error %d",
            get_instance_name(),
            ret);
        return -1;
    }

    // actually, OS_NetworkStack_run() is not supposed to return with
    // OS_SUCCESS. We have to assume this is a graceful shutdown for some
    // reason
    Debug_LOG_WARNING(
        "[NwStack '%s'] graceful termination",
        get_instance_name());

    return 0;
}
