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

NetworkStack_PicoTcp_DeclareCAmkESComponent(
    NetworkStack_PicoTcp
    C_FLAGS
        -DDEV_ADDR="10.0.0.11"
        -DGATEWAY_ADDR="10.0.0.1"
        -DSUBNET_MASK="255.255.255.0"
)

DeclareCAmkESComponent(
    TestAppTCPServer
    SOURCES
        components/TestAppTCPServer/TestAppTCPServer.c
    C_FLAGS
        -Wall
        -Werror
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
