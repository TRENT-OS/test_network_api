/*
 *  SEOS Network Stack CAmkES wrapper
 *
 *  Copyright (C) 2019, Hensoldt Cyber GmbH
 *
 */

#include "SeosNwStack.h"
#include "Seos_Nw_Config.h"
#include "Seos_Driver_Config.h"
#include "LibDebug/Debug.h"
#include <camkes.h>

int run()
{
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
        .e_write_nwstacktick =  e_write_nwstacktick_2_emit,
        .c_nwstacktick_wait  =  c_nwstacktick_2_wait,
        .e_initdone          =  e_initdone_2_emit,
        .c_initdone          =  c_initdone_2_wait
    };

    seos_nw_ports_glue nw_data =
    {
        .ChanMuxDataPort = chanMuxDataPort_2,
        .ChanMuxCtrlPort = chanMuxCtrlDataPort_2,
        .Appdataport     = NwAppDataPort_2

    };

    /* First init Nw driver and then init Nw stack. Driver fills the device
     * create callback to be used for Nw Stack
     */
    seos_driver_config nw_driver_config_server =
    {
        .chan_ctrl              = SEOS_TAP1_CTRL_CHANNEL,
        .chan_data              = SEOS_TAP1_DATA_CHANNEL,
        .device_create_callback = NULL
    };

    ret = Seos_NwDriver_init(&nw_driver_config_server);

    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_FATAL("Seos_NwDriver_init() for server failed, error %d", ret);
        return -1;
    }

    seos_nw_config nw_stack_config =
    {
        .dev_addr             = SEOS_TAP1_ADDR,
        .gateway_addr         = SEOS_TAP1_GATEWAY_ADDR,
        .subnet_mask          = SEOS_TAP1_SUBNET_MASK,
        .driver_create_device = nw_driver_config_server.device_create_callback
    };

    Seos_nw_camkes_info nw_camkes =
    {
        &nw_signal,
        &nw_data,
    };

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
