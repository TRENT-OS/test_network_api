/*
 *  SEOS Network Stack CAmkES wrapper
 *
 *  Copyright (C) 2019, Hensoldt Cyber GmbH
 *
 */

#include "seos_system_config.h"

#include "LibDebug/Debug.h"
#include "SeosError.h"
#include "seos_api_network_stack.h"
#include <camkes.h>


static const seos_network_stack_config_t config =
{
    .dev_addr      = ETH_1_ADDR,
    .gateway_addr  = ETH_1_GATEWAY_ADDR,
    .subnet_mask   = ETH_1_SUBNET_MASK
};


//------------------------------------------------------------------------------
int run()
{
    Debug_LOG_INFO("starting network stack #1 (client)\n");

    // can't make this "static const" or even "static" because the data ports
    // are allocated at runtime
    seos_camkes_network_stack_config_t camkes_config =
    {
        .notify_init_done        = event_network_init_done_emit,
        .wait_loop_event         = event_tick_or_data_wait,

        .internal =
        {
            .notify_loop        = event_internal_emit,

            .notify_write       = e_write_emit,
            .wait_write         = c_write_wait,

            .notify_read        = e_read_emit,
            .wait_read          = c_read_wait,

            .notify_connection  = NULL,
            .wait_connection    = NULL,
        },

        .drv_nic =
        {
            .wait_init_done     = event_nic_init_done_wait,

            .from = // NIC -> stack
            {
                .buffer         = port_nic_from,
                .len            = PAGE_SIZE
            },

            .to = // stack -> NIC
            {
                .buffer         = port_nic_to,
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
            .notify_init_done   = event_network_init_done_emit,

            .port =
            {
                .buffer         = port_app_io,
                .len            = PAGE_SIZE
            },
        }
    };

    seos_err_t ret = seos_network_stack_run(&camkes_config, &config);
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_FATAL("seos_network_stack_run() for #1 (client) failed, error %d", ret);
        return -1;
    }

    Debug_LOG_WARNING("network stack #1 (client) terminated gracefully");

    return 0;
}
