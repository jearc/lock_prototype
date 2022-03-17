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
    I->ticket = G->ticket = !G->ticket;
    while (G->ticket_waiters[!I->ticket] != 0);
}

void rw_bounded_write_release(rw_bounded_global_params *G, rw_bounded_qnode_ptr I) 
{
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

    G->reader_lock = I->ticket;
}

void rw_bounded_read_acquire(rw_bounded_global_params *G, rw_bounded_qnode_ptr I)
{
    FAI_U32(&G->ticket_waiters[0]);
    FAI_U32(&G->ticket_waiters[1]);
    I->ticket = G->ticket;
    FAD_U32(&G->ticket_waiters[!I->ticket]);

    while (G->reader_lock != I->ticket);
}

void rw_bounded_read_release(rw_bounded_global_params *G, rw_bounded_qnode_ptr I) 
{
    FAD_U32(&G->ticket_waiters[I->ticket]);
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
    assert(sizeof(rw_bounded_global_params) == CACHE_LINE_SIZE);
#endif    
    rw_bounded_global_params* the_locks = (rw_bounded_global_params*)malloc(num_locks * sizeof(rw_bounded_global_params));
    for (i=0;i<num_locks;i++) {
        the_locks[i].the_lock=(rw_bounded_lock*)malloc(sizeof(rw_bounded_lock));
        *(the_locks[i].the_lock)=0;
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
    rw_bounded_qnode** the_qnodes = (rw_bounded_qnode**)malloc(num_locks * sizeof(rw_bounded_qnode*));
    for (i=0;i<num_locks;i++) {
        the_qnodes[i]=(rw_bounded_qnode*)malloc(sizeof(rw_bounded_qnode));
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
    the_lock->the_lock=(rw_bounded_lock*)malloc(sizeof(rw_bounded_lock));
    *(the_lock->the_lock)=0;
    MEM_BARRIER;
    return 0;
}


int init_rw_bounded_local(uint32_t thread_num, rw_bounded_qnode** the_qnode) {
    set_cpu(thread_num);

    (*the_qnode)=(rw_bounded_qnode*)calloc(1, sizeof(rw_bounded_qnode));

    MEM_BARRIER;
    return 0;

}

void end_rw_bounded_local(rw_bounded_qnode* the_qnodes) {
    free(the_qnodes);
}

void end_rw_bounded_global(rw_bounded_global_params the_locks) {
    free(the_locks.the_lock);
}

