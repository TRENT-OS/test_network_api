/*
 * TestAppTCPServer
 *
 * Copyright (C) 2019-2021, HENSOLDT Cyber GmbH
 *
 */
#include "OS_Error.h"
#include "lib_compiler/compiler.h"
#include "lib_debug/Debug.h"
#include "stdint.h"
#include <string.h>

#include "OS_Socket.h"
#include "interfaces/if_OS_Socket.h"
#include "util/loop_defines.h"
#include <camkes.h>

static const if_OS_Socket_t network_stack =
    IF_OS_SOCKET_ASSIGN(networkStack);

/*
 * This example demonstrates a server with an incoming connection. Reads
 *  incoming data after a connection is established. Writes or echoes the
 * received data back to the client. When the connection with the client is
 * closed a new round begins, in an infinitive loop. Currently only a single
 * socket is supported per stack instance. i.e. no multitasking is supported as
 * of now.
 */

//------------------------------------------------------------------------------
void
pre_init(void)
{
#if defined(Debug_Config_PRINT_TO_LOG_SERVER)
    DECL_UNUSED_VAR(OS_Error_t err) = SysLoggerClient_init(sysLogger_Rpc_log);
    Debug_ASSERT(err == OS_SUCCESS);
#endif
}

#define NUM_TCP_SRV 5
#define NUM_UDP_SRV 5
#define NUM_SRV (NUM_TCP_SRV + NUM_UDP_SRV)

#define MAX_NUM_CLIENTS 64

#define TCP_SRV_BACKLOG 5

#define TEST_CFG_TCP_PORT 11000
#define TEST_CFG_UDP_PORT 12000

#define TEST_CFG_ETH_ADDR "10.0.0.10"


typedef struct {
    OS_Socket_Handle_t handle;
    int used;
    int type;    // OS_SOCK_STREAM = TCP, OS_SOCK_DGRAM = UDP
    OS_Socket_Addr_t srcAddr;

    uint8_t    eventMask;
    OS_Error_t currentError;
} Srv_Struct_t;

typedef struct {
    OS_Socket_Handle_t handle;
    int used;
    int type;    // OS_SOCK_STREAM = TCP, OS_SOCK_DGRAM = UDP
    OS_Socket_Addr_t srcAddr;

    int        parentSocketHandle;
    uint8_t    eventMask;
    OS_Error_t currentError;

    char echoBuffer[4096];
    int echoBuffer_start;
    int echoBuffer_end;
} Client_Struct_t;


static Srv_Struct_t srvHandle[NUM_SRV];
static Client_Struct_t clientHandle[MAX_NUM_CLIENTS];


Client_Struct_t* getUnusedClientHandle(){

    for (int i = 0; i < MAX_NUM_CLIENTS; i++){
        if (!clientHandle[i].used){
            return &clientHandle[i];
        }
    }
    return NULL;
}

Client_Struct_t* getClientHandleById(int socketId){

    for (int i = 0; i < MAX_NUM_CLIENTS; i++){
        if ((clientHandle[i].used) && (clientHandle[i].handle.handleID == socketId)){
            return &clientHandle[i];
        }
    }
    return NULL;
}

Srv_Struct_t* getServerHandleById(int socketId){

    for (int i = 0; i < NUM_SRV; i++){
        if ((srvHandle[i].used) && (srvHandle[i].handle.handleID == socketId)){
            return &srvHandle[i];
        }
    }
    return NULL;
}

void initSrvStruct(){
    for (int i = 0; i < NUM_SRV; i++){
        srvHandle[i].used = false;
        srvHandle[i].handle.ctx = network_stack;
        srvHandle[i].handle.handleID = -1;
        srvHandle[i].type = 0;
        srvHandle[i].srcAddr.addr[0] = 0;
        srvHandle[i].srcAddr.port = 0;        
        srvHandle[i].eventMask = 0;
        srvHandle[i].currentError = 0;        
    }
    return;
}

