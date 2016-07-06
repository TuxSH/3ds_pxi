/*
MyThread.c:
    Small threading library, based off ctrulib.

(c) TuxSH, 2016
This is part of 3ds_pxi, which is licensed under the MIT license (see LICENSE for details).
*/

#include "MyThread.h"
#include "memory.h"

#define THREADVARS_MAGIC  0x21545624 // !TV$

/*
extern const u8 __tdata_lma[];
extern const u8 __tdata_lma_end[];
extern u8 __tls_start[];
extern u8 __tls_end[];
*/

static void _thread_begin(void* arg)
{
    MyThread *t = (MyThread *)arg;
    //ThreadVars* tv = (ThreadVars *)getThreadLocalStorage();
    //tv->magic = THREADVARS_MAGIC;
    //tv->thread_ptr = t;
    t->ep();
    MyThread_Exit();
}

Result MyThread_Create(MyThread *t, void (*entrypoint)(void), void *stack, int prio, int affinity)
{
    /*u32 tlssize = __tls_end-__tls_start; //TODO: clean this up (I don't think it's actually needed in our case)
    u32 tlsloadsize = __tdata_lma_end-__tdata_lma;
    u32 tbsssize = tlssize-tlsloadsize;
*/
    t->ep       = entrypoint;
    t->stacktop = (u8 *)stack + THREAD_STACK_SIZE; //should be enough
/*
    if (tlsloadsize)
        memcpy(t->stacktop, __tdata_lma, tlsloadsize);
    if (tbsssize)
        for (u32 i = 0; i < tbsssize; i++) *((u8*)t->stacktop+tlsloadsize + i) = 0; //can't create a memset function due to a binutils bug*/

    return svcCreateThread(&t->handle, _thread_begin, (u32)t, (u32*)t->stacktop, prio, affinity);
}

Result MyThread_Join(MyThread *thread, s64 timeout_ns)
{
    if (thread == NULL) return 0;
    Result res = svcWaitSynchronization(thread->handle, timeout_ns);
    if(R_FAILED(res)) return res;

    svcCloseHandle(thread->handle);
    thread->handle = (Handle)0;

    return res;
}

void MyThread_Exit(void)
{
    svcExitThread();
}