/*
 * File: ttas.c
 * Author: Tudor David <tudor.david@epfl.ch>, Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *
 * Description: 
 *      Implementation of a test-and-test-and-set lock with back-off
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013 Tudor David, Vasileios Trigonakis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "ttas_simp.h"

#define UNLOCKED 0
#define LOCKED 1

__thread unsigned long * ttas_simp_seeds;

int ttas_simp_trylock(ttas_simp_lock_t * the_lock) {
    if (TAS_U8(&(the_lock->lock))==0) return 0;
#ifdef __arm__
    MEM_BARRIER;
#endif
    return 1;
}

void ttas_simp_lock(ttas_simp_lock_t * the_lock) {
#if defined(OPTERON_OPTIMIZE)
    volatile ttas_simp_lock_data_t* l = &(the_lock->lock);
    while (1){
        PREFETCHW(l);
        while ((*l)==1) {
            PREFETCHW(l);
        }
        if (TAS_U8(&(the_lock->lock))==UNLOCKED) {
            return;
        }
    }

#else  /* !OPTERON_OPTIMIZE */
    volatile ttas_simp_lock_data_t* l = &(the_lock->lock);
    while (1){
        while ((*l)==1) {}
        if (TAS_U8(l)==UNLOCKED) {
#ifdef __arm__
            MEM_BARRIER;
#endif
            return;
        }
    }
#endif	/* OPTERON_OPTIMIZE */
}


int is_free_ttas_simp(ttas_simp_lock_t * the_lock){
    if (the_lock->lock==UNLOCKED) return 1;
    return 0;
}

void ttas_simp_unlock(ttas_simp_lock_t *the_lock) 
{
#if defined(__tile__) || defined(__arm__)
    MEM_BARRIER;
#endif
    COMPILER_BARRIER;
    the_lock->lock=0;
}


/*
   Some methods for easy lock array manipulation
   */


//ttas_simp
ttas_simp_lock_t* init_ttas_simp_array_global(uint32_t num_locks) {

    ttas_simp_lock_t* the_locks;
#ifdef ADD_PADDING 
    assert(sizeof(ttas_simp_lock_t) == CACHE_LINE_SIZE);
#endif
    the_locks = (ttas_simp_lock_t*)malloc(num_locks * sizeof(ttas_simp_lock_t));
    uint32_t i;
    for (i = 0; i < num_locks; i++) {
        the_locks[i].lock=0;
    }
    MEM_BARRIER;
    return the_locks;
}



void end_ttas_simp_array_global(ttas_simp_lock_t* the_locks) {
    free(the_locks);
}

int init_ttas_simp_global(ttas_simp_lock_t* the_lock) {
    the_lock->lock=0;
    MEM_BARRIER;
    return 0;
}

void end_ttas_simp_global() {
    //function not needed
}

