#include "rw_fair.h"

__thread unsigned long * rw_fair_seeds;

int 
rw_fair_trylock(rw_fair* lock, uint32_t* limit) 
{
    assert(0);
}

void 
fair_read_acquire(rw_fair* lock, uint32_t* limit) 
{
    uint32_t prevRequests, newRequests; 
    do {
        prevRequests = lock->rw.requests;  
        newRequests = prevRequests & (~RC_TOPMSK); // clear top of reader count
        newRequests += RC_INCR;
    } while (CAS_U32(&lock->rw.requests, prevRequests, newRequests) != prevRequests);

    while ((lock->rw.completions & W_MASK) != (prevRequests & W_MASK));
}

void 
fair_read_release(rw_fair* lock) 
{
    uint32_t prevCompletions, newCompletions;
    do {
        prevCompletions = lock->rw.completions;
        newCompletions = prevCompletions & (~RC_TOPMSK); // clear top of reader count
        newCompletions += RC_INCR;
    } while (CAS_U32(&lock->rw.completions, prevCompletions, newCompletions) != prevCompletions);
}

void 
fair_write_acquire(rw_fair* lock, uint32_t* limit) 
{
    uint32_t prevRequests, newRequests;

    do {
        prevRequests = lock->rw.requests;
        newRequests = prevRequests & (~WC_TOPMSK); // clear top of writer count
        newRequests += WC_INCR;
    } while (CAS_U32(&lock->rw.requests, prevRequests, newRequests) != prevRequests);

    while (lock->rw.completions != prevRequests);
}

void 
fair_write_release(rw_fair* lock) 
{
    uint32_t prevCompletions, newCompletions;

    do {
        prevCompletions = lock->rw.completions;
        newCompletions = prevCompletions & (~WC_TOPMSK); // clear top of writer count
        newCompletions += WC_INCR;
    } while (CAS_U32(&lock->rw.completions, prevCompletions, newCompletions) != prevCompletions);
}

int 
is_free_rw_fair(rw_fair* lock)
{
    return lock->rw.completions == lock->rw.requests;
}

/*
 *  Some methods for easy lock array manipulation
 */
rw_fair* 
init_rw_fair_array_global(uint32_t num_locks) 
{
    rw_fair* the_locks;
    the_locks = (rw_fair*) malloc (num_locks * sizeof(rw_fair));
    uint32_t i;
    for (i = 0; i < num_locks; i++) {
        the_locks[i].lock_data = 0;
    }
    MEM_BARRIER;
    return the_locks;
}

uint32_t*
init_rw_fair_array_local(uint32_t thread_num, uint32_t size)
{
    set_cpu(thread_num);
    rw_fair_seeds = seed_rand();
    uint32_t* limits;
    limits = (uint32_t*)malloc(size * sizeof(uint32_t));
    uint32_t i;
    for (i = 0; i < size; i++) {
        limits[i]=1; 
    }
    MEM_BARRIER;
    return limits;
}

void
end_rw_fair_array_local(uint32_t* limits) 
{
    free(limits);
}

void
end_rw_fair_array_global(rw_fair* the_locks) 
{
    free(the_locks);
}

int 
init_rw_fair_global(rw_fair* the_lock) 
{
    the_lock->lock_data=0;
    MEM_BARRIER;
    return 0;
}

int 
init_rw_fair_local(uint32_t thread_num, uint32_t * limit)
{
    set_cpu(thread_num);
    *limit = 1;
    rw_fair_seeds = seed_rand();
    MEM_BARRIER;
    return 0;
}

void 
end_rw_fair_local() 
{
    //method not needed
}

void 
end_rw_fair_global() 
{
    //method not needed
}

