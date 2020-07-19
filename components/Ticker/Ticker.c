/*
 *  Ticker
 *
 *  Copyright (C) 2019, Hensoldt Cyber GmbH
 *
 */

#include "LibDebug/Debug.h"
#include "TimeServer.h"
#include <camkes.h>


//------------------------------------------------------------------------------
int run(void)
{
    Debug_LOG_INFO("ticker running");

    // set up a tick every second
    int ret = timeServer_rpc_periodic(0, NS_IN_S);
    if (0 != ret)
    {
        Debug_LOG_ERROR("timeServer_rpc_periodic() failed, code %d", ret);
        return -1;
    }

    seL4_CPtr timeServer_notification = timeServer_rpc_notification();

    uint64_t timestamp = TimeServer_getTime(TimeServer_PRECISION_NSEC);
    for(;;)
    {
        seL4_Word badge = 0;

        seL4_Wait(timeServer_notification, &badge);

        uint64_t timestamp_new = TimeServer_getTime(TimeServer_PRECISION_NSEC);
        uint64_t  delta = timestamp_new - timestamp;
        Debug_LOG_INFO(
            "tick, delta %" PRIu64 ".%" PRIu64 " sec",
            delta / NS_IN_S,
            delta % NS_IN_S);
        timestamp = timestamp_new;

        // send tick to both network stacks
        nwStack1_event_tick_emit();
        nwStack2_event_tick_emit();
    }
}
