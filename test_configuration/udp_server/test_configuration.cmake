TimeServer_DeclareCAmkESComponent(
    TimeServer
)

NetworkStack_PicoTcp_DeclareCAmkESComponent(
    NetworkStack_PicoTcp
    C_FLAGS
        -DNetworkStack_PicoTcp_USE_HARDCODED_IPADDR
        -DDEV_ADDR="10.0.0.10"
        -DGATEWAY_ADDR="10.0.0.1"
        -DSUBNET_MASK="255.255.255.0"
)

DeclareCAmkESComponent(
    TestAppUDPServer
    SOURCES
        components/TestAppUDPServer/TestAppUDPServer.c
        util/non_blocking_helper.c
    C_FLAGS
        -Wall
        -Werror
        -DOS_NETWORK_MAXIMUM_SOCKET_NO=1
        -DUDP_SERVER
        -DDEV_ADDR="10.0.0.10"
    LIBS
        system_config
        os_core_api
        lib_compiler
        lib_debug
        lib_macros
        os_socket_client
        syslogger_client
)

DeclareCAmkESComponent_SysLogger(
    SysLogger
    system_config
)
