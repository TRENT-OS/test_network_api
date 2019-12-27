/*
 *  Network Driver #1
 *
 *  Copyright (C) 2019, Hensoldt Cyber GmbH
 */

#include "seos_system_config.h"

#include "LibDebug/Debug.h"
#include "nic_driver_common.h"
#include <camkes.h>


//------------------------------------------------------------------------------
int run()
{
    Debug_LOG_INFO("starting network driver #1");

    int ret = chanmux_nic_driver_start(CHANMUX_CHANNEL_NIC_0_CTRL,
                                       CHANMUX_CHANNEL_NIC_0_DATA);
    if (ret < 0)
    {
        Debug_LOG_ERROR("chanmux_nic_driver_start() failed for driver #1, error %d", ret);
        return -1;
    }

    // actually, the driver is not supposed to return without an error. If it
    // does, we have to assume it wants to shutdown gracefully for some reason.
    Debug_LOG_WARNING("network driver #1 terminated gracefully");

    return 0;
}