void initClientStruct(){
    for (int i = 0; i < MAX_NUM_CLIENTS; i++){
        clientHandle[i].used = false;
        clientHandle[i].handle.ctx = network_stack;
        clientHandle[i].handle.handleID = -1;
        clientHandle[i].type = 0;
        clientHandle[i].srcAddr.addr[0] = 0;
        clientHandle[i].srcAddr.port = 0;
        clientHandle[i].parentSocketHandle = -1;
        clientHandle[i].eventMask = 0;
        clientHandle[i].currentError = 0;
        clientHandle[i].echoBuffer_start = 0;
        clientHandle[i].echoBuffer_end = -1;
    }
    return;
}


OS_Error_t sendRemainingData(Client_Struct_t* client){
    OS_Error_t err;    
    size_t actualLenWritten = 0;

    // we write back actualLenRecv bytes, at most 4096 bytes
    int loops = 0;
    // only try to write 3 times max
    while ((client->echoBuffer_start <= client->echoBuffer_end) && (loops < 3))
    {
        loops++;
        err = OS_Socket_write(
            client->handle,
            &client->echoBuffer[client->echoBuffer_start],
            client->echoBuffer_end - client->echoBuffer_start +1,
            &actualLenWritten);

        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_Socket_write() failed, socket: %d, error %d", client->handle.handleID, err);
            // TBD close socket ???
            return err;
        }
        client->echoBuffer_start += actualLenWritten;
    }
    return OS_SUCCESS;    
}


OS_Error_t echoTCP(Client_Struct_t* client){
    OS_Error_t err;    
    size_t actualLenRecv = 0;
    size_t actualLenWritten = 0;
    bool finished = false;    


    while (!finished)
    {
        err = OS_Socket_read(
                client->handle,
                client->echoBuffer,
                sizeof(client->echoBuffer),
                &actualLenRecv);
        if (err == OS_ERROR_TRY_AGAIN){
            // nothing to read, stop
            finished = true;
        } else if (OS_SUCCESS != err){
            Debug_LOG_ERROR("OS_Socket_read() failed, error %d", err);
            // TBD close socket ???
            return err;;
        } else {
            // success
            client->echoBuffer_start = 0;
            client->echoBuffer_end = actualLenRecv -1;
        }
    
    
        // we write back actualLenRecv bytes, at most 4096 bytes
        Debug_LOG_TRACE("OS_Socket_write() start");
        int loops = 0;
        // only try to write 3 times max
        while ((client->echoBuffer_start <= client->echoBuffer_end) && (loops < 3))
        {
            loops++;
            err = OS_Socket_write(
                client->handle,
                &client->echoBuffer[client->echoBuffer_start],
                client->echoBuffer_end - client->echoBuffer_start +1,
                &actualLenWritten);

            if (err != OS_SUCCESS)
            {
                Debug_LOG_ERROR("OS_Socket_write() failed, socket: %d, error %d", client->handle.handleID, err);            
                // TBD close socket ???
                return err;
            }
            client->echoBuffer_start += actualLenWritten;
        }

        if (client->echoBuffer_start <= client->echoBuffer_end)
        {
            // if unsent bytes in buffer, stop
            finished = true;
        }
        Debug_LOG_TRACE("OS_Socket_write() end %d", loops);
    }

    return OS_SUCCESS;    
}

OS_Error_t handleReadEventTCP(Client_Struct_t* client){
    OS_Error_t err;    

    // only read new data, if all previously received data has been returned
    if (client->echoBuffer_start <= client->echoBuffer_end){
        err = sendRemainingData(client);
    } else {
        err = echoTCP(client);
    }

    return err;
}

