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

#include <cbench/cbench.h>
#include <inttypes.h>
#include <isolario/strutil.h>
#include <isolario/util.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

void bulltoa(cbench_state_t *state)
{
    char buf[digsof(unsigned long long) + 1];

    unsigned long long x = ULLONG_MAX;

    while (cbench_next_iteration(state)) {
        ulltoa(buf, NULL, x);
        x--;
    }
}

void bcommsprintf(cbench_state_t *state)
{
    char buf[digsof(uint16_t) + 1 + digsof(uint16_t) + 1];

    uint32_t u = UINT32_MAX;

    while (cbench_next_iteration(state)) {
        sprintf(buf, "%"PRIu16":%"PRIu16, (uint16_t) (u >> 16), (uint16_t) (u & 0xffff));
    }
}

void bsprintf(cbench_state_t *state)
{
    char buf[digsof(unsigned long long) + 1];

    unsigned long long x = ULLONG_MAX;

    while (cbench_next_iteration(state)) {
        sprintf(buf, "%llu", x);
        x--;
    }
}

void bcommulltoa(cbench_state_t *state)
{
    char buf[digsof(uint16_t) + 1 + digsof(uint16_t) + 1];

    uint32_t u = UINT32_MAX;
    char *ptr;
    while (cbench_next_iteration(state)) {
        utoa(buf, &ptr, u >> 16);
        *ptr++ = ':';
        utoa(ptr, NULL, u & 0xffff);
        u--;
    }
}

void bsplit(cbench_state_t *state)
{
    while (cbench_next_iteration(state)) {
        char **str = splitstr(" ", "a b c d e f g h i j k l m n o p q r s t u v w x y z", NULL);
        free(str);
    }
}

void bjoinv(cbench_state_t *state)
{
    while (cbench_next_iteration(state)) {
        char *str = joinstrv(" ", "a", "b", "c", "d", "e", "f",
                                  "g", "h", "i", "j", "k", "l",
                                  "m", "n", "o", "p", "q", "r",
                                  "s", "t", "u", "v", "w", "x",
                                  "y", "z", (char *) NULL);
        free(str);
    }
}

void bjoin(cbench_state_t *state)
{
    static char *strarr[] = {
        "a", "b", "c", "d", "e", "f",
        "g", "h", "i", "j", "k", "l",
        "m", "n", "o", "p", "q", "r",
        "s", "t", "u", "v", "w", "x",
        "y", "z"
    };

    while (cbench_next_iteration(state)) {
        char *str = joinstr(" ", strarr, nelems(strarr));
        free(str);
    }
}

