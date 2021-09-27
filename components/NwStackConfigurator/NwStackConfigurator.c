/*
 * NwStackConfigurator implementation
 *
 * Copyright (C) 2021, HENSOLDT Cyber GmbH
 */

#include "OS_Error.h"
#include "OS_Network.h"
#include "if_NetworkStack_PicoTcp_Config.h"

#include "lib_debug/Debug.h"
#include <camkes.h>

//------------------------------------------------------------------------------
static const if_NetworkStack_PicoTcp_Config_t networkStackConfig =
    if_NetworkStack_PicoTcp_Config_ASSIGN(networkStack_PicoTcp_Config_rpc);


//------------------------------------------------------------------------------
void
post_init(void)
{
    static const OS_NetworkStack_AddressConfig_t ipAddrConfig =
    {
        .dev_addr       = NWSTACK_DEV_ADDR,
        .gateway_addr   = NWSTACK_GATEWAY_ADDR,
        .subnet_mask    = NWSTACK_SUBNET_MASK
    };

    OS_Error_t err = networkStackConfig.configIpAddr(&ipAddrConfig);
    Debug_ASSERT(err == OS_SUCCESS);
}
