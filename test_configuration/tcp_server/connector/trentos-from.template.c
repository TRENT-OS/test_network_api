/*#
 *#Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *#
 *#SPDX-License-Identifier: BSD-2-Clause
  #*/


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

/*- set notification = alloc('notification_%d' % client_id, seL4_NotificationObject, read=True) -*/

/*- set handoff = alloc('handoff_%d' % client_id, seL4_EndpointObject, label=me.instance.name, read=True, write=True) -*/
static volatile int handoff_value;

/*- set lock = alloc('lock_%d' % client_id, seL4_NotificationObject, label=me.instance.name, read=True, write=True) -*/
static volatile int lock_count = 1;

static int lock(void) {
    int result = sync_bin_sem_bare_wait(/*? lock ?*/, &lock_count);
    __sync_synchronize();
    return result;
}

static int unlock(void) {
    __sync_synchronize();
    return sync_bin_sem_bare_post(/*? lock ?*/, &lock_count);
}

static void (*callback)(void*);
static void *callback_arg;

int /*? me.interface.name ?*/__run(void) {
    while (true) {
        seL4_Wait(/*? notification ?*/, NULL);

        if (lock() != 0) {
            /* Failed to acquire the lock (`INT_MAX` threads in `register`?).
             * It's unsafe to go on at this point, so just drop the event.
             */
            continue;
        }

        /* Read and deregister any callback. */
        void (*cb)(void*) = callback;
        callback = NULL;
        void *arg = callback_arg;

        if (cb == NULL) {
            /* No callback was registered. */

            /* Check that we're not about to overflow the handoff semaphore. If
             * we are, we just silently drop the event as allowed by the
             * semantics of this connector. Note that there is no race
             * condition here because we are the only thread incrementing the
             * semaphore.
             */
            if (handoff_value != INT_MAX) {
                sync_sem_bare_post(/*? handoff ?*/, &handoff_value);
            }
        }

        int result UNUSED = unlock();
        assert(result == 0);

        if (cb != NULL) {
            /* A callback was registered. Note that `cb` is a local variable
             * and thus we know that the semaphore post above was not executed.
             */
            cb(arg);
        }
    }
}

static int poll(void) {
    return sync_sem_bare_trywait(/*? handoff ?*/, &handoff_value) == 0;
}

int /*? me.interface.name ?*/_poll(void) {
    return poll();
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

int /*? me.interface.name ?*/_reg_callback(void (*cb)(void*), void *arg) {

    /* First see if there's a pending event, allowing us to immediately invoke
     * the callback without having to register it.
     */
    if (poll()) {
        cb(arg);
        return 0;
    }

    if (lock() != 0) {
        /* Failed to acquire the lock (`INT_MAX` threads in this function?). */
        return -1;
    }

    if (poll()) {
        /* An event arrived in between us checking previously and acquiring the
         * lock.
         */
        int result UNUSED = unlock();
        assert(result == 0);
        cb(arg);
        return 0;

    } else if (callback != NULL) {
        /* There is already a registered callback. */
        int result UNUSED = unlock();
        assert(result == 0);
        return -1;

    } else {
        /* Register the callback. Expected case. */
        callback = cb;
        callback_arg = arg;
        int result UNUSED = unlock();
        assert(result == 0);
        return 0;
    }
}
