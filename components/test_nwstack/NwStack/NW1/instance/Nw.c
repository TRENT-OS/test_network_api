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
    Debug_LOG_INFO("starting network stack as Client...\n");

    int ret;

    seos_nw_camkes_signal_glue nw_signal =
    {
        .e_write_emit        =  e_write_emit,
        .c_write_wait        =  c_write_wait,
        .e_read_emit         =  e_read_emit,
        .c_read_wait         =  c_read_wait,
        .e_conn_emit         =  NULL,
        .c_conn_wait         =  NULL,
        .e_write_nwstacktick =  e_write_nwstacktick_emit,
        .c_nwstacktick_wait  =  c_nwstacktick_wait,
        .e_initdone          =  e_initdone_emit,
        .c_initdone          =  c_initdone_wait
    };

    seos_nw_ports_glue nw_data =
    {
        .ChanMuxDataPortRead  =  chanMuxDataPortRead,
        .ChanMuxDataPortWrite =  chanMuxDataPortWrite,
        .ChanMuxCtrlPort      =  chanMuxCtrlDataPort,
        .Appdataport          =  NwAppDataPort
    };

    /* First init Nw driver and then init Nw stack. Driver fills the device
     * create callback
     */
    seos_driver_config nw_driver_config_client =
    {
        .chan_ctrl              = SEOS_TAP0_CTRL_CHANNEL,
        .chan_data              = SEOS_TAP0_DATA_CHANNEL,
        .device_create_callback = NULL
    };

    ret = Seos_NwDriver_init(&nw_driver_config_client);
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_FATAL("Seos_NwDriver_init() for client failed, error %d", ret);
        return -1;
    }

    seos_nw_config nw_stack_config =
    {
        .dev_addr               = SEOS_TAP0_ADDR,
        .gateway_addr           = SEOS_TAP0_GATEWAY_ADDR,
        .subnet_mask            = SEOS_TAP0_SUBNET_MASK,
        .driver_create_device   = nw_driver_config_client.device_create_callback
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
        Debug_LOG_FATAL("Seos_NwStack_init Init() for client failed, error %d", ret);
        return -1;
    }

    Debug_LOG_WARNING("network stack for client terminated");

    return 0;
}
