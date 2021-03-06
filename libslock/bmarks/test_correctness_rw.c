/*
 * File: test_correct.c
 * Author: Tudor David <tudor.david@epfl.ch>
 *
 * Description:
 *      Test which exposes bugs in lock algorithms;
 *      By no means an exhaustive test, but generally exposes
 *      a buggy algorithm;
 *      Each thread continuously increments a global counter
 *      protected by a lock; if the final counter value is not
 *      equal to the sum of the increments by each thread, then
 *      the lock algorithm has a bug.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013 Tudor David
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

#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#if !defined(__sparc__) && !defined(__arm__) && !defined(HASWELL) && !defined(__aarch64__)
#include <numa.h>
#endif
#include "gl_lock.h"
#include "utils.h"
#include "lock_if.h"
#include "atomic_ops.h"

uint64_t c[2] = {0, 0};

#define XSTR(s) #s

#define PRINT_OUTPUT 1

//number of concurrent threads
#define DEFAULT_NUM_THREADS 1
//total duration of the test, in milliseconds
#define DEFAULT_DURATION 10000

static volatile int stop;

__thread unsigned long* seeds;
__thread uint32_t phys_id;
__thread uint32_t cluster_id;
lock_global_data the_lock;
__attribute__((aligned(CACHE_LINE_SIZE))) lock_local_data* local_th_data;

typedef struct barrier {
    pthread_cond_t complete;
    pthread_mutex_t mutex;
    int count;
    int crossing;
} barrier_t;

typedef struct shared_data{
    volatile uint64_t writer_counter[2];
    char padding[48];
    volatile struct {
        uint64_t counter;
        char padding[56];
    } reader_counter[8];
} shared_data;

typedef struct thread_data {
    union
    {
        struct
        {
            barrier_t *barrier;
            unsigned long num_reader_parallelism;
            unsigned long num_rw_mutex_violated;
            int id;
        };
        char padding[CACHE_LINE_SIZE];
    };
} thread_data_t;

__attribute__((aligned(CACHE_LINE_SIZE))) volatile shared_data* protected_data;
int duration;
int num_threads;

void barrier_init(barrier_t *b, int n)
{
    pthread_cond_init(&b->complete, NULL);
    pthread_mutex_init(&b->mutex, NULL);
    b->count = n;
    b->crossing = 0;
}

void barrier_cross(barrier_t *b)
{
    pthread_mutex_lock(&b->mutex);
    /* One more thread through */
    b->crossing++;
    /* If not all here, wait */
    if (b->crossing < b->count) {
        pthread_cond_wait(&b->complete, &b->mutex);
    } else {
        pthread_cond_broadcast(&b->complete);
        /* Reset for next time */
        b->crossing = 0;
    }
    pthread_mutex_unlock(&b->mutex);
}


void *reader_thread(void *data)
{
    thread_data_t *d = (thread_data_t *)data;
    phys_id = the_cores[d->id];
    cluster_id = get_cluster(phys_id);

    init_lock_local(phys_id, &the_lock, &(local_th_data[d->id]));

    barrier_cross(d->barrier);

    lock_local_data* local_d = &(local_th_data[d->id]);
    while (stop == 0) {
        unsigned long writer_counter[2] = {0};

        acquire_read(local_d, &the_lock);

        writer_counter[0] = protected_data->writer_counter[0];

        unsigned long reader_sum = 0;
        for (int i = 0; i < num_threads; i++) {
            if (i != d->id) {
                reader_sum += protected_data->reader_counter[i].counter;
            }
        }
        protected_data->reader_counter[d->id].counter++;
        for (int i = 0; i < num_threads; i++) {
            if (i != d->id) {
                reader_sum -= protected_data->reader_counter[i].counter;
            }
        }

        writer_counter[1] = protected_data->writer_counter[1];

        release_read(local_d, &the_lock);

        if (reader_sum != 0) {
            d->num_reader_parallelism++;
        }
        if (writer_counter[0] != writer_counter[1]) {
            d->num_rw_mutex_violated++;
        }
    }

    printf("reader %u not deadlocked\n", d->id);

    free_lock_local(local_th_data[d->id]);
    return NULL;
}

