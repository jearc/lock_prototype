#ifndef _RWTTAS_H_
#define _RWTTAS_H_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>
#include "utils.h"
#include "atomic_ops.h"

#define MAX_DELAY 1000

// layout of counter
//  31    ...   16 15    ...     0        
// +------------------------------+
// | reader count | writer count  |
// +------------------------------+
  
#define RC_INCR 0x10000 // to adjust reader count
#define WC_INCR 0x1     // to adjust writer count
#define W_MASK  0xffff  // to extract writer count
        
// mask bit for top of each count
#define WC_TOPMSK 0x8000      
#define RC_TOPMSK 0x80000000 

typedef struct rw_fair_data {
    volatile uint32_t requests;
    volatile uint32_t completions;
} rw_fair_data;

typedef struct rw_fair {
    union {
        rw_fair_data rw;
        volatile uint64_t lock_data;
#ifdef ADD_PADDING
        uint8_t padding[CACHE_LINE_SIZE];
#endif
    };
} rw_fair;

rw_fair* 
init_rw_fair_array_global(uint32_t num_locks);

uint32_t* 
init_rw_fair_array_local(uint32_t thread_num, uint32_t size);

void 
end_rw_fair_array_local(uint32_t* limits);

void 
end_rw_fair_array_global(rw_fair* the_locks);

int 
init_rw_fair_global(rw_fair* the_lock);

int 
init_rw_fair_local(uint32_t thread_num, uint32_t* limit);

void 
end_rw_fair_local();

void 
end_rw_fair_global();

void 
fair_read_release(rw_fair * lock);

void 
fair_write_acquire(rw_fair* lock, uint32_t * limit);

void 
fair_read_acquire(rw_fair* lock, uint32_t * limit);

int 
rw_fair_trylock(rw_fair* lock, uint32_t* limit);

void 
fair_write_release(rw_fair * lock);

int 
is_free_rw_fair(rw_fair* lock);

#endif
