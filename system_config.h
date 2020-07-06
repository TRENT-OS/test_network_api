/**
 * Copyright (C) 2019, Hensoldt Cyber GmbH
 *
 * OS libraries configurations
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

#define CHANMUX_CHANNEL_NIC_1_CTRL 4
#define CHANMUX_CHANNEL_NIC_1_DATA 5

#define CHANMUX_CHANNEL_NIC_2_CTRL 7
#define CHANMUX_CHANNEL_NIC_2_DATA 8

//-----------------------------------------------------------------------------
// ChanMUX clients
//-----------------------------------------------------------------------------

#define CHANMUX_ID_NIC_1 101
#define CHANMUX_ID_NIC_2 102

//-----------------------------------------------------------------------------
// Network Stack
//-----------------------------------------------------------------------------

#define CFG_TEST_HTTP_SERVER "10.0.0.1"

// App1
#define DOMAIN_NWSTACK "STACK"

#define CFG_ETH_ADDR_CLIENT       "ETH_ADDR_CLIENT"
#define CFG_ETH_ADDR_CLIENT_VALUE "10.0.0.10"

#define CFG_ETH_ADDR_SERVER       "ETH_ADDR_SERVER"
#define CFG_ETH_ADDR_SERVER_VALUE "10.0.0.11"

#define CFG_ETH_GATEWAY_ADDR       "ETH_GATEWAY_ADDR"
#define CFG_ETH_GATEWAY_ADDR_VALUE "10.0.0.1"

#define CFG_ETH_SUBNET_MASK       "ETH_SUBNET_MASK"
#define CFG_ETH_SUBNET_MASK_VALUE "255.255.255.0"

#ifndef OS_NETWORK_MAXIMUM_SOCKET_NO
#define OS_NETWORK_MAXIMUM_SOCKET_NO 16
#endif