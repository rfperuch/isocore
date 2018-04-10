//
// Copyright (c) 2018, Enrico Gregori, Alessandro Improta, Luca Sani, Institute
// of Informatics and Telematics of the Italian National Research Council
// (IIT-CNR). All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors
// may be used to endorse or promote products derived from this software without
// specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE IIT-CNR BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include <errno.h>
#include <isolario/branch.h>
#include <isolario/threading.h>
#include <pthread.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

enum {
    POOL_TRANSIENT_SECS = 2
};

typedef struct worker_s {
    pthread_t handle;
    pool_t *pool;
    bool transient;  ///< Thread is not in the permanent pool
    struct worker_s *next;
    struct worker_s *prev;
} worker_t;

typedef struct job_s {
    struct job_s *next;
    struct job_s *prev;
    max_align_t payload[];  // actual job data, as passed to pool_dispatch()
} job_t;

struct pool_s {
    pthread_mutex_t mutex;
    pthread_cond_t avail;

    int nthreads;
    void (*handle_job)(void *job);
    job_t jobs_head;
    job_t terminate;   // dummy value to terminate threads
    worker_t workers_head;
    worker_t perms[];  // permanent workers
};

static void *worker_routine(void *ptr)
{
    worker_t *self = ptr;
    pool_t *pool   = self->pool;
    while (true) {
        pthread_mutex_lock(&pool->mutex);

        int rc = 0;
        if (self->transient) {
            // transient workers die if no job is available within POOL_TRANSIENT_SECS
            struct timespec ts;

            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += POOL_TRANSIENT_SECS;
            while (pool->jobs_head.next == &pool->jobs_head && rc == 0)
                rc = pthread_cond_timedwait(&pool->avail, &pool->mutex, &ts);

        } else {
            while (pool->jobs_head.next == &pool->jobs_head)
                pthread_cond_wait(&pool->avail, &pool->mutex);
        }

        if (unlikely(pool->jobs_head.next == &pool->terminate))
            break;  // terminate request (priority over timeout)

        if (unlikely(rc != 0)) {
            // transient && timeout, detach and die
            pthread_detach(self->handle);

            self->prev->next = self->next;
            self->next->prev = self->prev;
            break;
        }

        job_t *job = pool->jobs_head.next;
        job->prev->next = job->next;
        job->next->prev = job->prev;

        pthread_mutex_unlock(&pool->mutex);

        // handle connection
        pool->handle_job(job->payload);
        free(job);
    }

    pthread_mutex_unlock(&pool->mutex);
    if (self->transient)
        free(self);  // transient threads manage their own memory

    return NULL;
}

pool_t *pool_create(int nthreads, void (*handle_job)(void *job))
{
    if (unlikely(nthreads < 0))
        nthreads = 0;

    pool_t *pool;

    size_t size = sizeof(*pool) + nthreads * sizeof(*pool->perms);
    pool = malloc(size);
    if (unlikely(!pool))
        return NULL;

    if (pthread_mutex_init(&pool->mutex, NULL) != 0) {
        free(pool);
        return NULL;
    }
    if (pthread_cond_init(&pool->avail, NULL) != 0) {
        free(pool);
        return NULL;
    }

    pool->nthreads          = 0;
    pool->handle_job        = handle_job;
    pool->jobs_head.next    = &pool->jobs_head;
    pool->jobs_head.prev    = &pool->jobs_head;
    pool->workers_head.next = &pool->workers_head;
    pool->workers_head.prev = &pool->workers_head;

    for (int i = 0; i < nthreads; i++) {
        worker_t *w = &pool->perms[i];

        w->pool      = pool;
        w->transient = false;
        if (pthread_create(&w->handle, NULL, worker_routine, w) != 0)
            break;

        w->next = &pool->workers_head;
        w->prev = pool->workers_head.prev;

        pool->workers_head.prev->next = w;
        pool->workers_head.prev = w;

        pool->nthreads++;
    }

    return pool;
}

int pool_nthreads(pool_t *pool)
{
    return pool->nthreads;
}

int pool_dispatch(pool_t *pool, const void *job, size_t size)
{
    job_t *task = malloc(sizeof(*task) + size);
    if (unlikely(!task))
        return -1;

    pthread_mutex_lock(&pool->mutex);

    task->next = &pool->jobs_head;
    task->prev = pool->jobs_head.prev;
    memcpy(task->payload, job, size);

    pool->jobs_head.prev->next = task;
    pool->jobs_head.prev = task;

    pthread_cond_signal(&pool->avail);
    pthread_mutex_unlock(&pool->mutex);
    return 0;
}

void pool_join(pool_t *pool)
{
    pthread_mutex_lock(&pool->mutex);

    pool->terminate.next = &pool->jobs_head;
    pool->terminate.prev = pool->jobs_head.prev;

    pool->jobs_head.prev->next = &pool->terminate;
    pool->jobs_head.prev       = &pool->terminate;

    pthread_cond_broadcast(&pool->avail);
    pthread_mutex_unlock(&pool->mutex);

    for (int i = 0; i < pool->nthreads; i++)
        pthread_join(pool->perms[i].handle, NULL);

    free(pool);
}

