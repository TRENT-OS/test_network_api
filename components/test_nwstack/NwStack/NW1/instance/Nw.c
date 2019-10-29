/*
 *  SEOS Network Stack CAmkES wrapper
 *
 *  Copyright (C) 2019, Hensoldt Cyber GmbH
 *
 */
#include "Seos_Nw_Config.h"

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
        .ChanMuxDataPort     =  chanMuxDataPort,
        .ChanMuxCtrlPort     =  chanMuxCtrlDataPort,
        .Appdataport         =  NwAppDataPort
    };

    Seos_nw_camkes_info nw_camkes =
    {
        &nw_signal,
        &nw_data,
        pico_chan_mux_tap_create
    };

    /* First init Nw driver and then init Nw stack */

#ifdef SEOS_NWSTACK_AS_CLIENT
#if (SEOS_NETWORK_TAP0_PROXY == IN_USE)
    Seos_TapDriverConfig nw_driver_config_client =
    {
        .name      = SEOS_TAP_DRIVER_NAME,
        .chan_ctrl = SEOS_TAP_CTRL_CHANNEL,
        .chan_data = SEOS_TAP_DATA_CHANNEL

    };
    ret = Seos_NwDriver_TapConfig(&nw_driver_config_client);
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_FATAL("%s():Nw Driver Tap Config Failed for Client!", __FUNCTION__);
        exit(0);
    }

#else

    // Config driver for other interfaces e.g. ethernet

#endif

#endif

    ret = Seos_NwStack_init(&nw_camkes);

    /* is possible when proxy does not run with tap =1 param. Just print and exit*/
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_WARNING("Network Stack Init() Failed as Client...Exiting NwStack. Error:%d\n",
                          ret);
    }
    return 0;
}