void *writer_thread(void *data)
{
    thread_data_t *d = (thread_data_t *)data;
    phys_id = the_cores[d->id];
    cluster_id = get_cluster(phys_id);

    init_lock_local(phys_id, &the_lock, &(local_th_data[d->id]));

    barrier_cross(d->barrier);

    lock_local_data* local_d = &(local_th_data[d->id]);
    while (stop == 0) {
        acquire_write(local_d, &the_lock);
        protected_data->writer_counter[0]++;
        protected_data->writer_counter[1]++;
        release_write(local_d, &the_lock);
    }

    printf("Writer %u not deadlocked\n", d->id);

    free_lock_local(local_th_data[d->id]);
    return NULL;
}

// void *test_correctness(void *data)
// {
//     thread_data_t *d = (thread_data_t *)data;
//     phys_id = the_cores[d->id];
//     cluster_id = get_cluster(phys_id);

//     init_lock_local(phys_id, &the_lock, &(local_th_data[d->id]));

//     barrier_cross(d->barrier);

//     lock_local_data* local_d = &(local_th_data[d->id]);
//     while (stop == 0) {
//         //printf("acquiring\n");
//         acquire_lock(local_d,&the_lock);
//         protected_data->counter++;
//         release_lock(local_d,&the_lock);
//         d->num_acquires++;
//     }

//     free_lock_local(local_th_data[d->id]);
//     return NULL;
// }


void catcher(int sig)
{
    static int nb = 0;
    printf("CAUGHT SIGNAL %d\n", sig);
    if (++nb >= 3)
        exit(1);
}


int main(int argc, char **argv)
{
    set_cpu(the_cores[0]);
    struct option long_options[] = {
        // These options don't set a flag
        {"help",                      no_argument,       NULL, 'h'},
        {"duration",                  required_argument, NULL, 'd'},
        {"num-threads",               required_argument, NULL, 'n'},
        {NULL, 0, NULL, 0}
    };

    int i, c;
    thread_data_t *data;
    pthread_t *threads;
    pthread_attr_t attr;
    barrier_t barrier;
    struct timeval start, end;
    struct timespec timeout;
    duration = DEFAULT_DURATION;
    num_threads = DEFAULT_NUM_THREADS;
    sigset_t block_set;

    while(1) {
        i = 0;
        c = getopt_long(argc, argv, "h:d:n:", long_options, &i);

        if(c == -1)
            break;

        if(c == 0 && long_options[i].flag == 0)
            c = long_options[i].val;

        switch(c) {
            case 0:
                /* Flag is automatically set */
                break;
            case 'h':
                printf("lock stress test\n"
                        "\n"
                        "Usage:\n"
                        "  stress_test [options...]\n"
                        "\n"
                        "Options:\n"
                        "  -h, --help\n"
                        "        Print this message\n"
                        "  -d, --duration <int>\n"
                        "        Test duration in milliseconds (0=infinite, default=" XSTR(DEFAULT_DURATION) ")\n"
                        "  -n, --num-threads <int>\n"
                        "        Number of threads (default=" XSTR(DEFAULT_NUM_THREADS) ")\n"
                      );
                exit(0);
            case 'd':
                duration = atoi(optarg);
                break;
            case 'n':
                num_threads = atoi(optarg);
                break;
            case '?':
                printf("Use -h or --help for help\n");
                exit(0);
            default:
                exit(1);
        }
    }
    assert(duration >= 0);
    assert(num_threads > 0);

    protected_data = (shared_data*) malloc(sizeof(shared_data));
    protected_data->writer_counter[0] = 0;
    protected_data->writer_counter[1] = 0;
    for (int i = 0; i < 8; i++) {
        protected_data->reader_counter[i].counter = 0;
    }
#ifdef PRINT_OUTPUT
    printf("Duration               : %d\n", duration);
    printf("Number of threads      : %d\n", num_threads);
#endif
    timeout.tv_sec = duration / 1000;
    timeout.tv_nsec = (duration % 1000) * 1000000;

    if ((data = (thread_data_t *)malloc(num_threads * sizeof(thread_data_t))) == NULL) {
        perror("malloc");
        exit(1);
    }
    if ((threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t))) == NULL) {
        perror("malloc");
        exit(1);
    }

    local_th_data = (lock_local_data *)malloc(num_threads*sizeof(lock_local_data));

    stop = 0;
    /* Init locks */
