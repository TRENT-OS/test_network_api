/*
 *  Seos_NwConfig.h
 *
 *  Copyright (C) 2019, Hensoldt Cyber GmbH
 *  Description: Helps configure network stack and network driver
*/

#pragma once

#include <camkes.h>
#include <string.h>
#include "LibDebug/Debug.h"
#include "SeosNwStack.h"
#include "ChanMux_config.h"
#include "Seos_pico_dev_chan_mux.h"

#define IN_USE                                              1
#define NOT_IN_USE                                          0
/* ########## USER CONFIGURATION FOR NETWORK DRIVER    ##################### */
/* Below params configure the driver, as of now we only have TAP proxy driver */


#ifdef  SEOS_NWSTACK_AS_CLIENT
#define SEOS_NETWORK_TAP0_PROXY                          IN_USE
// We could use other defines here e.g.ethernet,  instead of tap0

#elif   SEOS_NWSTACK_AS_SERVER
#define SEOS_NETWORK_TAP1_PROXY                          IN_USE
// We could use other defines here e.g.ethernet,  instead of tap1
#else
#error "Error:Either configure as Client or Server!!"
#endif


#if (SEOS_NETWORK_TAP0_PROXY == IN_USE)
/* Below defines are used to configure the network driver*/
#define SEOS_TAP_DRIVER_NAME           "tap0"
#define SEOS_TAP_CTRL_CHANNEL          CHANNEL_NW_STACK_CTRL
#define SEOS_TAP_DATA_CHANNEL          CHANNEL_NW_STACK_DATA

/* Below defines are used to configure the network stack*/
#define SEOS_NW_TAP_ADDR               "192.168.82.91"
#define SEOS_NW_GATEWAY_ADDR           "192.168.82.1"
#define SEOS_NW_SUBNET_MASK            "255.255.255.0"

#endif

#if (SEOS_NETWORK_TAP1_PROXY == IN_USE)
/* Below defines are used to configure the network driver*/
#define SEOS_TAP_DRIVER_NAME           "tap1"
#define SEOS_TAP_CTRL_CHANNEL          CHANNEL_NW_STACK_CTRL_2
#define SEOS_TAP_DATA_CHANNEL          CHANNEL_NW_STACK_DATA_2

/* Below defines are used to configure network stack*/
#define SEOS_NW_TAP_ADDR               "192.168.82.92"
#define SEOS_NW_GATEWAY_ADDR           "192.168.82.1"
#define SEOS_NW_SUBNET_MASK            "255.255.255.0"

#endif
