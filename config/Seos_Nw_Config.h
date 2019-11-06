/*
 *  Seos_Nw_Config.h
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
#include "Seos_Driver_Config.h"

/* ########## USER CONFIGURATION FOR NETWORK DRIVER    ##################### */
/* Below params configure the driver, as of now we only have TAP proxy driver */

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
