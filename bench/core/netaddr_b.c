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

#include <isolario/endian.h>
#include <cbench/cbench.h>
#include <isolario/netaddr.h>
#include <string.h>

volatile int bh;

void bprefixeqwithmask(cbench_state_t *state)
{
    netaddr_t addr, dest;
    memset(&addr, 0, sizeof(addr));
    memset(&dest, 0, sizeof(dest));
    addr.family = dest.family = AF_INET;
    addr.bitlen = dest.bitlen = 32;

    while (cbench_next_iteration(state)) {
        addr.u32[0] = tobig32(state->curiter);
        dest.u32[0] = state->curiter;
        bh = prefixeqwithmask(&addr, &dest, state->curiter % 129);
    }
}

static int patcompwithmask(const netaddr_t *addr, const netaddr_t *dest, int mask)
{
    if (memcmp(&addr->bytes[0], &dest->bytes[0], mask / 8) == 0) {
        int n = mask / 8;
        int m = ((unsigned int) (~0) << (8 - (mask % 8)));

        if (((mask & 0x7) == 0) || ((addr->bytes[n] & m) == (dest->bytes[n] & m)))
            return 1;
    }

    return 0;
}

void bppathcompwithmask(cbench_state_t *state)
{
    netaddr_t addr, dest;
    memset(&addr, 0, sizeof(addr));
    memset(&dest, 0, sizeof(dest));
    addr.family = dest.family = AF_INET;
    addr.bitlen = dest.bitlen = 32;

    while (cbench_next_iteration(state)) {
        addr.u32[0] = tobig32(state->curiter);
        dest.u32[0] = state->curiter;
        bh = patcompwithmask(&addr, &dest, state->curiter % 129);
    }
}
