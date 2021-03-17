#
# Test Network API
#
# Copyright (C) 2020, Hensoldt Cyber GmbH
#

cmake_minimum_required(VERSION 3.7.2)

os_set_config_file(system_config.h CREATE_CONFIG_PROJECT)


#-------------------------------------------------------------------------------
project(tests_picotcp_api C)

include("plat/${PLATFORM}/plat_nic.cmake" OPTIONAL)

TimeServer_DeclareCAmkESComponent(
    TimeServer
)

DeclareCAmkESComponent(
    Ticker
    SOURCES
        components/Ticker/Ticker.c
    C_FLAGS
        -Wall
        -Werror
    LIBS
        os_core_api
        lib_debug
)

DeclareCAmkESComponent(
    NwStack1
    SOURCES
        components/NwStack/network_stack.c
        components/NwStack/util/helper_func.c
    C_FLAGS
        -Wall
        -Werror
        -DOS_NWSTACK_AS_CLIENT
        -DOS_CONFIG_SERVICE_CAMKES_CLIENT
        #-DDEV_ADDR="192.168.82.231"
        #-DGATEWAY_ADDR="192.168.82.1"
        #-DSUBNET_MASK="255.255.255.0"
        -DOS_NETWORK_STACK_USE_CONFIGSERVER
    LIBS
        system_config
        os_core_api
        lib_debug
        os_configuration
        os_network_lib
        TimeServer_client
        syslogger_client
)

DeclareCAmkESComponent(
    NwStack2
    SOURCES
        components/NwStack/network_stack.c
        components/NwStack/util/helper_func.c
    C_FLAGS
        -Wall
        -Werror
        -DOS_NWSTACK_AS_SERVER
        -DOS_CONFIG_SERVICE_CAMKES_CLIENT
        #-DDEV_ADDR="192.168.82.232"
        #-DGATEWAY_ADDR="192.168.82.1"
        #-DSUBNET_MASK="255.255.255.0"
        -DOS_NETWORK_STACK_USE_CONFIGSERVER
    LIBS
        system_config
        os_core_api
        lib_debug
        os_configuration
        os_network_lib
        TimeServer_client
        syslogger_client
)

DeclareCAmkESComponent(
    NwApp1
    SOURCES
        components/TestAppClient/test_app_client.c
    C_FLAGS
        -Wall
        -Werror
    LIBS
        system_config
        os_core_api
        lib_debug
        lib_macros
        os_network_api
        syslogger_client
)

DeclareCAmkESComponent(
    NwApp2
    SOURCES
        components/TestAppServer/test_app_server.c
    C_FLAGS
        -Wall
        -Werror
    LIBS
        system_config
        os_core_api
        lib_debug
        os_network_api
        syslogger_client
)

DeclareCAmkESComponent_SysLogger(
    SysLogger
    system_config
)

DeclareCAmkESComponent(
    ConfigServer
    SOURCES
        components/ConfigServer/src/ConfigServer.c
        components/ConfigServer/src/init_lib_with_mem_backend.c
        components/ConfigServer/src/create_parameters.c
    C_FLAGS
        -Wall
        -Werror
        -DOS_CONFIG_SERVICE_BACKEND_MEMORY
        -DOS_CONFIG_SERVICE_CAMKES_SERVER
    LIBS
        system_config
        lib_debug
        os_core_api
        os_configuration
)


DeclareAndCreateCamkESSystem(main.camkes)
GenerateSimulateScript()
