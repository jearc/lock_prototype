#include "rw_bounded.h"
#include <stdbool.h>

int rw_bounded_trylock(rw_bounded_global_params *G, rw_bounded_qnode_ptr I) {
//     rw_bounded_lock *L = G->the_lock;
    
//     I->next=NULL;
//     MEM_BARRIER;
//     if (CAS_PTR( L, NULL, I)==NULL) return 0;
// #ifdef __arm__
//     MEM_BARRIER;
// #endif
    return 1;
}

void rw_bounded_write_acquire(rw_bounded_global_params *G, rw_bounded_qnode_ptr I)
{
    I->next = NULL;
    MEM_BARRIER;
    rw_bounded_qnode_ptr pred = (rw_bounded_qnode*) SWAP_PTR( G->the_lock, (void *)I);

    if (pred != NULL) {
        I->waiting = 1; // word on which to spin
        MEM_BARRIER;
        pred->next = I; // make pred point to me
        while (I->waiting != 0) 
        {
            /* PAUSE */;
        }
    }
    I->observed_turn = G->turn = !G->turn;
    MEM_BARRIER;
    while (G->n_waiting_readers[!I->observed_turn] != 0);
}

void rw_bounded_write_release(rw_bounded_global_params *G, rw_bounded_qnode_ptr I) 
{
    G->completed_turn = I->observed_turn;

    rw_bounded_qnode_ptr succ;
    if (!(succ = I->next) && CAS_PTR(G->the_lock, I, NULL) != I) /* I seem to have no succ. */
    {
        do {
            succ = I->next;
            PAUSE;
        } while (!succ); // wait for successor
    }
    if (succ != NULL) {
        succ->waiting = 0;
    }
}

void rw_bounded_read_acquire(rw_bounded_global_params *G, rw_bounded_qnode_ptr I)
{
    FAI_U32(&G->n_waiting_readers[0]);
    FAI_U32(&G->n_waiting_readers[1]);
    I->observed_turn = G->turn;
    FAD_U32(&G->n_waiting_readers[!I->observed_turn]);

    while (G->completed_turn != I->observed_turn);
}

void rw_bounded_read_release(rw_bounded_global_params *G, rw_bounded_qnode_ptr I) 
{
    FAD_U32(&G->n_waiting_readers[I->observed_turn]);
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
    uint32_t i;
#ifdef ADD_PADDING
    printf("sizeof(rw_bounded_global_params) == %u\n", sizeof(rw_bounded_global_params));
    assert(sizeof(rw_bounded_global_params) == CACHE_LINE_SIZE);
#endif    
    rw_bounded_global_params *the_locks = calloc(num_locks, sizeof *the_locks);
    for (i=0;i<num_locks;i++) {
        the_locks[i].the_lock = calloc(1, sizeof *the_locks[i].the_lock);
    }
    MEM_BARRIER;
    return the_locks;
}


rw_bounded_qnode** init_rw_bounded_array_local(uint32_t thread_num, uint32_t num_locks) {
    set_cpu(thread_num);

    //init its qnodes
    uint32_t i;
#ifdef ADD_PADDING
    assert(sizeof(rw_bounded_qnode) == CACHE_LINE_SIZE);
#endif 
    rw_bounded_qnode** the_qnodes = calloc(num_locks, sizeof *the_qnodes);
    for (i=0;i<num_locks;i++) {
        the_qnodes[i] = calloc(1, sizeof *the_qnodes[i]);
	    assert(((uint32_t)the_qnodes[i] % CACHE_LINE_SIZE) != (CACHE_LINE_SIZE - 4));
    }
    MEM_BARRIER;
    return the_qnodes;

}

void end_rw_bounded_array_local(rw_bounded_qnode** the_qnodes, uint32_t size) {
    uint32_t i;
    for (i = 0; i < size; i++) {
        free(the_qnodes[i]);
    }
    free(the_qnodes);
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
    the_lock->the_lock = calloc(1, sizeof *the_lock->the_lock);
    MEM_BARRIER;
    return 0;
}


int init_rw_bounded_local(uint32_t thread_num, rw_bounded_qnode** the_qnode) {
    set_cpu(thread_num);

    *the_qnode = calloc(1, sizeof **the_qnode);

    MEM_BARRIER;
    return 0;

}

void end_rw_bounded_local(rw_bounded_qnode* the_qnodes) {
    free(the_qnodes);
}

void end_rw_bounded_global(rw_bounded_global_params the_locks) {
    free(the_locks.the_lock);
}

