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

#ifndef ISOLARIO_CACHE_H_
#define ISOLARIO_CACHE_H_

#include <stddef.h>

#if defined(__i386__) || defined(__x86_64__)
#define CPU_CACHELINE_ALIGN ((size_t) 64)
#elif defined(__powerpc64__)
#define CPU_CACHELINE_ALIGN ((size_t) 128)
#elif defined(__arm__)

#if defined(__ARM_ARCH_5T__)
#define CPU_CACHELINE_ALIGN ((size_t) 32)
#elif defined(__ARM_ARCH_7A__)
#define CPU_CACHELINE_ALIGN ((size_t) 64)
#endif

#endif

#ifndef CPU_CACHELINE_ALIGN
// conservative guess
#define CPU_CACHELINE_ALIGN sizeof(max_align_t)

#endif

/// @brief Retrieve cacheline size at runtime, by directly querying the kernel.
size_t cacheline(void);

typedef enum {
    CACHE_NONE = 0,
    CACHE_MODERATE,
    CACHE_HIGH
} cache_locality_t;

#ifdef __GNUC__
#define memprefetch(addr, locality) __builtin_prefetch(addr, locality)
#else
#define memprefetch(addr, locality) ((void) (addr), (void) (locality))
#endif

#endif
