/**
 * Copyright (C) 2019, Hensoldt Cyber GmbH
 *
 * SEOS libraries configurations
 *
 */
#pragma once


//-----------------------------------------------------------------------------
// Debug
//-----------------------------------------------------------------------------

#if !defined(NDEBUG)
#   define Debug_Config_STANDARD_ASSERT
#   define Debug_Config_ASSERT_SELF_PTR
#else
#   define Debug_Config_DISABLE_ASSERT
#   define Debug_Config_NO_ASSERT_SELF_PTR
#endif

#define Debug_Config_LOG_LEVEL              Debug_LOG_LEVEL_DEBUG
#define Debug_Config_INCLUDE_LEVEL_IN_MSG
#define Debug_Config_LOG_WITH_FILE_LINE


//-----------------------------------------------------------------------------
// Memory
//-----------------------------------------------------------------------------

#define Memory_Config_USE_STDLIB_ALLOC


//-----------------------------------------------------------------------------
// ChanMUX
//-----------------------------------------------------------------------------
enum
{
    CHANNEL_UNUSED_0,        // 0
    CHANNEL_UNUSED_1,        // 1
    CHANNEL_UNUSED_2,        // 2
    CHANNEL_UNUSED_3,        // 3
    CHANNEL_NW_STACK_CTRL,   // 4
    CHANNEL_NW_STACK_DATA,   // 5
    CHANNEL_UNUSED_6,        // 6
    CHANNEL_NW_STACK_CTRL_2, // 7
    CHANNEL_NW_STACK_DATA_2, // 8

    CHANMUX_NUM_CHANNELS     // 9
};


//-----------------------------------------------------------------------------
// Network Stack
//-----------------------------------------------------------------------------

#define SEOS_TAP0_CTRL_CHANNEL          CHANNEL_NW_STACK_CTRL
#define SEOS_TAP0_DATA_CHANNEL          CHANNEL_NW_STACK_DATA

#define SEOS_TAP1_CTRL_CHANNEL          CHANNEL_NW_STACK_CTRL_2
#define SEOS_TAP1_DATA_CHANNEL          CHANNEL_NW_STACK_DATA_2

/* Below defines are used to configure the network stack*/
#define SEOS_TAP0_ADDR                  "192.168.82.91"
#define SEOS_TAP0_GATEWAY_ADDR          "192.168.82.1"
#define SEOS_TAP0_SUBNET_MASK           "255.255.255.0"

#define SEOS_TAP1_ADDR                  "192.168.82.92"
#define SEOS_TAP1_GATEWAY_ADDR          "192.168.82.1"
#define SEOS_TAP1_SUBNET_MASK           "255.255.255.0"


//-----------------------------------------------------------------------------
// Client Test App
//-----------------------------------------------------------------------------
#define CFG_TEST_HTTP_SERVER            "192.168.82.12"
