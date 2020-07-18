/*
 *  OS Network Stack CAmkES wrapper
 *
 *  Copyright (C) 2019, Hensoldt Cyber GmbH
 *
 */

#include "system_config.h"

#include "LibDebug/Debug.h"
#include "OS_Error.h"
#include "OS_NetworkStack.h"
#include "util/helper_func.h"
#include <camkes.h>
#include "OS_Dataport.h"
#include "OS_Network.h"
#include "util/loop_defines.h"

#ifdef OS_NETWORK_STACK_USE_CONFIGSERVER
char DEV_ADDR[20];
char GATEWAY_ADDR[20];
char SUBNET_MASK[20];
#endif

static const OS_NetworkStack_AddressConfig_t config =
{
    .dev_addr      = DEV_ADDR,
    .gateway_addr  = GATEWAY_ADDR,
    .subnet_mask   = SUBNET_MASK
};

#ifdef OS_NETWORK_STACK_USE_CONFIGSERVER
OS_Error_t
read_ip_from_config_server(void)
{
    OS_Error_t ret;
    // Create a handle to the remote library instance.
    OS_ConfigServiceHandle_t serverLibWithMemBackend;

    static OS_ConfigService_ClientCtx_t ctx =
    {
        .dataport = OS_DATAPORT_ASSIGN(cfg_dataport_buf)
    };
    ret = OS_ConfigService_createHandleRemote(
                                        0,
                                        &ctx,
                                        &serverLibWithMemBackend);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigService_createHandleRemote failed with :%d", ret);
        return ret;
    }

    // Get the needed param values one by one from config server, using below API
    ret = helper_func_getConfigParameter(&serverLibWithMemBackend,
                                         DOMAIN_NWSTACK,
#ifdef OS_NWSTACK_AS_CLIENT
                                         CFG_ETH_ADDR_CLIENT,
#endif
#ifdef OS_NWSTACK_AS_SERVER
                                         CFG_ETH_ADDR_SERVER,
#endif
                                         DEV_ADDR,
                                         sizeof(DEV_ADDR));
    if (ret != OS_SUCCESS)
    {
#ifdef OS_NWSTACK_AS_CLIENT
        Debug_LOG_ERROR("helper_func_getConfigParameter for param %s failed with :%d",
                        CFG_ETH_ADDR_CLIENT, ret);
#endif
#ifdef OS_NWSTACK_AS_SERVER
        Debug_LOG_ERROR("helper_func_getConfigParameter for param %s failed with :%d",
                        CFG_ETH_ADDR_SERVER, ret);
#endif
        return ret;
    }
    Debug_LOG_INFO("[NwStack '%s'] IP ADDR: %s", get_instance_name(), DEV_ADDR);

    ret = helper_func_getConfigParameter(&serverLibWithMemBackend,
                                         DOMAIN_NWSTACK,
                                         CFG_ETH_GATEWAY_ADDR,
                                         GATEWAY_ADDR,
                                         sizeof(GATEWAY_ADDR));
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("helper_func_getConfigParameter for param %s failed with :%d",
                        CFG_ETH_GATEWAY_ADDR, ret);
        return ret;
    }
    Debug_LOG_INFO("[NwStack '%s'] GATEWAY ADDR: %s", get_instance_name(),
                   GATEWAY_ADDR);

    ret = helper_func_getConfigParameter(&serverLibWithMemBackend,
                                         DOMAIN_NWSTACK,
                                         CFG_ETH_SUBNET_MASK,
                                         SUBNET_MASK,
                                         sizeof(SUBNET_MASK));
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("helper_func_getConfigParameter for param %s failed with :%d",
                        CFG_ETH_SUBNET_MASK, ret);
        return ret;
    }
    Debug_LOG_INFO("[NwStack '%s'] SUBNETMASK: %s", get_instance_name(),
                   SUBNET_MASK);

    return OS_SUCCESS;
}
#endif


//------------------------------------------------------------------------------
int run(void)
{
    Debug_LOG_INFO("[NwStack '%s'] starting", get_instance_name());

    // can't make this "static const" or even "static" because the data ports
    // are allocated at runtime
    OS_NetworkStack_CamkesConfig_t camkes_config =
    {
        .notify_init_done        = nwStack_event_ready_emit,
        .wait_loop_event         = c_tick_or_data_wait,

        .internal =
        {
            .notify_loop        = e_tick_or_data_emit,

            .allocator_lock     = allocatorMutex_lock,
            .allocator_unlock   = allocatorMutex_unlock,

            .nwStack_lock       = nwstackMutex_lock,
            .nwStack_unlock     = nwstackMutex_unlock,

            .socketCB_lock      = socketControlBlockMutex_lock,
            .socketCB_unlock    = socketControlBlockMutex_unlock,

            .stackTS_lock       = stackThreadSafeMutex_lock,
            .stackTS_unlock     = stackThreadSafeMutex_unlock,
        },

        .drv_nic =
        {
            .from = OS_DATAPORT_ASSIGN(nic_port_from),

            .to = OS_DATAPORT_ASSIGN(nic_port_to),

            .rpc =
            {
                .dev_write      = nic_driver_tx_data,
                .get_mac        = nic_driver_get_mac,
            }
        },

        .app =
        {
            .notify_init_done   = nwStack_event_ready_emit,

        }
    };

    #define LOOP_ELEMENT          {            \
        .notify_write = GEN_EMIT(e_write),     \
        .wait_write = GEN_WAIT(c_write),       \
        .notify_read = GEN_EMIT(e_read),       \
        .wait_read = GEN_WAIT(c_read),         \
        .notify_connection = GEN_EMIT(e_conn), \
        .wait_connection = GEN_WAIT(c_conn),   \
        .buf = OS_DATAPORT_ASSIGN(GEN_ID(nwStack_port)), \
        .accepted_handle = -1, \
    },

    static OS_NetworkStack_SocketResources_t socks[OS_NETWORK_MAXIMUM_SOCKET_NO] = {
        #define LOOP_COUNT OS_NETWORK_MAXIMUM_SOCKET_NO
        #include "util/loop.h"
    };
    camkes_config.internal.number_of_sockets = OS_NETWORK_MAXIMUM_SOCKET_NO;
    camkes_config.internal.sockets = socks;

    OS_Error_t ret;

#ifdef OS_NETWORK_STACK_USE_CONFIGSERVER
    ret = read_ip_from_config_server();
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_FATAL("[NwStack '%s'] Read from config failed, error %d",
                        get_instance_name(), ret);
        return -1;
    }
#endif

    ret = OS_NetworkStack_run(&camkes_config, &config);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_FATAL("[NwStack '%s'] OS_NetworkStack_run() failed, error %d",
                        get_instance_name(), ret);
        return -1;
    }

    // actually, OS_NetworkStack_run() is not supposed to return with
    // OS_SUCCESS. We have to assume this is a graceful shutdown for some
    // reason
    Debug_LOG_WARNING("[NwStack '%s'] graceful termination",
                      get_instance_name());

    return 0;
}
