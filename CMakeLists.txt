#
# Test Network API
#
# Copyright (C) 2020-2024, HENSOLDT Cyber GmbH
# 
# SPDX-License-Identifier: GPL-2.0-or-later
#
# For commercial licensing, contact: info.cyber@hensoldt.net
#

cmake_minimum_required(VERSION 3.7.2)

#-------------------------------------------------------------------------------
# Config options, which can be configured via cli


set(DEV_ADDR "10.0.0.10" CACHE STRING "Set ip of TRENTOS")
set(GATEWAY_ADDR "10.0.0.1" CACHE STRING "Ip of the device hosting the test container")
set(SUBNET_MASK "255.255.255.0" CACHE STRING "Subnet mask")
set(FORBIDDEN_HOST "10.0.0.1" CACHE STRING "Forbidden host test addr")
set(REACHABLE_HOST "10.0.0.1" CACHE STRING "Reachable host test addr")
set(ETH_ADDR_CLIENT_VALUE "10.0.0.10" CACHE STRING "Ip of the client")
set(ETH_ADDR_SERVER_VALUE "10.0.0.11" CACHE STRING "Ip of the server")


#-------------------------------------------------------------------------------




# use the SDK
find_package("os-sdk" REQUIRED)
os_sdk_set_defaults()
os_sdk_setup(CONFIG_FILE "system_config.h" CONFIG_PROJECT "system_config")

# Set additional include paths.
CAmkESAddCPPInclude("plat/${PLATFORM}")
# The CAmkES system description file is not in the root folder, thus this folder
# is not included in the search path by default and needs to be added manually.
CAmkESAddCPPInclude(".")



#-------------------------------------------------------------------------------
project(tests_picotcp_api C)

if(NOT TEST_CONFIGURATION)
    # set default test configuration to be build
    set(TEST_CONFIGURATION tcp_server CACHE STRING "")
endif()

include("plat/${PLATFORM}/plat_nic.cmake")
include("test_configuration/${TEST_CONFIGURATION}/test_configuration.cmake")

os_sdk_create_CAmkES_system("test_configuration/${TEST_CONFIGURATION}/main.camkes")
