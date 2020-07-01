#
# Test Network API
#
# Copyright (C) 2020, Hensoldt Cyber GmbH
#

cmake_minimum_required(VERSION 3.7.2)

os_set_config_file(system_config.h)


#-------------------------------------------------------------------------------
project(system_config C)
add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR})


#-------------------------------------------------------------------------------
project(tests_picotcp_api C)

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
        -DOS_NETWORK_STACK_USE_CONFIGSERVER
    LIBS
        system_config
        os_core_api
        os_libs
        os_configuration
        os_network_lib
)

DeclareCAmkESComponent_NIC_ChanMux(
    NwDriver1
    CHANMUX_CHANNEL_NIC_1_CTRL
    CHANMUX_CHANNEL_NIC_1_DATA
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
        -DOS_NETWORK_STACK_USE_CONFIGSERVER
    LIBS
        system_config
        os_core_api
        os_libs
        os_configuration
        os_network_lib
)

DeclareCAmkESComponent_NIC_ChanMux(
    NwDriver2
    CHANMUX_CHANNEL_NIC_2_CTRL
    CHANMUX_CHANNEL_NIC_2_DATA
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
        os_libs
        os_network_api
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
        os_libs
        os_network_api
)

DeclareCAmkESComponent_ChanMux(
    ChanMux
    components/ChanMux/ChanMux_config.c
    system_config
)

DeclareCAmkESComponent(
    TimerClient
    SOURCES
        components/TimerClient/timer_client.c
    C_FLAGS
        -Wall
        -Werror
    LIBS
        system_config
        os_core_api
        os_libs
)

DeclareCAmkESComponent(
    Timer
    SOURCES
        components/Timer/timer.c
    C_FLAGS
        -Wall
        -Werror
)

DeclareCAmkESComponent_UART(
    UART
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
        os_libs
        os_core_api
        os_configuration
)


DeclareAndCreateCamkESSystem(main.camkes)
GenerateSimulateScript()
