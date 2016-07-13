/*
common.h:
    Common types and global variables.

(c) TuxSH, 2016
This is part of 3ds_pxi, which is licensed under the MIT license (see LICENSE for details).
*/

#pragma once

#include <3ds.h>

/*
typedef struct
{
    const char *name;
    u8 id;
    u8 unknown;
    u8 maxSessions;
} ServiceInfo;
*/

/*
const ServiceInfo servicesInfo[10] =
{
    {"PxiFS0", 1, 2, 1},
    {"PxiFS1", 2, 2, 1},
    {"PxiFSB", 3, 2, 1},
    {"PxiFSR", 4, 2, 1},

    {"PxiPM", 5, 1, 1},

    {"pxi:am9", 7, 4, 1},
    {"pxi:dev", 6, 4, 1}, //in the official PXI module maxSessions == 2. It doesn't matter anyways, since srvSysRegisterService is always called with 1
    {"pxi:mc", 0, 0, 1},
    {"pxi:ps9", 8, 4, 1}
*/

typedef enum SessionState
{
    STATE_IDLE = 0,
    STATE_ARM11_COMMAND_RECEIVED = 1,
    STATE_ARM9_COMMAND_SENT = 2,
        STATE_ARM9_REPLY_RECEIVED = 3,

} SessionState;

typedef struct SessionData
{
    SessionState state;
    u32 buffer[0x100/4];

    Handle handle;
    u32 usedStaticBuffers;

    RecursiveLock lock; //why is this needed?

} SessionData;

#define NB_STATIC_BUFFERS 21

typedef struct SessionManager
{
    Handle sendAllBuffersToArm9Event, replySemaphore;
    u32 latest_PXI_MC5_val, pendingArm9Commands;
    u32 receivedServiceId;
    RecursiveLock senderLock;
    bool sendingDisabled;
    SessionData sessionData[10]; //9 actual services + 1 deleted service

    u32 currentlyProvidedStaticBuffers, freeStaticBuffers;
} SessionManager;

extern u32 __attribute__((aligned(0x1000))) staticBuffers[NB_STATIC_BUFFERS][0x1000/4];

extern Handle PXISyncInterrupt, PXITransferMutex;
extern Handle terminationRequestedEvent;
extern bool shouldTerminate;
extern SessionManager sessionManager;

extern const u32 nbStaticBuffersByService[10];

static inline Result assertSuccess(Result res)
{
    if(R_FAILED(res)) svcBreak(USERBREAK_PANIC);
    return res;
}

static inline u32 __clz(u32 val)
{
    u32 res;
    __asm__ volatile("clz %[res], %[val]" : [res] "=r" (res) : [val] "r" (val));
    return res;
}

static inline s32 getMSBPosition(u32 val)
{
    return 31 - (s32) __clz(val);
}

static inline u32 clearMSBs(u32 val, u32 nb)
{
    for(u32 i = 0; i < nb; i++)
    {
        s32 pos = getMSBPosition(val);
        if(pos == -1) break;
        val &= ~(1 << pos);
    }

    return val;
}

static inline u32 countNbBitsSet(u32 val)
{
    u32 nb = 0;
    while(val != 0)
    {
        val = clearMSBs(val, 1);
        nb++;
    }

    return nb;
}
