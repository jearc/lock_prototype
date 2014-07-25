#include "rw_scalable_fair.h"

__thread unsigned long * rw_scalable_fair_seeds;

int 
rw_scalable_fair_trylock(rw_scalable_fair* lock, rw_scalable_fair_qnode_t* I) 
{
    assert(0);
}

void 
scalable_fair_read_acquire(rw_scalable_fair* lock, rw_scalable_fair_qnode_t* I) 
{
    I->type = RW_SCALABLE_FAIR_QNODE_READ;
    I->next = NULL;
    I->blocked = 1;
    I->succ_type = RW_SCALABLE_FAIR_QNODE_NONE;

    rw_scalable_fair_qnode_t *pred = SWAP_PTR(&lock->rw.tail, I);

    if (pred == NULL) {
        FAI_U32(&lock->rw.reader_count);
        I->blocked = 0;
    } else {
        if (pred->type == RW_SCALABLE_FAIR_QNODE_WRIT || CAS_U32((uint32_t*)&pred->type, (uint32_t)RW_SCALABLE_FAIR_QNODE_NONE, (uint32_t)RW_SCALABLE_FAIR_QNODE_READ)) {
            pred->next = I;
            while (I->blocked);
        } else {
            FAI_U32(&lock->rw.reader_count);
            pred->next = I;
            I->blocked = 0;
        }
    }
}

void 
scalable_fair_read_release(rw_scalable_fair* lock, rw_scalable_fair_qnode_t* I) 
{
    rw_scalable_fair_qnode_t* w;
    if (I->next != NULL || CAS_PTR(&lock->rw.tail, I, NULL)) {
        while (I->next == NULL);
        if (I->succ_type == RW_SCALABLE_FAIR_QNODE_WRIT) {
            lock->rw.next_writer = I->next;
        }
    }
    if (FAD_U32(&lock->rw.reader_count) == 1 
            && (w = lock->rw.next_writer) != NULL
            && lock->rw.reader_count == 0
            && CAS_PTR(&lock->rw.next_writer, w, NULL)) {
        w->blocked = 0;
    }
}

void 
scalable_fair_write_acquire(rw_scalable_fair* lock, rw_scalable_fair_qnode_t* I) 
{
  /* MEM_BARRIER; */

    I->type = RW_SCALABLE_FAIR_QNODE_WRIT;
    I->next = NULL;
    I->blocked = 1;
    I->succ_type = RW_SCALABLE_FAIR_QNODE_NONE;
    rw_scalable_fair_qnode_t* pred; 

    MEM_BARRIER; /* ensure I is visible before swap */
    pred = SWAP_PTR(&lock->rw.tail, I);
    /* MEM_BARRIER; */

    if (pred == NULL) {
      /* MEM_BARRIER; */
        lock->rw.next_writer = I;
        if (lock->rw.reader_count == 0 
                && (SWAP_PTR(&lock->rw.next_writer, 0)) == I) {
          MEM_BARRIER;
          I->blocked = 0;
        }
    } else {
        pred->succ_type = RW_SCALABLE_FAIR_QNODE_WRIT;
        MEM_BARRIER;
        pred->next = I;
    }

    while (I->blocked);

    MEM_BARRIER; /* make sure memory does not leak around the lock */
}

void 
scalable_fair_write_release(rw_scalable_fair* lock, rw_scalable_fair_qnode_t* I)
{
    MEM_BARRIER;

    if (I->next != NULL || !CASB_PTR(&lock->rw.tail, I, NULL)) {
        while (I->next == NULL);
        if (I->next->type == RW_SCALABLE_FAIR_QNODE_READ) {
            FAI_U32(&lock->rw.reader_count);
        }
        MEM_BARRIER;
        I->next->blocked = 0;
    }

}

int 
is_free_rw_scalable_fair(rw_scalable_fair* lock)
{
    return lock->rw.reader_count == 0 && lock->rw.next_writer == NULL;
}

/*
 *  Some methods for easy lock array manipulation
 */
rw_scalable_fair* 
init_rw_scalable_fair_array_global(uint32_t num_locks) 
{
    rw_scalable_fair* the_locks;
    the_locks = (rw_scalable_fair*) malloc (num_locks * sizeof(rw_scalable_fair));
    uint32_t i;
    for (i = 0; i < num_locks; i++) {
        the_locks[i].rw.tail = NULL;
        the_locks[i].rw.next_writer = NULL;
        the_locks[i].rw.reader_count = 0;
    }
    MEM_BARRIER;
    return the_locks;
}

rw_scalable_fair_qnode_t*
init_rw_scalable_fair_array_local(uint32_t thread_num, uint32_t size)
{
    set_cpu(thread_num);
    rw_scalable_fair_qnode_t* I;
    I = (rw_scalable_fair_qnode_t*)malloc(size * sizeof(rw_scalable_fair_qnode_t));
    uint32_t i;
    for (i = 0; i < size; i++) {
        I[i].type = 0;
        I[i].succ_type = 0;
        I[i].blocked = 0;
        I[i].next = NULL;
    }
    MEM_BARRIER;
    return I;
}

void
end_rw_scalable_fair_array_local(rw_scalable_fair_qnode_t* I) 
{
    free(I);
}

void
end_rw_scalable_fair_array_global(rw_scalable_fair* the_locks) 
{
    free(the_locks);
}

int 
init_rw_scalable_fair_global(rw_scalable_fair* the_lock) 
{
    //the_lock->lock_data
    MEM_BARRIER;
    return 0;
}

int 
init_rw_scalable_fair_local(uint32_t thread_num, rw_scalable_fair_qnode_t* I)
{
    set_cpu(thread_num);
    I->type = 0;
    I->succ_type = RW_SCALABLE_FAIR_QNODE_NONE;
    I->blocked = 0;
    I->next = NULL;
    MEM_BARRIER;
    return 0;
}

void 
end_rw_scalable_fair_local(rw_scalable_fair_qnode_t* I) 
{
    //method not needed
}

void 
end_rw_scalable_fair_global() 
{
    //method not needed
}

