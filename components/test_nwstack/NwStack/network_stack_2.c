/*
 *  SEOS Network Stack CAmkES wrapper
 *
 *  Copyright (C) 2019, Hensoldt Cyber GmbH
 *
 */

#include "Seos_Nw_Config.h"
#include "LibDebug/Debug.h"
#include "SeosError.h"
#include "seos_api_network_stack.h"
#include <camkes.h>




/*
    Debug_LOG_INFO("initializing network stack as Server...\n");
    int ret;

    seos_nw_camkes_signal_glue  nw_signal =
    {
        .e_write_emit        =  e_write_2_emit,
        .c_write_wait        =  c_write_2_wait,
        .e_read_emit         =  e_read_2_emit,
        .c_read_wait         =  c_read_2_wait,
        .e_conn_emit         =  e_conn_2_emit,
        .c_conn_wait         =  c_conn_2_wait,
        .e_write_nwstacktick =  e_write_nwstack_tick_2_emit,
        .c_nwstacktick_wait  =  c_nwstack_tick_2_wait,
        .e_initdone          =  e_initdone_2_emit,
        // .c_initdone          =  c_initdone_2_wait
    };

    seos_nw_ports_glue nw_data =
    {
        .nwdriver_ReadPort     = driverReadPort_2,
        .nwdriver_WritePort    = driverWritePort_2,
        .Appdataport           = NwAppDataPort_2

    };

    seos_nw_driver_rpc_api nw_driver_api =
    {
        .dev_write             = seos_driver_tx_data,
        .get_mac               = seos_driver_get_mac,
        .dev_link_state        = NULL
    };

    // Wait for an event here from Driver to get Initialsed
    // < ---------------------------->
    c_driver_initdone_2_wait();


    // this runs the network stack main loop, it does not return during normal
    // operation
    ret = Seos_NwStack_init(&nw_camkes, &nw_stack_config);
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_FATAL("Seos_NwStack_init Init() for server failed, error %d", ret);
        return -1;
    }

    Debug_LOG_WARNING("network stack for server terminated");

    return 0;
}
*/


static const seos_network_stack_config_t config =
{
    .dev_addr      = SEOS_TAP1_ADDR,
    .gateway_addr  = SEOS_TAP1_GATEWAY_ADDR,
    .subnet_mask   = SEOS_TAP1_SUBNET_MASK
};


//------------------------------------------------------------------------------
int run()
{
    Debug_LOG_INFO("driver up, starting network stack #2 (server)\n");

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

            .notify_connection  = e_conn_emit,
            .wait_connection    = c_conn_wait,
        },

        .drv_nic =
        {
            .wait_init_done     = event_nic_init_done_wait,

            .from =
            {
                .buffer         = port_nic_from, // NIC -> stack
                .len            = PAGE_SIZE
            },

            .to =
            {
                .buffer         = port_nic_to,   // stack -> NIC
                .len            = PAGE_SIZE
            },

            .rpc =
            {
                .dev_write      = nic_driver_tx_data,
                .get_mac        = nic_driver_get_mac
            },
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
        Debug_LOG_FATAL("seos_network_stack_run() for #2 (server) failed, error %d", ret);
        return -1;
    }

    Debug_LOG_WARNING("network stack 2 (server terminated gracefully");

    return 0;
}
