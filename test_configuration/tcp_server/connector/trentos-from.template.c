/*#
 *#Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *#
 *#SPDX-License-Identifier: BSD-2-Clause
  #*/

#define COMPONENT_NOTIFICATION 0
#define SOCKET_NOTIFICATION    1

#define SOCKET_NONE_EVENT    0
#define SOCKET_CONNECT_EVENT 1
#define SOCKET_READ_EVENT    2
#define SOCKET_WRITE_EVENT   3
#define SOCKET_CLOSE_EVENT   4
#define SOCKET_ERROR_EVENT   5

/*- set client_ids = namespace() -*/
/*- if me.parent.type.to_threads == 0 -*/
    /*- include 'seL4RPCNoThreads-from.template.c' -*/
    /*- from 'global-endpoint.template.c' import allocate_rpc_endpoint_cap with context -*/
    /*- do allocate_rpc_endpoint_cap(me.parent.to_end, is_reader=False, other_end=me) -*/
    /*- set client_ids.badges = pop('badges') -*/

/*- else -*/
    /*- include 'seL4RPCCall-from.template.c' -*/
    /*- from 'rpc-connector.c' import allocate_badges with context -*/
    /*- do allocate_badges(client_ids) -*/
/*- endif -*/

/*- set suffix = "_buf" -*/

#include <camkes/dataport.h>
#include <utils/util.h>

/*? macros.show_includes(me.instance.type.includes) ?*/

/*# Assign client ids and badges #*/
/*- set badges = namespace() -*/
/*- if client_ids is not undefined -*/
    /*- set badges.badges = client_ids.badges -*/
/*- else -*/
    /*- from 'rpc-connector.c' import allocate_badges with context -*/
    /*- do allocate_badges(badges) -*/
/*- endif -*/
/*- set client_id = badges.badges[me.parent.from_ends.index(me)] -*/

/*- if suffix is not defined -*/
  /*- set suffix = '' -*/
/*- endif -*/

/*- set socket_quota = configuration[me.instance.name].get("%s_socket_quota" % me.interface.name, 0) -*/

/*- set shmem_size = configuration[me.instance.name].get("%s_shmem_size" % me.interface.name, 4096) -*/
/*- set shmem_section = 'from_%s' % me.interface.name -*/
/*- set shmem_symbol = 'from_%s_data' % me.interface.name -*/
/*- set shmem_name = '%s%s' % (me.interface.name, suffix) -*/
/*- set page_size = macros.get_page_size(shmem_size, options.architecture) -*/
/*- if page_size == 0 -*/
  /*? raise(TemplateError('Setting %s.%s_shmem_size does not meet minimum size and alignment requirements. %d must be at least %d and %d aligned' % (me.instance.name, me.interface.name, size, 4096, 4096))) ?*/
/*- endif -*/

/*? macros.shared_buffer_symbol(sym=shmem_symbol, shmem_size=shmem_size, page_size=page_size) ?*/
/*? register_shared_variable('%s_%s_data' % (me.parent.name, client_id), shmem_symbol, shmem_size, frame_size=page_size) ?*/

volatile void * /*? shmem_name ?*/ = (volatile void *) & /*? shmem_symbol ?*/;

size_t /*? me.interface.name ?*/_get_size(void) {
    return ROUND_UP_UNSAFE(/*? shmem_size ?*/, PAGE_SIZE_4K);
}

size_t /*? me.interface.name ?*/_socket_quota(void) {
    return /*? socket_quota ?*/;
}

int /*? me.interface.name ?*/_wrap_ptr(dataport_ptr_t *p, void *ptr) {
    return -1;
}

void * /*? me.interface.name ?*/_unwrap_ptr(dataport_ptr_t *p) {
    return NULL;
}

/*- import 'helpers/error.c' as error with context -*/

#include <assert.h>
#include <camkes/error.h>
#include <camkes/tls.h>
#include <limits.h>
#include <sel4/sel4.h>
#include <stdbool.h>
#include <stddef.h>
#include <sync/bin_sem_bare.h>
#include <sync/sem-bare.h>
#include <utils/util.h>

/*? macros.show_includes(me.instance.type.includes) ?*/

/* Interface-specific error handling. */
/*- set error_handler = '%s_error_handler' % me.interface.name -*/

/*- set notification = alloc('notification_%d' % client_id, seL4_EndpointObject, read=True) -*/

/*- set handoff = alloc('handoff_%d' % client_id, seL4_EndpointObject, label=me.instance.name, read=True, write=True) -*/
static volatile int handoff_value;

/*- set lock = alloc('lock_%d' % client_id, seL4_NotificationObject, label=me.instance.name, read=True, write=True) -*/
static volatile int lock_count = 1;

