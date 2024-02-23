/*
 * Helper functions for the non-blocking scheme.
 *
 * Copyright (C) 2021-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

#pragma once

#include "OS_Socket.h"
#include "OS_Types.h"

//------------------------------------------------------------------------------
OS_Error_t
nb_helper_init(
    void (*event_notify_func_t)(void),
    void (*event_wait_func_t)(void),
    int (*mutex_lock_func_t)(void),
    int (*mutex_unlock_func_t)(void));

void
nb_helper_collect_pending_ev_handler(
    void* ctx);

OS_Error_t
nb_helper_wait_for_network_stack_init(
    const if_OS_Socket_t* const ctx);

OS_Error_t
nb_helper_wait_for_read_ev_on_socket(
    const OS_Socket_Handle_t handle);

OS_Error_t
nb_helper_wait_for_conn_est_ev_on_socket(
    const OS_Socket_Handle_t handle);

OS_Error_t
nb_helper_wait_for_conn_acpt_ev_on_socket(
    const OS_Socket_Handle_t handle);

OS_Error_t
nb_helper_reset_ev_struct_for_socket(
    const OS_Socket_Handle_t handle);
