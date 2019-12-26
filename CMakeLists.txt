cmake_minimum_required(VERSION 3.7.2)

seos_set_config_file(config.h)
set(SEOS_NW_CONFIG_H_FILE ${CMAKE_CURRENT_LIST_DIR}/config/Seos_Nw_Config.h)

seos_use_libs(
    SEOS_LIBS
    SEOS_NETWORK_DRIVER_TAP_PROXY
    SEOS_NW_STACK
)

project(tests_picotcp_api C)

DeclareCAmkESComponent(NwStack_1
    INCLUDES
        components/ChanMux/src/instance
        config
    SOURCES
        components/NwStack/network_stack_1.c
    C_FLAGS
        -Wall -Werror
        -DSEOS_NWSTACK_AS_CLIENT
    LIBS
        seos_core_api
        seos_libs
        seos_nw_lib
)

DeclareCAmkESComponent(NwDriver_1
    INCLUDES
        components/ChanMux/src/instance
        config
    SOURCES
        components/NwDriver/nic_driver_common.c
        components/NwDriver/nic_driver_1.c
    C_FLAGS
        -Wall -Werror
    LIBS
        seos_core_api
        seos_libs
        seos_nw_driver_tap_proxy_lib
)

DeclareCAmkESComponent(NwStack_2
    INCLUDES
        components/ChanMux/src/instance
        config
    SOURCES
        components/NwStack/network_stack_2.c
    C_FLAGS
        -Wall -Werror
        -DSEOS_NWSTACK_AS_SERVER
    LIBS
        seos_core_api
        seos_libs
        seos_nw_lib
)

DeclareCAmkESComponent(NwDriver_2
    INCLUDES
        components/ChanMux/src/instance
        config
    SOURCES
        components/NwDriver/nic_driver_common.c
        components/NwDriver/nic_driver_2.c
    C_FLAGS
        -Wall -Werror
    LIBS
        seos_core_api
        seos_libs
        seos_nw_driver_tap_proxy_lib
)

DeclareCAmkESComponent(NwApp_1
    INCLUDES
        components/test_APP/include
    SOURCES
        components/test_APP/test_NwApp/src/Client_App.c
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
        components/test_APP/include
    SOURCES
        components/test_APP/test_NwApp2/src/Server_App.c
        ${SEOS_LIBS_DIR}/seos_nwstack/network-client-lib/seos_nw_api_interface.c
    C_FLAGS
        -Wall -Werror
        -DSERVER_CONFIG=1
    LIBS
        seos_core_api
        seos_libs
)

DeclareCAmkESComponent(ChanMux
    SOURCES
        components/ChanMux/src/instance/ChanMux_Instance.c
    C_FLAGS
        -Wall -Werror
    LIBS
        seos_core_api
        seos_libs
)

DeclareCAmkESComponent(TimerClient
    SOURCES
        components/TimerClient/src/Timer_Client.c
    C_FLAGS
        -Wall -Werror
    LIBS
        seos_core_api
        seos_libs
)

DeclareCAmkESComponent(Timer
    SOURCES
        components/Timer/src/timer.c
    C_FLAGS
        -Wall -Werror
)

DeclareCAmkESComponent(UartDrv
    SOURCES
        components/Uart/src/UartDrv.c
        components/Uart/src/qemu_uart.c
    C_FLAGS
        -Wall -Werror
)

DeclareCAmkESRootserver(main.camkes)
GenerateCAmkESRootServer()
GenerateSimulateScript()
