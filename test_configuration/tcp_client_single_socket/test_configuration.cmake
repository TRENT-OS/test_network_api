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
    NwStack
    SOURCES
        components/NwStack/network_stack.c
        components/NwStack/util/helper_func.c
    C_FLAGS
        -Wall
        -Werror
        -DOS_NWSTACK_AS_CLIENT
        -DOS_CONFIG_SERVICE_CAMKES_CLIENT
        -DOS_NETWORK_MAXIMUM_SOCKET_NO=1
        -DDEV_ADDR="10.0.0.10"
        -DGATEWAY_ADDR="10.0.0.1"
        -DSUBNET_MASK="255.255.255.0"
        #-DOS_NETWORK_STACK_USE_CONFIGSERVER
    LIBS
        system_config
        os_core_api
        lib_compiler
        lib_debug
        os_configuration
        os_network_lib
        TimeServer_client
        syslogger_client
)

DeclareCAmkESComponent(
    TestAppTCPClient
    SOURCES
        components/TestAppTCPClient/TestAppTCPClient.c
    C_FLAGS
        -Wall
        -Werror
        -DOS_NETWORK_MAXIMUM_SOCKET_NO=1
        -DTCP_CLIENT
        -DDEV_ADDR="10.0.0.10"
        -DGATEWAY_ADDR="10.0.0.1"
        -DSUBNET_MASK="255.255.255.0"
    LIBS
        system_config
        os_core_api
        lib_compiler
        lib_debug
        lib_macros
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
