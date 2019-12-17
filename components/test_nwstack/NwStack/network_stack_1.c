/*
 *  SEOS Network Stack CAmkES wrapper
 *
 *  Copyright (C) 2019, Hensoldt Cyber GmbH
 *
 */

#include "Seos_Nw_Config.h"
#include "LibDebug/Debug.h"
#include "SeosError.h"
#include "seos_api_network_stack.h"
#include "seos_config_client.h"
#include <camkes.h>


static const seos_network_stack_config_t default_config =
{
    .dev_addr      = SEOS_TAP0_ADDR,
    .gateway_addr  = SEOS_TAP0_GATEWAY_ADDR,
    .subnet_mask   = SEOS_TAP0_SUBNET_MASK
};


// use network stack params configured in config server.
char DEV_ADDR[20];
char GATEWAY_ADDR[20];
char SUBNET_MASK[20];

static seos_network_stack_config_t param_config =
{
    .dev_addr      =   DEV_ADDR,
    .gateway_addr  =   GATEWAY_ADDR,
    .subnet_mask   =   SUBNET_MASK
};

//------------------------------------------------------------------------------
int run()
{
    bool set_default_param = false;
    Debug_LOG_INFO("driver up, starting network stack #1 (client)\n");

    // can't make this "static const" or even "static" because the data ports
    // are allocated at runtime
    seos_camkes_network_stack_config_t camkes_config =
    {
        .notify_init_done        = event_network_init_done_emit,
        .wait_loop_event         = event_tick_or_data_wait,

        .internal =
        {
            .notify_loop        = event_internal_emit,

            .notify_write       = e_write_emit,
            .wait_write         = c_write_wait,

            .notify_read        = e_read_emit,
            .wait_read          = c_read_wait,

            .notify_connection  = NULL,
            .wait_connection    = NULL,
        },

        .drv_nic =
        {
            .wait_init_done     = event_nic_init_done_wait,

            .from = // NIC -> stack
            {
                .buffer         = port_nic_from,
                .len            = PAGE_SIZE
            },

            .to = // stack -> NIC
            {
                .buffer         = port_nic_to,
                .len            = PAGE_SIZE
            },

            .rpc =
            {
                .dev_write      = nic_driver_tx_data,
                .get_mac        = nic_driver_get_mac,
            }
        },

        .app =
        {
            .notify_init_done   = event_network_init_done_emit,

            .port =
            {
                .buffer         = port_app_io,
                .len            = PAGE_SIZE
            },
        }
    };


    Debug_LOG_INFO(" Extract Network Params from the Config #1");

    // Wait to sync with the config server. This is how it is currently available in
    // config server
    while (!server_seos_configuration_IsUpAndRunning())
        ;

    // Create a handle to the remote library instance.
    seos_err_t ret;
    SeosConfigHandle serverLibWithMemBackend;

    ret = seos_configuration_createHandle(
                 SEOS_CONFIG_HANDLE_KIND_RPC,
                 0,
                 &serverLibWithMemBackend
             );
    if (ret != SEOS_SUCCESS)
    {
        set_default_param = true;
    }


    if(!set_default_param)
    {
        // Get the needed param values one by one from config server, using below API
        size_t bytesCopied;
        char* domain_name = "Domain-Network Stack";
        int error = SEOS_SUCCESS;

        seos_err_t ret = seos_configuration_parameterGetValueFromDomainName(
                                                    serverLibWithMemBackend,
                                                    domain_name,
                                                    "SEOS_TAP0_ADDR",
                                                    DEV_ADDR,
                                                    sizeof(DEV_ADDR),
                                                    &bytesCopied
                                                    );
        error |= ret;
        ret = seos_configuration_parameterGetValueFromDomainName(
                                                    serverLibWithMemBackend,
                                                    domain_name,
                                                    "SEOS_TAP0_GATEWAY_ADDR",
                                                    GATEWAY_ADDR,
                                                    sizeof(GATEWAY_ADDR),
                                                    &bytesCopied
                                                    );
        error |= ret;
        ret = seos_configuration_parameterGetValueFromDomainName(
                                                    serverLibWithMemBackend,
                                                    domain_name,
                                                    "SEOS_TAP0_SUBNET_MASK",
                                                    SUBNET_MASK,
                                                    sizeof(SUBNET_MASK),
                                                    &bytesCopied
                                                    );
        error |= ret;
        if(error != SEOS_SUCCESS)
        {
            set_default_param = true;
        }
    }


    if(set_default_param)
    {
        param_config = default_config;
        Debug_LOG_INFO("Using Default config for Stack #1 instance");
    }

    ret = seos_network_stack_run(&camkes_config, &param_config);

    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_FATAL("seos_network_stack_run() for #1 (client) failed, error %d",
                        ret);
        return -1;
    }

    Debug_LOG_WARNING("network stack #1 (client) terminated gracefully");

    return 0;
}
