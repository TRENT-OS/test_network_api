/*
 *  WAN/LAN/NetworkStack Channel MUX
 *
 *  Copyright (C) 2019, Hensoldt Cyber GmbH
 */


#include "ChanMux/ChanMux.h"
#include "seos_system_config.h"
#include "SeosError.h"
#include "assert.h"
#include <camkes.h>

#define NO_CHANMUX_FIFO         { .buffer = NULL, .len = 0 }
#define NO_CHANMUX_DATA_PORT    { .io = NULL, .len = 0 }


static uint8_t nwFifoBuf[PAGE_SIZE];
static uint8_t nwCtrFifoBuf[128];

static uint8_t nwFifoBuf_2[PAGE_SIZE];
static uint8_t nwCtrFifoBuf_2[128];

static const ChanMuxConfig_t cfgChanMux =
{
    .numChannels = CHANMUX_NUM_CHANNELS,
    .outputDataport = {
        .io  = (void**) &outputDataPort,
        .len = PAGE_SIZE
    },
    .channelsFifos = {
        NO_CHANMUX_FIFO,
        NO_CHANMUX_FIFO,
        NO_CHANMUX_FIFO,
        NO_CHANMUX_FIFO,
        { .buffer = nwCtrFifoBuf,   .len = sizeof(nwCtrFifoBuf) },
        { .buffer = nwFifoBuf,      .len = sizeof(nwFifoBuf) },
        NO_CHANMUX_FIFO,
        { .buffer = nwCtrFifoBuf_2, .len = sizeof(nwCtrFifoBuf_2) },
        { .buffer = nwFifoBuf_2,    .len = sizeof(nwFifoBuf_2) }
    }
};


typedef struct {
    ChannelDataport_t  read;
    ChannelDataport_t  write;
} dataport_rw_t;

#define CHANMUX_DATA_PORT( _pBuf_, _len_ )     { .io = _pBuf_, .len = _len_ }

#define CHANMUX_DATA_PORT_RW_SHARED(_pBuf_, _len_) \
            { \
                .read = CHANMUX_DATA_PORT(_pBuf_, _len_), \
                .write = CHANMUX_DATA_PORT(_pBuf_, _len_) \
            }

#define NO_CHANMUX_DATA_PORT_RW     CHANMUX_DATA_PORT_RW_SHARED(NULL, 0)


static const dataport_rw_t dataports[] =
{
    NO_CHANMUX_DATA_PORT_RW,
    NO_CHANMUX_DATA_PORT_RW,
    NO_CHANMUX_DATA_PORT_RW,
    NO_CHANMUX_DATA_PORT_RW,
    CHANMUX_DATA_PORT_RW_SHARED( (void**)&port_nic_1_ctrl, PAGE_SIZE ),
    {
        .read  = CHANMUX_DATA_PORT( (void**)&port_nic_1_data_read,  PAGE_SIZE ),
        .write = CHANMUX_DATA_PORT( (void**)&port_nic_1_data_write, PAGE_SIZE )
    },
    NO_CHANMUX_DATA_PORT_RW,
    CHANMUX_DATA_PORT_RW_SHARED( (void**)&port_nic_2_ctrl, PAGE_SIZE ),
    {
        .read  = CHANMUX_DATA_PORT( (void**) &port_nic_2_data_read,  PAGE_SIZE ),
        .write = CHANMUX_DATA_PORT( (void**) &port_nic_2_data_write, PAGE_SIZE )
    }
};


//------------------------------------------------------------------------------
static void
ChanMux_dataAvailable_emit(
    unsigned int chanNum)
{
    switch (chanNum)
    {
    //---------------------------------
    case CHANMUX_CHANNEL_NIC_0_CTRL:
    case CHANMUX_CHANNEL_NIC_0_DATA:
        event_nic_1_hasData_emit();
        break;

    //---------------------------------
    case CHANMUX_CHANNEL_NIC_1_CTRL:
    case CHANMUX_CHANNEL_NIC_1_DATA:
        event_nic_2_hasData_emit();
        break;


    //---------------------------------
    default:
        Debug_LOG_ERROR("[channel %u] invalid channel to signal data available",
                        chanNum);

        break;
    }
}


//------------------------------------------------------------------------------
static ChanMux*
ChanMux_getInstanceOrCreate(
    unsigned int doCreate)
{
    // ToDo: actually, we need a mutex here to ensure all access and especially
    //       the creation is serialized. In the current implementation, the
    //       creation happens from the main thread before the interfaces are
    //       up and afterward we just try to get the instance, but never try to
    //       create it.

    // singleton
    static ChanMux* self = NULL;
    static ChanMux  theOne;
    static Channel_t channels[CHANMUX_NUM_CHANNELS];

    if ((NULL == self) && doCreate)
    {
        if (!ChanMux_ctor(&theOne,
                          channels,
                          &cfgChanMux,
                          NULL,
                          ChanMux_dataAvailable_emit,
                          Output_write))
        {
            Debug_LOG_ERROR("ChanMux_ctor() failed");
            return NULL;
        }

        self = &theOne;
    }

    return self;
}


