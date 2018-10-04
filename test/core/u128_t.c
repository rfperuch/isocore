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

#include <CUnit/CUnit.h>
#include <isolario/u128_t.h>
#include <stdlib.h>

void testu128iter(void)
{
    int expect = 0;
    for (u128_t i = UINT128_ZERO; u128cmpu(i, 100) < 0; i = u128addu(i, 1)) {
        CU_ASSERT(u128cmpu(i, expect) == 0);
        expect++;
    }
}

enum {
    CONV_SCALE = 2,
    CONV_STEP = 7
};

void testu128conv(void)
{
    u128_t u;
    char *s;

    u128_t limit = u128divu(UINT128_MAX, CONV_SCALE);
    limit = u128subu(limit, CONV_STEP);

    for (u = UINT128_ZERO; u128cmp(u, limit) < 0; u = u128muladdu(u, CONV_SCALE, CONV_STEP)) {
        s = u128tos(u, 10);
        CU_ASSERT(u128cmp(u, stou128(s, NULL, 10)) == 0);

        s = u128tos(u, 2);
        CU_ASSERT(u128cmp(u, stou128(s, NULL, 2)) == 0);

        s = u128tos(u, 8);
        CU_ASSERT(u128cmp(u, stou128(s, NULL, 8)) == 0);

        s = u128tos(u, 16);
        CU_ASSERT(u128cmp(u, stou128(s, NULL, 16)) == 0);

        s = u128tos(u, 36);
        CU_ASSERT(u128cmp(u, stou128(s, NULL, 36)) == 0);
    }

    u = UINT128_MAX;
    s = u128tos(u, 10);
    CU_ASSERT(u128cmp(u, stou128(s, NULL, 10)) == 0);

    s = u128tos(u, 2);
    CU_ASSERT(u128cmp(u, stou128(s, NULL, 2)) == 0);

    s = u128tos(u, 8);
    CU_ASSERT(u128cmp(u, stou128(s, NULL, 8)) == 0);

    s = u128tos(u, 16);
    CU_ASSERT(u128cmp(u, stou128(s, NULL, 16)) == 0);

    s = u128tos(u, 36);
    CU_ASSERT(u128cmp(u, stou128(s, NULL, 36)) == 0);
}

