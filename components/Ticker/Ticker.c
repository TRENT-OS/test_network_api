/*
 *  Ticker
 *
 *  Copyright (C) 2019, Hensoldt Cyber GmbH
 *
 */

#include "LibDebug/Debug.h"
#include <camkes.h>


//------------------------------------------------------------------------------
int run(void)
{
    Debug_LOG_INFO("ticker running");

    // set up a tick every second
    int ret = timeServer_rpc_periodic(0, 1 * NS_IN_MS);
    if (0 != ret)
    {
        Debug_LOG_ERROR("timeServer_rpc_periodic() failed, code %d", ret);
        return -1;
    }

    uint64_t timestamp;
    timeServer_rpc_time(&timestamp);
    for(;;)
    {
        timeServer_notify_wait();

        uint64_t timestamp_new, delta;

        timeServer_rpc_time(&timestamp_new);
        delta = timestamp_new - timestamp;

        Debug_LOG_DEBUG(
            "tick, delta %" PRIu64 ".%" PRIu64 " sec",
            delta / NS_IN_S,
            delta % NS_IN_S);
        timestamp = timestamp_new;

        // send tick to both network stacks
        nwStack1_event_tick_emit();
        nwStack2_event_tick_emit();
    }
}
