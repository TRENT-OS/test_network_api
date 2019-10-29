/*
 *  SEOS Network Stack CAmkES wrapper
 *
 *  Copyright (C) 2019, Hensoldt Cyber GmbH
 *
 */
#include "Seos_Nw_Config.h"

int run()
{
    Debug_LOG_INFO("starting network stack as Server...\n");
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

    Seos_nw_camkes_info nw_camkes =
    {
        &nw_signal,
        &nw_data,
        pico_chan_mux_tap_create
    };

    /* First init Nw driver and then init Nw stack */

#ifdef SEOS_NWSTACK_AS_SERVER
#if (SEOS_NETWORK_TAP1_PROXY == IN_USE)
    Seos_TapDriverConfig nw_driver_config_server =
    {
        .name      = SEOS_TAP_DRIVER_NAME,
        .chan_ctrl = SEOS_TAP_CTRL_CHANNEL,
        .chan_data = SEOS_TAP_DATA_CHANNEL
    };
    ret = Seos_NwDriver_TapConfig(&nw_driver_config_server);
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_FATAL("%s():Nw Driver Tap Config Failed for Server!", __FUNCTION__);
        exit(0);
    }
#else

    // Config driver for other interfaces e.g. ethernet
#endif

#endif

    // should never return as this starts pico_stack_tick().

    ret = Seos_NwStack_init(&nw_camkes);

    /*is possible when proxy does not run with use_tap=1 param. Just print and exit*/
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_WARNING("Network Stack Init()-Sever Failed...Exiting NwStack-2. Error:%d\n",
                          ret);
    }
    return 0;
}
