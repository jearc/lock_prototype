#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#if !defined(__sparc__) && !defined(__arm__) && !defined(HASWELL) && !defined(__aarch64__)
#include <numa.h>
#endif
#include <pthread.h>
#include <stdbool.h>
#include "utils.h"
#include "atomic_ops.h"

typedef struct rw_bounded_qnode {
    volatile uint8_t locked;
    uint8_t padding[54];
} rw_bounded_qnode;

typedef volatile rw_bounded_qnode *rw_bounded_qnode_ptr;
typedef rw_bounded_qnode_ptr rw_bounded_lock; //initialized to NULL

typedef struct rw_bounded_local_params {
    rw_bounded_qnode *my_qnode;
    rw_bounded_qnode *my_pred;
    volatile bool observed_turn;
} rw_bounded_local_params;

typedef struct rw_bounded_global_params {
    rw_bounded_lock* the_lock;
    volatile uint32_t n_waiting_readers[2];
    volatile bool turn;
    volatile bool completed_turn;
    uint8_t padding[46];
} rw_bounded_global_params;


/*
   Methods for easy lock array manipulation
   */

rw_bounded_global_params* init_rw_bounded_array_global(uint32_t num_locks);

rw_bounded_local_params* init_rw_bounded_array_local(uint32_t thread_num, uint32_t num_locks);

void end_rw_bounded_array_local(rw_bounded_local_params *the_params, uint32_t size);

void end_rw_bounded_array_global(rw_bounded_global_params* the_locks, uint32_t size);
/*
   single lock manipulation
   */

int init_rw_bounded_global(rw_bounded_global_params* the_lock);

int init_rw_bounded_local(uint32_t thread_num, rw_bounded_local_params *local_params);

void end_rw_bounded_local(rw_bounded_local_params the_params);

void end_rw_bounded_global(rw_bounded_global_params the_locks);

/*
 *  Acquire and release methods
 */

void rw_bounded_write_acquire(rw_bounded_global_params *G, rw_bounded_local_params *L);

void rw_bounded_write_release(rw_bounded_global_params *G, rw_bounded_local_params *L);

void rw_bounded_read_acquire(rw_bounded_global_params *G, rw_bounded_local_params *L);

void rw_bounded_read_release(rw_bounded_global_params *G, rw_bounded_local_params *L);

int is_free_rw_bounded(rw_bounded_global_params *G);

int rw_bounded_trylock(rw_bounded_global_params *G, rw_bounded_local_params *L);
