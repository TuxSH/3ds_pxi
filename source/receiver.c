/*
receiver.c:
    Fetches replies coming from Process9, writing them in the appropriate buffer.

(c) TuxSH, 2016
This is part of 3ds_pxi, which is licensed under the MIT license (see LICENSE for details).
*/

#include "receiver.h"
#include "PXI.h"
#include "memory.h"

static inline void receiveArm9Reply(void)
{
    u32 serviceId = PXIReceiveWord();
    
    //The offcical implementation can return 0xD90043FA
    if(serviceId >= 10 || sessionManager.sessionData[serviceId].state != STATE_ARM9_COMMAND_SENT)
        svcBreak(USERBREAK_PANIC);

    sessionManager.receivedServiceId = serviceId;
    RecursiveLock_Lock(&(sessionManager.sessionData[serviceId].lock));
    u32 replyHeader = PXIReceiveWord();
    u32 replySizeWords = (replyHeader & 0x3F) + ((replyHeader & 0xFC0) >> 6) + 1;

    if(replySizeWords > 0x40) svcBreak(USERBREAK_PANIC);

    u32 *buf = sessionManager.sessionData[serviceId].buffer;

    buf[0] = replyHeader;
    PXIReceiveBuffer(buf + 1, replySizeWords - 1);
    sessionManager.sessionData[serviceId].state = STATE_ARM9_REPLY_RECEIVED;
    RecursiveLock_Unlock(&(sessionManager.sessionData[serviceId].lock));

    if(serviceId == 0 && shouldTerminate)
    {
        assertSuccess(svcSignalEvent(terminationRequestedEvent));
        return;
    }

    if(serviceId != 9)
    {
        s32 count;
        assertSuccess(svcReleaseSemaphore(&count, sessionManager.replySemaphore, 1));
    }
    else
    {
        assertSuccess(svcSignalEvent(sessionManager.PXISRV11CommandReceivedEvent));
    }
}

void receiver(void)
{
    Handle handles[] = {PXISyncInterrupt, terminationRequestedEvent};

    while(true)
    {
        s32 index;
        assertSuccess(svcWaitSynchronizationN(&index, handles, 2, false, -1LL));

        if(index == 1) return;
        while(!PXIIsReceiveFIFOEmpty())
            receiveArm9Reply();
    }
}