OS_Error_t handleReadEventUDP(Srv_Struct_t* server){
    OS_Error_t err;    
    static char buffer[4096];
    size_t actualLenRecv = 0;
    size_t actualLenWritten = 0;
    size_t sumLenWritten = 0;
    bool finished = false;

    Debug_LOG_INFO("handleReadEventUDP: started for handle: %d", server->handle.handleID);
    // Try to read some data.
    while (!finished)
    {
        err = OS_Socket_recvfrom(
                server->handle,
                buffer,
                sizeof(buffer),
                &actualLenRecv,
                &server->srcAddr);
        if (err == OS_ERROR_TRY_AGAIN){
            // nothing to read, continue
            finished = true;
        } else if (err != OS_SUCCESS){
            Debug_LOG_ERROR("OS_Socket_read() failed, error %d", err);
            // TBD close socket ???
            return err;
        } else {
            // success
            // we write back actualLenRecv bytes, at most 4096 bytes
            sumLenWritten = 0;
            int loops = 0;
            Debug_LOG_DEBUG("OS_Socket_sendto() start");
            while ((sumLenWritten < actualLenRecv) && (loops < 3))
            {
                loops++;
                err = OS_Socket_sendto(
                    server->handle,
                    &buffer[sumLenWritten],
                    actualLenRecv-sumLenWritten,
                    &actualLenWritten,
                    &server->srcAddr);

                if (err != OS_SUCCESS)
                {
                    Debug_LOG_ERROR("OS_Socket_sendto() failed, error %d", err);
                    // TBD close socket ???
                    return err;
                }
                sumLenWritten += actualLenWritten;
            }
            Debug_LOG_DEBUG("OS_Socket_sendto() end loops: %d, sent: %d, remaining: %d", loops, actualLenRecv, actualLenRecv - sumLenWritten);
        }
    }
    return OS_SUCCESS;
}


OS_Error_t closeClient (Client_Struct_t* client){
    if (client == NULL){
        Debug_LOG_ERROR("closeClient: invalid handle");
        return OS_ERROR_GENERIC;
    }
    
    OS_Socket_close(client->handle);
    
    client->used = false;
    client->handle.ctx = network_stack;
    client->handle.handleID = -1;
    client->type = 0;
    client->srcAddr.addr[0] = 0;
    client->srcAddr.port = 0;
    client->parentSocketHandle = -1;
    client->eventMask = 0;
    client->currentError = 0;
    client->echoBuffer_start = 0;
    client->echoBuffer_end = -1;

    return OS_SUCCESS;
}

OS_Error_t closeServer (Srv_Struct_t* server){

    if (server == NULL){
        Debug_LOG_ERROR("closeServer: invalid handle");
        return OS_ERROR_GENERIC;
    }
 
    OS_Socket_close(server->handle);
    
    server->used = false;
    server->handle.ctx = network_stack;
    server->handle.handleID = -1;
    server->type = 0;
    server->srcAddr.addr[0] = 0;
    server->srcAddr.port = 0;
    server->eventMask = 0;
    server->currentError = 0;

    return OS_SUCCESS;
}

