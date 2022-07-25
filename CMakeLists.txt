#
# Test Network API
#
# Copyright (C) 2020-2022, HENSOLDT Cyber GmbH
#

cmake_minimum_required(VERSION 3.7.2)

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
