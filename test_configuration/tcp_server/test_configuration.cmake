#
# Copyright (C) 2020-2024, HENSOLDT Cyber GmbH
# 
# SPDX-License-Identifier: GPL-2.0-or-later
#
# For commercial licensing, contact: info.cyber@hensoldt.net
#

TimeServer_DeclareCAmkESComponent(
    TimeServer
)

DeclareCAmkESComponent(
    NwStackConfigurator
    SOURCES
        components/NwStackConfigurator/NwStackConfigurator.c
    C_FLAGS
        -Wall
        -Werror
        -DNWSTACK_DEV_ADDR="10.0.0.11"
        -DNWSTACK_GATEWAY_ADDR="10.0.0.1"
        -DNWSTACK_SUBNET_MASK="255.255.255.0"
    LIBS
        system_config
        os_core_api
        lib_debug
        lib_compiler
        networkStack_PicoTcp_api
)

NetworkStack_PicoTcp_DeclareCAmkESComponent(
    NetworkStack_PicoTcp
)

DeclareCAmkESComponent(
    TestAppTCPServer
    SOURCES
        components/TestAppTCPServer/TestAppTCPServer.c
        util/non_blocking_helper.c
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
