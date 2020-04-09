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
#define Debug_Config_STANDARD_ASSERT
#define Debug_Config_ASSERT_SELF_PTR
#else
#define Debug_Config_DISABLE_ASSERT
#define Debug_Config_NO_ASSERT_SELF_PTR
#endif

#define Debug_Config_LOG_LEVEL Debug_LOG_LEVEL_DEBUG
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
    CHANMUX_CHANNEL_UNUSED_0,   // 0
    CHANMUX_CHANNEL_UNUSED_1,   // 1
    CHANMUX_CHANNEL_UNUSED_2,   // 2
    CHANMUX_CHANNEL_UNUSED_3,   // 3
    CHANMUX_CHANNEL_NIC_1_CTRL, // 4
    CHANMUX_CHANNEL_NIC_1_DATA, // 5
    CHANMUX_CHANNEL_UNUSED_6,   // 6
    CHANMUX_CHANNEL_NIC_2_CTRL, // 7
    CHANMUX_CHANNEL_NIC_2_DATA, // 8

    CHANMUX_NUM_CHANNELS // 9
};

#define CFG_TEST_HTTP_SERVER "192.168.82.12"

//App1
#define DOMAIN_NWSTACK "STACK"

#define CFG_ETH_ADDR_CLIENT "ETH_ADDR_CLIENT"
#define CFG_ETH_ADDR_CLIENT_VALUE "10.0.0.10"

#define CFG_ETH_ADDR_SERVER "ETH_ADDR_SERVER"
#define CFG_ETH_ADDR_SERVER_VALUE "10.0.0.11"

#define CFG_ETH_GATEWAY_ADDR "ETH_GATEWAY_ADDR"
#define CFG_ETH_GATEWAY_ADDR_VALUE "10.0.0.1"

#define CFG_ETH_SUBNET_MASK "ETH_SUBNET_MASK"
#define CFG_ETH_SUBNET_MASK_VALUE "255.255.255.0"