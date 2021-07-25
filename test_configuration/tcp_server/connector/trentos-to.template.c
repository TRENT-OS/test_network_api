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
    /*- include 'seL4RPCNoThreads-to.template.c' -*/
    /*- from 'global-endpoint.template.c' import allocate_rpc_endpoint_cap with context -*/
    /*- do allocate_rpc_endpoint_cap(me.parent.to_end, is_reader=True, other_end=me.parent.from_ends[0]) -*/
    /*- set client_ids.badges = pop('badges') -*/

/*- else -*/
    /*- include 'seL4RPCCall-to.template.c' -*/
    /*- from 'rpc-connector.c' import allocate_badges with context -*/
    /*- do allocate_badges(client_ids) -*/
/*- endif -*/
#include <camkes/dataport.h>
#include <sel4/sel4.h>
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
/*- set badges = badges.badges -*/

/*# Enumerate all the incoming interfaces #*/
/*- set shmems = [] -*/
/*- set client_ids = set() -*/
/*- for c in me.parent.from_ends -*/

    /*- set client_id = badges[loop.index0] -*/

    /*- if client_id not in client_ids -*/
        /*- do client_ids.add(client_id) -*/

        /*- set socket_quota = configuration[c.instance.name].get("%s_socket_quota" % c.interface.name, 88) -*/
        /*- set shmem_size = configuration[c.instance.name].get("%s_shmem_size" % c.interface.name, 4096) -*/
        /*- set shmem_section = 'to_%s_%s' % (me.interface.name, client_id) -*/
        /*- set shmem_symbol = 'to_%s_%s_data' % (me.interface.name, client_id) -*/
        /*- set shmem_name = "%s_buf_%s" % (me.interface.name, client_id) -*/
        /*- set page_size = macros.get_page_size(shmem_size, options.architecture) -*/
        /*- if page_size == 0 -*/
          /*? raise(TemplateError('Setting %s.%s_shmem_size does not meet minimum size and alignment requirements. %d must be at least %d and %d aligned' % (c.instance.name, c.interface.name, size, 4096, 4096))) ?*/
        /*- endif -*/
        /*- set page_size_bits = int(math.log(page_size, 2)) -*/

        /*- set notification = alloc('notification_%d' % client_id, seL4_EndpointObject, write=True) -*/

        /*- set socket_notifications = [] -*/

        /*- for i in six.moves.range(socket_quota) -*/
            /*- set socket_notification = alloc('notification_%d_%d' % (client_id, i), seL4_NotificationObject, write=True) -*/
            /*- do socket_notifications.append((i, socket_notification)) -*/
        /*- endfor -*/

        /*? macros.shared_buffer_symbol(sym=shmem_symbol, shmem_size=shmem_size, page_size=page_size) ?*/
        /*? register_shared_variable('%s_%s_data' % (me.parent.name, client_id), shmem_symbol, shmem_size, frame_size=page_size) ?*/

        volatile void * /*? shmem_name ?*/ = (volatile void *) & /*? shmem_symbol ?*/;

        /*- do shmems.append((shmem_name, client_id, shmem_size, socket_quota, notification, socket_notifications)) -*/
    /*- else -*/
        /* skipping /*? client_id ?*/ */
    /*- endif -*/

/*- endfor -*/


void * /*? me.interface.name ?*/_buf(seL4_Word client_id) {
    /*- if len(shmems) == 0 -*/
        return NULL;
    /*- else -*/
        switch (client_id) {
            /*- for symbol, id, _, _, _, _ in shmems -*/
            case /*? id ?*/:
                return (void *) /*? symbol ?*/;
            /*- endfor -*/
        default:
            return NULL;
        }
    /*- endif -*/
}

size_t /*? me.interface.name ?*/_buf_size(seL4_Word client_id) {
    /*- if len(shmems) == 0 -*/
        return 0;
    /*- else -*/
        switch (client_id) {
            /*- for _, id, size, _, _, _ in shmems -*/
            case /*? id ?*/:
                return ROUND_UP_UNSAFE(/*? size ?*/, PAGE_SIZE_4K);
            /*- endfor -*/
        default:
            return 0;
        }
    /*- endif -*/
}

unsigned int /*? me.interface.name ?*/_num_badges(void) {
    return /*? len(shmems) ?*/;
}

seL4_Word /*? me.interface.name ?*/_enumerate_badge(unsigned int i) {
    /*- if len(shmems) == 0 -*/
       return -1;
    /*- else -*/
        switch (i) {
            /*- for _, id, _, _, _, _ in shmems -*/
                case /*? loop.index0 ?*/:
                    return /*? id ?*/;
            /*- endfor -*/
        default:
            return -1;
        }
    /*- endif -*/
}

unsigned int /*? me.interface.name ?*/_socket_quota(seL4_Word client_id) {
    /*- if len(shmems) == 0 -*/
       return -1;
    /*- else -*/
        switch (client_id) {
            /*- for _, id, _, socket_quota, _, _ in shmems -*/
            case /*? id ?*/:
                    return /*? socket_quota ?*/;
            /*- endfor -*/
        default:
            return -1;
        }
    /*- endif -*/
}



int /*? me.interface.name ?*/_wrap_ptr(dataport_ptr_t *p, void *ptr) {
    return -1;
}

void * /*? me.interface.name ?*/_unwrap_ptr(dataport_ptr_t *p) {
    return NULL;
}

#include <sel4/sel4.h>

/*? macros.show_includes(me.instance.type.includes) ?*/

void /*? me.interface.name ?*/_emit(seL4_Word client_id) {
    seL4_MessageInfo_t msgInfo = seL4_MessageInfo_new(0, 0, 0, 0);
    /*- if len(shmems) == 0 -*/
    return;
    /*- else -*/
    switch (client_id)
    {
    /*- for _, id, _, _, notification, _ in shmems -*/
    case /*? id ?*/:
        seL4_SetMR(0, COMPONENT_NOTIFICATION);
        msgInfo = seL4_MessageInfo_set_length(msgInfo, 1);
        seL4_Send(
            /*? notification ?*/,
            msgInfo);
        return;
        /*- endfor -*/
    default:
        return;
    }
    /*- endif -*/
}

void /*? me.interface.name ?*/_notify_socket(
    seL4_Word client_id,
    unsigned int socket)
{
    seL4_MessageInfo_t msgInfo = seL4_MessageInfo_new(0, 0, 0, 0);
    /*- if len(shmems) == 0 -*/
    return;
    /*- else -*/
    switch (client_id)
    {
    //    seL4_Send(seL4_CPtr dest, seL4_MessageInfo_t msgInfo)
    /*- for _, id, _, _, notification, _ in shmems -*/
        case /*? id ?*/:
            seL4_SetMR(0, SOCKET_NOTIFICATION);
            seL4_SetMR(1, socket);
            msgInfo = seL4_MessageInfo_set_length(msgInfo, 3);
            seL4_Send(
                /*? notification ?*/,
                msgInfo);
            return ;
    /*- endfor -*/
        default:
            return;
        }
    /*- endif -*/
}