//------------------------------------------------------------------------------
int
run()
{
    Debug_LOG_INFO("Starting TestAppMultiServer ...");
    
    OS_Error_t err;
  
    // Buffer big enough to hold 2 frames, rounded to the nearest power of 2
    static char evtBuffer[4096];
    size_t evtBufferSize = sizeof(evtBuffer);
    int numberOfSocketsWithEvents;

    initClientStruct();
    initSrvStruct();

    // create TCP Server sockets
    for (int i = 0; i < NUM_TCP_SRV; i++){
        do
        {
            seL4_Yield();
            err = OS_Socket_create(
                                &network_stack,
                                &srvHandle[i].handle,
                                OS_AF_INET,
                                OS_SOCK_STREAM);
        } while (err == OS_ERROR_NOT_INITIALIZED);

        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_Socket_create() failed, srvHandle[%d].handle, code %d", i, err);
            return -1;
        }

        srvHandle[i].type = OS_SOCK_STREAM;
        srvHandle[i].used = true;

        OS_Socket_Addr_t dstAddr =
        {
            .addr = OS_INADDR_ANY_STR,
            .port = TEST_CFG_TCP_PORT + i
        };

        err = OS_Socket_bind(
                srvHandle[i].handle,
                &dstAddr);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_Socket_bind() failed, srvHandle[%d].handle, code %d", i, err);
            return -1;
        }

        err = OS_Socket_listen(
                srvHandle[i].handle,
                TCP_SRV_BACKLOG);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_Socket_listen() failed, srvHandle[%d].handle, code %d", i, err);
            return -1;
        }
    }

    // create UDP Server sockets
    for (int i = 0; i < NUM_UDP_SRV; i++){
        err = OS_Socket_create(
                            &network_stack,
                            &srvHandle[NUM_TCP_SRV + i].handle,
                            OS_AF_INET,
                            OS_SOCK_DGRAM);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_Socket_create() failed, srvHandle[%d].handle, code %d", NUM_TCP_SRV + i, err);
            return -1;
        }
        
        srvHandle[NUM_TCP_SRV + i].type = OS_SOCK_DGRAM;
        srvHandle[NUM_TCP_SRV + i].used = true;        

        OS_Socket_Addr_t dstAddr =
        {
            .addr = OS_INADDR_ANY_STR,
            .port = TEST_CFG_UDP_PORT + i
        };

        err = OS_Socket_bind(srvHandle[NUM_TCP_SRV + i].handle, &dstAddr);
        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_Socket_bind() failed, srvHandle[%d].handle, code %d", NUM_TCP_SRV + i, err);
            return -1;
        }
      
    }

    Debug_LOG_INFO("launching multi echo server");


    for (;;)
    {
        // Wait until we get an event for the listening socket.
        networkStack_event_notify_wait();
        Debug_LOG_TRACE("Event Notification received");

        err = OS_Socket_getPendingEvents(
                &network_stack,
                evtBuffer,
                evtBufferSize,
                &numberOfSocketsWithEvents);

        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_Socket_getPendingEvents() failed, code %d", err);
            continue;
        }

        Debug_LOG_TRACE("OS_Socket_getPendingEvents: received %d events ", numberOfSocketsWithEvents);
        // Verify that the received number of sockets with events is within expected
        // bounds.

        int offset = 0;

        for (int i = 0; i < numberOfSocketsWithEvents; i++)
        {
            OS_Socket_Evt_t event;
            memcpy(&event, &evtBuffer[offset], sizeof(event));
            offset += sizeof(event);

            // => handle socket events
            Debug_LOG_TRACE("handle event - handle: %d, parentSocketHandle: %d, eventMask 0x%x, currentError %d",
                event.socketHandle, event.parentSocketHandle, event.eventMask, event.currentError);

            if ((event.socketHandle < 0) ||
                (event.socketHandle >= OS_NETWORK_MAXIMUM_SOCKET_NO))
            {
                Debug_LOG_ERROR("handle event - recieved invalid handle: %d", event.socketHandle);
                continue;
            }



            Client_Struct_t* client;
            Srv_Struct_t* server;
            client = getClientHandleById(event.socketHandle);
            server = getServerHandleById(event.socketHandle);


            // socket has been closed by Network stack
            if (event.eventMask & OS_SOCK_EV_FIN)
            {
                Debug_LOG_ERROR("OS_Socket_getPendingEvents: OS_SOCK_EV_FIN handle: %d", event.socketHandle);
                // socket has been closed by network stack - close socket
  
                if (client != NULL){
                    // close client
                    closeClient(client);
                }

                if (server != NULL){
                    // close server
                    closeServer(server);
                }
                continue;
            }

            // Connection event successful - not used in this application, only socket
            if (event.eventMask & OS_SOCK_EV_CONN_EST)
            {
                Debug_LOG_ERROR("OS_Socket_getPendingEvents: Unexpected event - OS_SOCK_EV_CONN_EST handle: %d", event.socketHandle);
            }

            // new client connection (TCP only)
            if (event.eventMask & OS_SOCK_EV_CONN_ACPT)
            {
                Client_Struct_t* acceptedHandle;

                // find empty client socket slot
                acceptedHandle = getUnusedClientHandle();
                if (acceptedHandle == NULL){
                    Debug_LOG_ERROR("OS_Socket_getPendingEvents: no free client handles to accept incoming connection handle: %d",
                        event.socketHandle);
                    continue;
                }

                server = getServerHandleById(event.socketHandle);
                if (server == NULL){
                    Debug_LOG_ERROR("OS_Socket_getPendingEvents: parentID not valid to accept incoming connection handle: %d, parentSocketHandle: %d, eventMask 0x%x, currentError %d",
                        event.socketHandle, event.parentSocketHandle, event.eventMask, event.currentError);
                    continue;
                }                

                // accept new connection
                err = OS_Socket_accept(
                                server->handle,
                                &(acceptedHandle->handle),
                                &(acceptedHandle->srcAddr));
                if (err != OS_SUCCESS)
                {
                    Debug_LOG_ERROR("OS_Socket_accept() failed, error %d", err);
                    
                } else {
                    acceptedHandle->used = true;
                    acceptedHandle->type = server->type;
                    acceptedHandle->parentSocketHandle = server->handle.handleID;
                }
            }

            // new data to read - only clients
            if (event.eventMask & OS_SOCK_EV_READ)
            {
                if ((client == NULL) && (server == NULL)){
                    Debug_LOG_ERROR("OS_Socket_getPendingEvents: OS_SOCK_EV_READ socketHandle not valid, handle: %d", event.socketHandle);
                    continue;
                } else if ((client != NULL) && (server != NULL)) {
                    Debug_LOG_ERROR("OS_Socket_getPendingEvents: OS_SOCK_EV_READ socketHandle ambigous, handle: %d", event.socketHandle);
                    continue;
                } else if ((client != NULL) && (server == NULL)) {
                    // Client handle, must be TCP connection
                    if (client->type == OS_SOCK_STREAM){
                        handleReadEventTCP(client);
                    } else {
                        Debug_LOG_ERROR("OS_Socket_getPendingEvents: TCP client expected, handle: %d", event.socketHandle);
                        continue;
                    }
                } else {
                    // Server handle, must be UDP connection
                    if (server->type == OS_SOCK_DGRAM){
                        handleReadEventUDP(server);
                    } else {
                        Debug_LOG_ERROR("OS_Socket_getPendingEvents: UDP server expected, handle: %d", event.socketHandle);
                        continue;
                    }
                }
            }

            // socket is available for sending - ignore for now
            if (event.eventMask & OS_SOCK_EV_WRITE)
            {
                // do nothing
            }

            
            // remote socket requested to be closed only valid for clients, 
            if (event.eventMask & OS_SOCK_EV_CLOSE)
            {
                if (client != NULL){
                    // write received data still in buffer
                    // issue is, that this is being performed only once the client (netcat)  closes the connection
                    // data will be send, but not received by client
                    // echo server needs to be changed, so that additionally to the network event notification, we run a periodic tidy-up task
                    sendRemainingData(client);
                    // close client
                    closeClient(client);
                }

                if (server != NULL){
                    // close server
                    closeServer(server);
                }
            }

            // Error received - print error
            if (event.eventMask & OS_SOCK_EV_ERROR)
            {
                Debug_LOG_ERROR("OS_Socket_getPendingEvents: OS_SOCK_EV_ERROR handle: %d, code: %d", event.socketHandle, event.currentError);
                // tbd what to do here.
            }

            // tidy up task, a little hacky, as it still depends on the network stack event notification
            for (int i = 0; i < MAX_NUM_CLIENTS; i++){
                if (clientHandle[i].used){
                    sendRemainingData(&clientHandle[i]);
                }
            }
        }
    }
    return 0;
}
