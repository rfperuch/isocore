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

#include <isolario/backtrace.h>
#include <isolario/branch.h>

#ifdef __GNUC__
#include <execinfo.h>
#endif
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#ifdef __GNUC__

size_t btrace(void **trace, size_t size)
{
    size_t n = size + 1;
    void *buf[n];

    n = backtrace(buf, n);
    if (n < 1)
        return 0;

    n--;  // omit this function itself from backtrace

    memcpy(trace, &buf[1], n * sizeof(*trace));
    return n;
}


void *caller(int depth)
{
    // 2 since 0 is this function, 1 is this function's caller
    if (unlikely(depth < 0))
        return NULL;

    int n = depth + 2;
    void *trace[n];

    int count = backtrace(trace, n);
    return (count == n) ? trace[depth + 1] : NULL;
}


char *symname(void *sym)
{
    static _Thread_local char buf[1024];
    if (unlikely(!sym))
        return NULL;

    char **names = backtrace_symbols(&sym, 1);
    if (unlikely(!names))
        return NULL;

    strncpy(buf, names[0], sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    free(names);
    return buf;
}

#else

size_t backtrace(void **trace, size_t size)
{
    (void) trace;
    (void) size;

    return 0;
}

void *caller(int depth)
{
    (void) depth;

    return NULL;
}

char *symname(void *sym)
{
    (void) sym;

    return NULL;
}

#endif