/*- set handoffs = [] -*/
/*- for i in six.moves.range(socket_quota) -*/

/*- set hdof = alloc('handoff_%d_%d' % (client_id, i), seL4_EndpointObject, label=me.instance.name, read=True, write=True) -*/
/*- do handoffs.append((i, hdof)) -*/
static volatile int handoff_value_/*? client_id ?*/_/*? i ?*/;

/*- endfor -*/




static int lock(void) {
    int result = sync_bin_sem_bare_wait(/*? lock ?*/, &lock_count);
    __sync_synchronize();
    return result;
}

static int unlock(void) {
    __sync_synchronize();
    return sync_bin_sem_bare_post(/*? lock ?*/, &lock_count);
}

int /*? me.interface.name ?*/__run(void) {
    int notification_type = 0;
    int socket_number = 0;
    int socket_event = 0;

    while (true)
    {
        notification_type = 0;
        socket_number     = 0;
        socket_event      = 0;

        seL4_Recv(/*? notification ?*/, NULL);

        notification_type = seL4_GetMR(0);
        if(notification_type == SOCKET_NOTIFICATION)
        {
            socket_number     = seL4_GetMR(1);
            socket_event      = seL4_GetMR(2);
        }

        ZF_LOGE(
            "\n\nGot $%d$ %d, %d\n\n",
            notification_type,
            socket_number,
            socket_event);

        if (lock() != 0)
        {
            /* Failed to acquire the lock (`INT_MAX` threads in `register`?).
             * It's unsafe to go on at this point, so just drop the event.
             */
            continue;
        }

        /* Check that we're not about to overflow the handoff semaphore. If
            * we are, we just silently drop the event as allowed by the
            * semantics of this connector. Note that there is no race
            * condition here because we are the only thread incrementing the
            * semaphore.
            */

        /*- if len(handoffs) == 0 -*/
        goto end;
        /*- else -*/
        switch (socket_number)
        {
        /*- for i, hdf in handoffs -*/
        case /*? i ?*/:
            if (handoff_value_/*? client_id ?*/_/*? i ?*/ != INT_MAX)
            {
                sync_sem_bare_post(/*? hdf ?*/, &handoff_value_/*? client_id ?*/_/*? i ?*/);
            }
            goto end;
            /*- endfor -*/
        default:
            goto end;
        }
        /*- endif -*/

        if (handoff_value != INT_MAX) {
            sync_sem_bare_post(/*? handoff ?*/, &handoff_value);
        }
    int result UNUSED;
end:
    result = unlock();
    assert(result == 0);
}
}

int /*? me.interface.name ?*/_poll(void) {
    return sync_sem_bare_trywait(/*? handoff ?*/, &handoff_value) == 0;
}

void /*? me.interface.name ?*/_wait(void) {
#ifndef CONFIG_KERNEL_MCS
    camkes_protect_reply_cap();
#endif
    if (unlikely(sync_sem_bare_wait(/*? handoff ?*/, &handoff_value) != 0)) {
        ERR(/*? error_handler ?*/, ((camkes_error_t){
                .type = CE_OVERFLOW,
                .instance = "/*? me.instance.name ?*/",
                .interface = "/*? me.interface.name ?*/",
                .description = "failed to wait on event due to potential overflow",
            }), ({
                return;
            }));
    }
}

int /*? me.interface.name ?*/_socket_poll(unsigned int socket) {
    switch (socket)
    {
        /*- for i, hdf in handoffs -*/
        case /*? i ?*/:
             return sync_sem_bare_trywait(/*? hdf ?*/, &handoff_value_/*? client_id ?*/_/*? i ?*/) == 0;
        /*- endfor -*/
        default:
            return 0;
    }
}

void /*? me.interface.name ?*/_socket_wait(unsigned int socket) {
#ifndef CONFIG_KERNEL_MCS
    camkes_protect_reply_cap();
#endif
    switch (socket)
    {
        /*- for i, hdf in handoffs -*/
        case /*? i ?*/:
            if (unlikely(sync_sem_bare_wait(/*? hdf ?*/, &handoff_value_/*? client_id ?*/_/*? i ?*/) != 0)) {
                ERR(/*? error_handler ?*/, ((camkes_error_t){
                        .type = CE_OVERFLOW,
                        .instance = "/*? me.instance.name ?*/",
                        .interface = "/*? me.interface.name ?*/",
                        .description = "failed to wait on event due to potential overflow",
                    }), ({
                        return;
                    }));
            }
            return;
        /*- endfor -*/
        default:
            return;
    }
}
