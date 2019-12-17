cmake_minimum_required(VERSION 3.7.2)

seos_set_config_file(config.h)
set(SEOS_NW_CONFIG_H_FILE ${CMAKE_CURRENT_LIST_DIR}/config/Seos_Nw_Config.h)

seos_use_libs(
    SEOS_LIBS
    SEOS_NETWORK_DRIVER_TAP_PROXY
    SEOS_NW_STACK
    SEOS_CONFIGURATION
)

project(tests_picotcp_api C)

DeclareCAmkESComponent(NwStack_1
    INCLUDES
        components/test_nwstack/ChanMux/src/instance
        config
        components/test_nwstack/CONFIGURATION/src
    SOURCES
        components/test_nwstack/NwStack/network_stack_1.c
       # components/test_nwstack/NwStack/network_configuration.c
    C_FLAGS
        -Wall -Werror
        -DSEOS_NWSTACK_AS_CLIENT
        -DSEOS_CONFIG_CAMKES_CLIENT    # indicates a camkes configuration server will be used
    LIBS
        seos_core_api
        seos_libs
        seos_nw_lib
        seos_configuration
)

DeclareCAmkESComponent(NwDriver_1
    INCLUDES
        components/test_nwstack/ChanMux/src/instance
        config
    SOURCES
        components/test_nwstack/NwDriver/nic_driver_common.c
        components/test_nwstack/NwDriver/nic_driver_1.c
    C_FLAGS
        -Wall -Werror
    LIBS
        seos_core_api
        seos_libs
        seos_nw_driver_tap_proxy_lib
)

DeclareCAmkESComponent(NwStack_2
    INCLUDES
        components/test_nwstack/ChanMux/src/instance
        config
        components/test_nwstack/CONFIGURATION/src
    SOURCES
        components/test_nwstack/NwStack/network_stack_2.c
        #components/test_nwstack/NwStack/network_configuration.c
    C_FLAGS
        -Wall -Werror
        -DSEOS_NWSTACK_AS_SERVER
        -DSEOS_CONFIG_CAMKES_CLIENT    # indicates a camkes configuration server will be used
    LIBS
        seos_core_api
        seos_libs
        seos_nw_lib
        seos_configuration
)

DeclareCAmkESComponent(NwDriver_2
    INCLUDES
        components/test_nwstack/ChanMux/src/instance
        config
    SOURCES
        components/test_nwstack/NwDriver/nic_driver_common.c
        components/test_nwstack/NwDriver/nic_driver_2.c
    C_FLAGS
        -Wall -Werror
    LIBS
        seos_core_api
        seos_libs
        seos_nw_driver_tap_proxy_lib
)

DeclareCAmkESComponent(NwApp_1
    INCLUDES
        components/test_nwstack/test_APP/include
    SOURCES
        components/test_nwstack/test_APP/test_NwApp/src/Client_App.c
        ${SEOS_LIBS_DIR}/seos_nwstack/network-client-lib/seos_nw_api_interface.c
    C_FLAGS
        -Wall -Werror
        -DCLIENT_CONFIG=1
    LIBS
        seos_core_api
        seos_libs
)

DeclareCAmkESComponent(NwApp_2
    INCLUDES
        components/test_nwstack/test_APP/include
    SOURCES
        components/test_nwstack/test_APP/test_NwApp2/src/Server_App.c
        ${SEOS_LIBS_DIR}/seos_nwstack/network-client-lib/seos_nw_api_interface.c
    C_FLAGS
        -Wall -Werror
        -DSERVER_CONFIG=1
    LIBS
        seos_core_api
        seos_libs
)

# CONFIGURATION #
DeclareCAmkESComponent(CONFIGURATION_SERVER
    SOURCES
        components/test_nwstack/CONFIGURATION/src/main.c
        components/test_nwstack/CONFIGURATION/src/init_lib_with_mem_backend.c
        components/test_nwstack/CONFIGURATION/src/create_parameters.c
        components/test_nwstack/CONFIGURATION/src/dump_config_lib.c
    C_FLAGS
        -Wall
        -Werror
        -DSEOS_CFG_CONFIG_H_FILE=${SEOS_CFG_CONFIG_H_FILE}
        -DSEOS_CONFIG_CAMKES_SERVER     # indicates a camkes configuration server is being built
    LIBS
        seos_libs
        seos_configuration
)

DeclareCAmkESComponent(ChanMux
    SOURCES
        components/test_nwstack/ChanMux/src/instance/ChanMux_Instance.c
    C_FLAGS
        -Wall -Werror
    LIBS
        seos_core_api
        seos_libs
)

DeclareCAmkESComponent(TimerClient
    SOURCES
        components/test_nwstack/TimerClient/src/Timer_Client.c
    C_FLAGS
        -Wall -Werror
    LIBS
        seos_core_api
        seos_libs
)

DeclareCAmkESComponent(Timer
    SOURCES
        components/test_nwstack/Timer/src/timer.c
    C_FLAGS
        -Wall -Werror
)

DeclareCAmkESComponent(UartDrv
    SOURCES
        components/test_nwstack/Uart/src/UartDrv.c
        components/test_nwstack/Uart/src/qemu_uart.c
    C_FLAGS
        -Wall -Werror
)

DeclareCAmkESRootserver(main.camkes)
GenerateCAmkESRootServer()
GenerateSimulateScript()
