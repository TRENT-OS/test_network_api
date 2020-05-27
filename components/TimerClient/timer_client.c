/*
 *  OS Network Stack CAmkES App for timer client
 *
 *  Copyright (C) 2019, Hensoldt Cyber GmbH
 *
 */

#include "LibDebug/Debug.h"
#include <camkes.h>

#define MSECS_TO_SLEEP      10
// It looks like the events from the stack aren't coming as often as they should, and we
// have to be saved by the timer. Setting it to a random low value that seems to work
// SEOS-930
#define SIGNAL_PERIOD_MS    50

static unsigned int counterMs = 0;

//------------------------------------------------------------------------------
int
run(void)
{
    Debug_LOG_INFO("Starting TimerClient");

    for(;;)
    {
        Timer_sleep(MSECS_TO_SLEEP);
        counterMs += MSECS_TO_SLEEP;
        if ((counterMs % SIGNAL_PERIOD_MS) == 0)
        {
            // Debug_LOG_DEBUG("sending tick");
            nwStack_1_event_tick_emit();
            nwStack_2_event_tick_emit();
        }
    }
    return 0;
}


//------------------------------------------------------------------------------
unsigned int
TimerClient_getTimeMs(void)
{
    return counterMs;
}
