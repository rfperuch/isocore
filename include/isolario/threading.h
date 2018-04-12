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

/**
 * @file isolario/threading.h
 *
 * @brief Thread and pool utilities
 *
 * @note This file is guaranteed to include standard \a stddef.h,
 *       a number of other files may be included in the interest of providing
 *       API functionality, but the includer of this file
 *       should not rely on such behavior.
 */

#ifndef ISOLARIO_THREADING_H_
#define ISOLARIO_THREADING_H_

#include <stddef.h>
#ifdef __SSE2__
#include <emmintrin.h>
#endif

typedef struct pool_s pool_t;

/**
 * @brief Retrieve a platform-specific integral unique descriptor of the calling
 *        thread.
 *
 * @return The platform specific integral descriptor of the calling thread.
 *         If this platform does not provide any mean of retrieving such
 *         descriptor, then 0 is returned, and \a errno is set to \a ENOTSUP.
 *
 * @warning Since 0, in theory, is a valid platform specific descriptor for a
 *          thread, the caller should set \a errno to 0 and check it afterwards
 *          to ensure this function was successful.
 */
unsigned long long getthreaddescr(void);

/**
 * @brief SMT pause hint for busy loops.
 *
 * @see [Benefitting power and performance sleep loops](https://software.intel.com/en-us/articles/benefitting-power-and-performance-sleep-loops)
 */
inline void smtpause(void)
{
#ifdef __SSE2__
    _mm_pause();
#endif
}

pool_t *pool_create(int nthreads, void (*handle_job)(void *job));

int pool_nthreads(pool_t *pool);

int pool_dispatch(pool_t *pool, const void *job, size_t size);

void pool_join(pool_t *pool);

#endif
