/*
 * File: ttas.h
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


#ifndef _TTAS_SIMP_H_
#define _TTAS_SIMP_H_

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
#include "atomic_ops.h"
#include "utils.h"


#define MIN_DELAY 100
#define MAX_DELAY 1000

typedef volatile uint32_t ttas_simp_index_t;
#ifdef __tile__
typedef uint32_t ttas_simp_lock_data_t;
#else
typedef uint8_t ttas_simp_lock_data_t;
#endif

typedef struct ttas_simp_lock_t {
    union {
        ttas_simp_lock_data_t lock;
#ifdef ADD_PADDING
        uint8_t padding[CACHE_LINE_SIZE];
#else
        uint8_t padding;
#endif
    };
}ttas_simp_lock_t;


/*
 *  Lock acquire and release methods
 */

void ttas_simp_lock(ttas_simp_lock_t* the_lock);

int ttas_simp_trylock(ttas_simp_lock_t* the_lock);

void ttas_simp_unlock(ttas_simp_lock_t* the_lock);

int is_free_ttas_simp(ttas_simp_lock_t * the_lock);
/*
   Some methods for easy lock array manipluation
   */

ttas_simp_lock_t* init_ttas_simp_array_global(uint32_t num_locks);


uint32_t* init_ttas_simp_array_local(uint32_t thread_num, uint32_t size);


void end_ttas_simp_array_local(uint32_t* limits);


void end_ttas_simp_array_global(ttas_simp_lock_t* the_locks);

/*
 *  Single lock initialization and destruction
 */

int init_ttas_simp_global(ttas_simp_lock_t* the_lock);

int  init_ttas_simp_local(uint32_t thread_num);

void end_ttas_simp_local();

void end_ttas_simp_global();


#endif


