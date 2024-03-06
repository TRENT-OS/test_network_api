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

NetworkStack_PicoTcp_DeclareCAmkESComponent(
    NetworkStack_PicoTcp
    C_FLAGS
        -DNetworkStack_PicoTcp_USE_HARDCODED_IPADDR
        -DDEV_ADDR="${DEV_ADDR}"
        -DGATEWAY_ADDR="${GATEWAY_ADDR}"
        -DSUBNET_MASK="${SUBNET_MASK}"
)

DeclareCAmkESComponent(
    TestAppTCPClient
    SOURCES
        components/TestAppTCPClient/TestAppTCPClient.c
        util/non_blocking_helper.c
    C_FLAGS
        -Wall
        -Werror
        -DOS_NETWORK_MAXIMUM_SOCKET_NO=1
        -DTCP_CLIENT
        -DDEV_ADDR="${DEV_ADDR}"
        -DGATEWAY_ADDR="${GATEWAY_ADDR}"
        -DSUBNET_MASK="${SUBNET_MASK}"
        -DREACHABLE_HOST="${REACHABLE_HOST}"
        -DFORBIDDEN_HOST="${FORBIDDEN_HOST}"
        -DETH_ADDR_CLIENT_VALUE="${ETH_ADDR_CLIENT_VALUE}"
        -DETH_ADDR_SERVER_VALUE="${ETH_ADDR_SERVER_VALUE}"
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