//------------------------------------------------------------------------------
static ChanMux*
ChanMux_getInstance(void)
{
    return ChanMux_getInstanceOrCreate(0);
}


//==============================================================================
// CAmkES component
//==============================================================================

//---------------------------------------------------------------------------
// called before any other init function is called. Full runtime support is not
// available, e.g. interfaces cannot be expected to be accessible.
void pre_init(void)
{
    Debug_LOG_DEBUG("create ChanMUX instance");
    ChanMux* chanMux = ChanMux_getInstanceOrCreate(1);
    if (NULL == chanMux)
    {
        Debug_LOG_ERROR("ChanMUX instance creation failed");
    }
}


//==============================================================================
// CAmkES Interface "ChanMuxOut" (ChanMUX bottom)
//==============================================================================

//------------------------------------------------------------------------------
// function takeByte() of interface
void
ChanMuxOut_takeByte(char byte)
{
    ChanMux* chanMux = ChanMux_getInstance();
    if (NULL == chanMux)
    {
        Debug_LOG_ERROR("ChanMUX instance not available");
        return;
    }

    // process the byte. May trigger the notifications defined in cfgChanMux if
    // there is data in the channel or the state of the channel changed    
    ChanMux_takeByte(chanMux, byte);
}


//==============================================================================
// CAmkES Interface "ChanMux_driver" (ChanMUX top)
//==============================================================================

//------------------------------------------------------------------------------
// function write() of interface
seos_err_t
ChanMux_driver_write(
    unsigned int  chanNum,
    size_t        len,
    size_t*       lenWritten)
{
    Debug_LOG_TRACE("[Channel %u] write len %u", chanNum, len);

    // set defaults
    *lenWritten = 0;

    ChanMux* chanMux = ChanMux_getInstance();
    if (NULL == chanMux)
    {
        Debug_LOG_ERROR("[Channel %u] ChanMUX instance not available", chanNum);
        return SEOS_ERROR_GENERIC;
    }

    const ChannelDataport_t* dp = NULL;
    switch (chanNum)
    {
    //---------------------------------
    case CHANMUX_CHANNEL_NIC_0_CTRL:
    case CHANMUX_CHANNEL_NIC_0_DATA:
    case CHANMUX_CHANNEL_NIC_1_CTRL:
    case CHANMUX_CHANNEL_NIC_1_DATA:
        dp = &dataports[chanNum].write;
        break;
    //---------------------------------
    default:
        Debug_LOG_ERROR("[Channel %u] invalid channel for writing", chanNum);
        return SEOS_ERROR_ACCESS_DENIED;
    }

    Debug_ASSERT( NULL != dp );
    seos_err_t ret = ChanMux_write(chanMux, chanNum, dp, &len);
    *lenWritten = len;

    Debug_LOG_TRACE("[Channel %u] lenWritten %u", chanNum, len);

    return ret;
}


//------------------------------------------------------------------------------
// function write() of interface
seos_err_t
ChanMux_driver_read(
    unsigned int  chanNum,
    size_t        len,
    size_t*       lenRead)
{
    Debug_LOG_TRACE("[Channel %u] read len %u", __func__, chanNum, len);

    // set defaults
    *lenRead = 0;

    ChanMux* chanMux = ChanMux_getInstance();
    if (NULL == chanMux)
    {
        Debug_LOG_ERROR("[Channel %u] ChanMUX instance not available", chanNum);
        return SEOS_ERROR_GENERIC;
    }

    const ChannelDataport_t* dp = NULL;
    switch (chanNum)
    {
    //---------------------------------
    case CHANMUX_CHANNEL_NIC_0_CTRL:
    case CHANMUX_CHANNEL_NIC_0_DATA:
    case CHANMUX_CHANNEL_NIC_1_CTRL:
    case CHANMUX_CHANNEL_NIC_1_DATA:
        dp = &dataports[chanNum].read;
        break;
    //---------------------------------
    default:
        Debug_LOG_ERROR("[Channel %u] invalid channel for reading", chanNum);
        return SEOS_ERROR_ACCESS_DENIED;
    }

    Debug_ASSERT( NULL != dp );
    seos_err_t ret = ChanMux_read(chanMux, chanNum, dp, &len);
    *lenRead = len;

    Debug_LOG_TRACE("[Channel %u] lenRead %u", chanNum, len);

    return ret;
}
