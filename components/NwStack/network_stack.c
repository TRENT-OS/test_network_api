/*
 *  SEOS Network Stack CAmkES wrapper
 *
 *  Copyright (C) 2019, Hensoldt Cyber GmbH
 *
 */

#include "system_config.h"

#include "LibDebug/Debug.h"
#include "SeosError.h"
#include "seos_api_network_stack.h"
#include "util/helper_func.h"
#include <camkes.h>

#ifdef OS_NETWORK_STACK_USE_CONFIGSERVER
char DEV_ADDR[20];
char GATEWAY_ADDR[20];
char SUBNET_MASK[20];
#endif

static const seos_network_stack_config_t config =
{
    .dev_addr      = DEV_ADDR,
    .gateway_addr  = GATEWAY_ADDR,
    .subnet_mask   = SUBNET_MASK
};

#ifdef OS_NETWORK_STACK_USE_CONFIGSERVER
seos_err_t
read_ip_from_config_server(void)
{
    seos_err_t ret;
    // Create a handle to the remote library instance.
    OS_ConfigServiceHandle_t serverLibWithMemBackend;

    ret = OS_ConfigService_createHandle(OS_CONFIG_HANDLE_KIND_RPC,
                                        0,
                                        &serverLibWithMemBackend);
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigService_createHandle failed with :%d", ret);
        return ret;
    }

    // Get the needed param values one by one from config server, using below API
    ret = helper_func_getConfigParameter(&serverLibWithMemBackend,
                                         DOMAIN_NWSTACK,
#ifdef SEOS_NWSTACK_AS_CLIENT
                                         CFG_ETH_ADDR_CLIENT,
#endif
#ifdef SEOS_NWSTACK_AS_SERVER
                                         CFG_ETH_ADDR_SERVER,
#endif
                                         DEV_ADDR,
                                         sizeof(DEV_ADDR));
    if (ret != SEOS_SUCCESS)
    {
#ifdef SEOS_NWSTACK_AS_CLIENT
        Debug_LOG_ERROR("helper_func_getConfigParameter for param %s failed with :%d",
                        CFG_ETH_ADDR_CLIENT, ret);
#endif
#ifdef SEOS_NWSTACK_AS_SERVER
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
    if (ret != SEOS_SUCCESS)
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
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("helper_func_getConfigParameter for param %s failed with :%d",
                        CFG_ETH_SUBNET_MASK, ret);
        return ret;
    }
    Debug_LOG_INFO("[NwStack '%s'] SUBNETMASK: %s", get_instance_name(),
                   SUBNET_MASK);

    return SEOS_SUCCESS;
}
#endif
//------------------------------------------------------------------------------
int run(void)
{
    Debug_LOG_INFO("[NwStack '%s'] starting", get_instance_name());

    // can't make this "static const" or even "static" because the data ports
    // are allocated at runtime
    seos_camkes_network_stack_config_t camkes_config =
    {
        .notify_init_done        = nwStack_event_ready_emit,
        .wait_loop_event         = c_tick_or_data_wait,

        .internal =
        {
            .notify_loop        = e_tick_or_data_emit,

            .notify_write       = e_write_emit,
            .wait_write         = c_write_wait,

            .notify_read        = e_read_emit,
            .wait_read          = c_read_wait,

            .notify_connection  = e_conn_emit,
            .wait_connection    = c_conn_wait,
        },

        .drv_nic =
        {
            .from = // NIC -> stack
            {
                .buffer         = nic_port_from,
                .len            = PAGE_SIZE
            },

            .to = // stack -> NIC
            {
                .buffer         = nic_port_to,
                .len            = PAGE_SIZE
            },

            .rpc =
            {
                .dev_write      = nic_driver_tx_data,
                .get_mac        = nic_driver_get_mac,
            }
        },

        .app =
        {
            .notify_init_done   = nwStack_event_ready_emit,

            .port =
            {
                .buffer         = nwStack_port,
                .len            = PAGE_SIZE
            },
        }
    };

    seos_err_t ret;
#ifdef OS_NETWORK_STACK_USE_CONFIGSERVER
    ret = read_ip_from_config_server();
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_FATAL("[NwStack '%s'] Read from config failed, error %d",
                        get_instance_name(), ret);
        return -1;
    }
#endif

    ret = seos_network_stack_run(&camkes_config, &config);
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_FATAL("[NwStack '%s'] seos_network_stack_run() failed, error %d",
                        get_instance_name(), ret);
        return -1;
    }

    // actually, seos_network_stack_run() is not supposed to return with
    // SEOS_SUCCESS. We have to assume this is a graceful shutdown for some
    // reason
    Debug_LOG_WARNING("[NwStack '%s'] graceful termination",
                      get_instance_name());

    return 0;
}