#ifdef PRINT_OUTPUT
    printf("Initializing locks\n");
#endif
    init_lock_global_nt(num_threads,&the_lock);

    /* Access set from all threads */
    barrier_init(&barrier, num_threads + 1);
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    for (i = 0; i < num_threads - 1; i++) {
#ifdef PRINT_OUTPUT
        printf("Creating reader thread %d\n", i);
#endif
        data[i].id = i;
        data[i].num_reader_parallelism = 0;
        data[i].num_rw_mutex_violated = 0;
        data[i].barrier = &barrier;
        if (pthread_create(&threads[i], &attr, reader_thread, (void *)(&data[i])) != 0) {
            fprintf(stderr, "Error creating thread\n");
            exit(1);
        }
    }
    for (i = num_threads - 1; i < num_threads; i++) {
#ifdef PRINT_OUTPUT
        printf("Creating writer thread %d\n", num_threads - 1 - i);
#endif
        data[i].id = i;
        data[i].num_reader_parallelism = 0;
        data[i].num_rw_mutex_violated = 0;
        data[i].barrier = &barrier;
        if (pthread_create(&threads[i], &attr, writer_thread, (void *)(&data[i])) != 0) {
            fprintf(stderr, "Error creating thread\n");
            exit(1);
        }
    }
    pthread_attr_destroy(&attr);

    /* Catch some signals */
    if (signal(SIGHUP, catcher) == SIG_ERR ||
            signal(SIGINT, catcher) == SIG_ERR ||
            signal(SIGTERM, catcher) == SIG_ERR) {
        perror("signal");
        exit(1);
    }

    /* Start threads */
    barrier_cross(&barrier);
#ifdef PRINT_OUTPUT
    printf("STARTING...\n");
#endif
    gettimeofday(&start, NULL);
    if (duration > 0) {
        nanosleep(&timeout, NULL);
    } else {
        sigemptyset(&block_set);
        sigsuspend(&block_set);
    }
    stop = 1;
    gettimeofday(&end, NULL);
#ifdef PRINT_OUTPUT
    printf("STOPPING...\n");
#endif
    /* Wait for thread completion */
    for (i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            fprintf(stderr, "Error waiting for thread completion\n");
            exit(1);
        }
    }

    duration = (end.tv_sec * 1000 + end.tv_usec / 1000) - (start.tv_sec * 1000 + start.tv_usec / 1000);

    uint64_t num_reader_parallelism = 0;
    uint64_t num_rw_mutex_violated = 0;
    for (i = 0; i < num_threads - 1; i++) {
#ifdef PRINT_OUTPUT
        printf("Thread %d\n", i);
        // printf("  #acquire   : %lu\n", data[i].num_acquires);
        printf("  #reader_parallelism   : %lu\n", data[i].num_reader_parallelism);
        printf("  #rw_mutex_violated    : %lu\n", data[i].num_rw_mutex_violated);
#endif
        num_reader_parallelism += data[i].num_reader_parallelism;
        num_rw_mutex_violated += data[i].num_rw_mutex_violated;
    }
#ifdef PRINT_OUTPUT
    printf("Duration      : %d (ms)\n", duration);
#endif
    // printf("Counter total : %llu, Expected: %llu\n", (unsigned long long) protected_data->counter, (unsigned long long) acquires);
    // if (protected_data->counter != acquires) {
    //     printf("Incorrect lock behavior!\n");
    // }
    printf("Reader parallelism total : %llu\n", (unsigned long long)num_reader_parallelism);
    printf("RW mutex violated total  : %llu\n", (unsigned long long)num_rw_mutex_violated);
    if (num_reader_parallelism) {
        printf("Reader parallelism observed\n");
    } else {
        printf("Reader parallelism not observed: incorrect RW lock behaviour!\n");
    }
    if (!num_rw_mutex_violated) {
        printf("Violation of reader-writer mutual exclusion NOT observed\n");
    } else {
        printf("Violation of reader-writer mutual exclusion observed: incorrect RW lock behaviour!\n");
    }

    /* Cleanup locks */
    free_lock_global(the_lock);

    free(threads);
    free(data);

    return 0;
}
