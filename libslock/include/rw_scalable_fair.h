#ifndef _RW_SC_FAIR_H_
#define _RW_SC_FAIR_H_

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

#define RW_SCALABLE_FAIR_QNODE_NONE 0
#define RW_SCALABLE_FAIR_QNODE_READ 1
#define RW_SCALABLE_FAIR_QNODE_WRIT 2

typedef struct rw_scalable_fair_qnode {
    volatile uint32_t type;
    volatile uint32_t blocked;
    volatile struct rw_scalable_fair_qnode *next;
    volatile uint32_t succ_type;
} rw_scalable_fair_qnode_t;

typedef struct rw_scalable_fair_data {
    volatile rw_scalable_fair_qnode_t *tail;
    volatile rw_scalable_fair_qnode_t *next_writer;
    volatile uint32_t reader_count;
} rw_scalable_fair_data;

typedef struct rw_scalable_fair {
    union {
        rw_scalable_fair_data rw;
        volatile uint32_t lock_data [3];
#ifdef ADD_PADDING
        uint8_t padding[CACHE_LINE_SIZE];
#endif
    };
} rw_scalable_fair;

rw_scalable_fair* 
init_rw_scalable_fair_array_global(uint32_t num_locks);

rw_scalable_fair_qnode_t* 
init_rw_scalable_fair_array_local(uint32_t thread_num, uint32_t size);

void 
end_rw_scalable_fair_array_local(rw_scalable_fair_qnode_t* I);

void 
end_rw_scalable_fair_array_global(rw_scalable_fair* the_locks);

int 
init_rw_scalable_fair_global(rw_scalable_fair* the_lock);

int 
init_rw_scalable_fair_local(uint32_t thread_num, rw_scalable_fair_qnode_t* I);

void 
end_rw_scalable_fair_local();

void 
end_rw_scalable_fair_global();

void 
scalable_fair_read_release(rw_scalable_fair* lock, rw_scalable_fair_qnode_t* I);

void 
scalable_fair_write_acquire(rw_scalable_fair* lock, rw_scalable_fair_qnode_t * I);

void 
scalable_fair_read_acquire(rw_scalable_fair* lock, rw_scalable_fair_qnode_t * I);

int 
rw_scalable_fair_trylock(rw_scalable_fair* lock, rw_scalable_fair_qnode_t* I);

void 
scalable_fair_write_release(rw_scalable_fair * lock, rw_scalable_fair_qnode_t* I);

int 
is_free_rw_scalable_fair(rw_scalable_fair* lock);

#endif

