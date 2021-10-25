TimeServer_DeclareCAmkESComponent(
    TimeServer
)

NetworkStack_PicoTcp_DeclareCAmkESComponent(
    NetworkStack_PicoTcp
    C_FLAGS
        -DNetworkStack_PicoTcp_USE_HARDCODED_IPADDR
        -DDEV_ADDR="10.0.0.11"
        -DGATEWAY_ADDR="10.0.0.1"
        -DSUBNET_MASK="255.255.255.0"
)

DeclareCAmkESComponent(
    TestAppMultiServer
    SOURCES
        components/TestAppMultiServer/TestAppMultiServer.c
    C_FLAGS
        -Wall
        -Werror
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
