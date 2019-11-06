#include "refobj.h"

#include <time.h>
#include <sys/time.h>
#include <setjmp.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include "time.h"


#define FENCE __sync_synchronize

static __thread sigjmp_buf *ptrJmpbuf = NULL;

#define TRY do{\
    sigjmp_buf jumpbuffer;\
    int savesigs= SIGSEGV;\
    ptrJmpbuf = &jumpbuffer;\
    if(sigsetjmp(jumpbuffer,savesigs) == 0)

#define CATCH else

#define ENDTRY\
    ptrJmpbuf = NULL;\
    }while(0);

static inline void signal_segv(int signum,siginfo_t* info, void*ptr){
    if(NULL != ptrJmpbuf) {
        siglongjmp(*ptrJmpbuf,signum);
    }
    return;
}

int setup_sigsegv(){
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_sigaction = signal_segv;
    action.sa_flags = SA_SIGINFO;
    if(sigaction(SIGSEGV, &action, NULL) < 0) {
        perror("sigaction");
        return 0;
    }
    return 1;
}

volatile uint32_t g_ref_counter = 0;

void refobj_init(refobj *r,void (*destructor)(void*)){
	r->destructor = destructor;
	r->high32 = chk_systick32();
	r->low32  = (uint32_t)(ATOMIC_INCREASE(&g_ref_counter));
	ATOMIC_INCREASE(&r->refcount);
}

uint32_t refobj_dec(refobj *r) {
    volatile uint32_t count;
    assert(r->refcount > 0);
    if((count = ATOMIC_DECREASE(&r->refcount)) == 0){
        r->identity = 0;//立即清0,使得cast2refobj可以快速查觉
        r->destructor(r);
    }
    return count;
}

refobj *cast2refobj(ident _ident) {
    refobj *ptr = NULL;
    if(unlikely(!_ident.ptr)) return NULL;
    /*
     *  try的作用，ptr指向的对象可能已经被free掉，对其访问将导致sigmentfault。
     *  如果出现这种情况说明ident是无效的，由CATCH捕获错误并返回NULL
     */
    TRY{
        refobj *o = (refobj*)_ident.ptr;
        /*
         *   假设refcount == 1
         *   A线程执行到COMPARE_AND_SWAP的前一条指令然后暂停,     
         *   B执行ATOMIC_DECREASE,refcount == 0,此时无论r->identity = 0是否执行。当A恢复执行后最终必将会发现oldCount == 0跳出for循环
         *
         */
        for(;_ident.identity == o->identity;) {
            uint32_t oldCount = o->refcount;
            uint32_t newCount = oldCount + 1;
            if(oldCount == 0) {
                break;
            }
            if(COMPARE_AND_SWAP(&o->refcount,oldCount,newCount)){
                ptr = o;
                break;
            }    
        }
    } CATCH {
        printf("segv\n");
        ptr = NULL;      
    }ENDTRY;
    return ptr; 
}    

