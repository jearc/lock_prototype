#include "rw_bounded.h"
#include <stdbool.h>

int rw_bounded_trylock(rw_bounded_global_params *G, rw_bounded_local_params *L) {
//     rw_bounded_lock *L = G->the_lock;
    
//     I->next=NULL;
//     MEM_BARRIER;
//     if (CAS_PTR( L, NULL, I)==NULL) return 0;
// #ifdef __arm__
//     MEM_BARRIER;
// #endif
    return 1;
}

void rw_bounded_write_acquire(rw_bounded_global_params *G, rw_bounded_local_params *L)
{
    L->my_qnode->locked = 1;
    MEM_BARRIER;
    L->my_pred = SWAP_PTR(G->the_lock, L->my_qnode);
    while (L->my_pred->locked);

    G->turn = !G->turn;
    L->observed_turn = G->turn;
    MEM_BARRIER;
    while (G->n_waiting_readers[!L->observed_turn] != 0);
#if defined(__aarch64__)
    MEM_BARRIER;
#endif
}

void rw_bounded_write_release(rw_bounded_global_params *G, rw_bounded_local_params *L) 
{
#if defined(__aarch64__)
    MEM_BARRIER;
#endif

    G->completed_turn = L->observed_turn;

    L->my_qnode->locked = 0;
    L->my_qnode = L->my_pred;
}

void rw_bounded_read_acquire(rw_bounded_global_params *G, rw_bounded_local_params *L)
{
    FAI_U32(&G->n_waiting_readers[0]);
    FAI_U32(&G->n_waiting_readers[1]);
    // barrier should be required but __sync_fetch_and_add (used by FAI_U32) has implicit barrier functionality
    L->observed_turn = G->turn;
    FAD_U32(&G->n_waiting_readers[!L->observed_turn]);

    while (G->completed_turn != L->observed_turn);
}

void rw_bounded_read_release(rw_bounded_global_params *G, rw_bounded_local_params *L) 
{
    FAD_U32(&G->n_waiting_readers[L->observed_turn]);
}

int is_free_rw_bounded(rw_bounded_global_params *G){
    // rw_bounded_lock *L = G->the_lock;
    // if ((*L) == NULL) return 1;
    return 0;
}

/*
   Methods for easy lock array manipulation
   */

rw_bounded_global_params* init_rw_bounded_array_global(uint32_t num_locks) {
    rw_bounded_global_params *the_params = calloc(num_locks, sizeof *the_params);
    uint32_t i;
#ifdef ADD_PADDING
    printf("sizeof(rw_bounded_global_params) == %u\n", sizeof(rw_bounded_global_params));
    assert(sizeof(rw_bounded_global_params) == CACHE_LINE_SIZE);
#endif
    for (i=0;i<num_locks;i++) {
        the_params[i].the_lock = calloc(1, sizeof *the_params[i].the_lock);
        *(the_params[i].the_lock) = calloc(1, sizeof **(the_params[i].the_lock));
    }
    MEM_BARRIER;
    return the_params;
}


rw_bounded_local_params* init_rw_bounded_array_local(uint32_t thread_num, uint32_t num_locks) {
    set_cpu(thread_num);

    //init its qnodes
    uint32_t i;
    rw_bounded_local_params* local_params = calloc(num_locks, sizeof *local_params);
    for (i=0;i<num_locks;i++) {
        local_params[i].my_qnode = calloc(1, sizeof *local_params[i].my_qnode);
        local_params[i].my_pred = NULL;
    }
    MEM_BARRIER;
    return local_params;

}

void end_rw_bounded_array_local(rw_bounded_local_params *the_params, uint32_t size) {
    free(the_params);
}

void end_rw_bounded_array_global(rw_bounded_global_params* the_locks, uint32_t size) {
    uint32_t i;
    for (i = 0; i < size; i++) {
        free(the_locks[i].the_lock);
    }
    free(the_locks); 
}

int init_rw_bounded_global(rw_bounded_global_params* the_lock) {
    memset(the_lock, 0, sizeof *the_lock);
    the_lock->the_lock = calloc(1, sizeof *(the_lock->the_lock));    
    *(the_lock->the_lock) = calloc(1, sizeof **(the_lock->the_lock));
    MEM_BARRIER;
    return 0;
}


int init_rw_bounded_local(uint32_t thread_num, rw_bounded_local_params *local_params) {
    set_cpu(thread_num);

    local_params->my_qnode = (rw_bounded_qnode *) calloc(1, sizeof *local_params->my_qnode);
    local_params->my_pred = NULL;
    MEM_BARRIER;
    return 0;
}

void end_rw_bounded_local(rw_bounded_local_params the_params) {
    // free(the_params);
}

void end_rw_bounded_global(rw_bounded_global_params the_locks) {
    free(the_locks.the_lock);
}

