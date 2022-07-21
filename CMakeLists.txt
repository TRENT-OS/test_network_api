#
# Test Network API
#
# Copyright (C) 2020, HENSOLDT Cyber GmbH
#

cmake_minimum_required(VERSION 3.7.2)

#os_set_config_file(system_config.h CREATE_CONFIG_PROJECT)
find_package("os-sdk" REQUIRED)
os_sdk_set_defaults()
os_sdk_setup(CONFIG_FILE "system_config.h" CONFIG_PROJECT "system_config")
CAmkESAddCPPInclude(".")
CAmkESAddCPPInclude("plat/${PLATFORM}")
#-------------------------------------------------------------------------------
project(tests_picotcp_api C)

if(NOT TEST_CONFIGURATION)
    # set default test configuration to be build
    set(TEST_CONFIGURATION tcp_server CACHE STRING "")
endif()

include("plat/${PLATFORM}/plat_nic.cmake")
include("test_configuration/${TEST_CONFIGURATION}/test_configuration.cmake")

#DeclareAndCreateCamkESSystem(test_configuration/${TEST_CONFIGURATION}/main.camkes)
os_sdk_create_CAmkES_system(test_configuration/${TEST_CONFIGURATION}/main.camkes)
#GenerateSimulateScript()
