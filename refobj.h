#ifndef _REFOBJ_H
#define _REFOBJ_H

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <assert.h>

#define COMPARE_AND_SWAP(PTR,OLD,NEW)\
    ({int __result;\
      do __result = __sync_val_compare_and_swap(PTR,OLD,NEW) == OLD?1:0;\
      while(0);\
      __result;})

#define ATOMIC_INCREASE(PTR) __sync_add_and_fetch(PTR,1)
#define ATOMIC_DECREASE(PTR) __sync_sub_and_fetch(PTR,1)

typedef  void (*refobj_destructor)(void*);

typedef struct refobj
{
        volatile uint32_t refcount;
        uint32_t               pad1;      
        union{
            struct{
                volatile uint32_t low32; 
                volatile uint32_t high32;       
            };
            volatile uint64_t identity;
        };
        void (*destructor)(void*);
} refobj;

typedef struct {
    union{  
        struct{ 
            uint64_t identity;    
            refobj   *ptr;
        };
        uint32_t _data[4];
    };
}ident;

int setup_sigsegv();

uint32_t refobj_dec(refobj *r);

void refobj_init(refobj *r,void (*destructor)(void*));

refobj *cast2refobj(ident _ident);

static inline uint32_t refobj_inc(refobj *r) {
    return ATOMIC_INCREASE(&r->refcount);
}

static inline ident make_ident(refobj *ptr) {
    return (ident){.identity=ptr->identity,.ptr=ptr};
}


#endif
