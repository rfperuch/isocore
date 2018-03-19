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

#include <ctype.h>
#include <errno.h>
#include <isolario/uint128_t.h>
#include <isolario/util.h>
#include <stdbool.h>

static int digval(int ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    else if (ch >= 'A' && ch <= 'Z')
        return ch - 'A' + 10;
    else if (ch >= 'a' && ch <= 'z')
        return ch - 'a' + 10;
    else
        return -1;
}

uint128_t stou128(const char *s, char **eptr, int base)
{
    while (isspace(*s)) s++;

    bool minus = false;
    if (*s == '-' || *s == '+')
        minus = (*s++ == '-');

    if (base == 0) {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
            s += 2;
            base = 16;
        } else if (s[0] == '0') {
            s++;
            base = 8;
        } else {
            base = 10;
        }
    } else if (base == 16) {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
            s += 2;
    }

    int dig;
    uint128_t u = UINT128_ZERO;
    while ((dig = digval(*s)) >= 0 && dig < base) {
        uint128_t v = u128muladdu(u, base, dig);
        if (unlikely(u128cmp(u, v) > 0)) {
            // overflow (keep going to consume all digits in string)
            errno = ERANGE;
            v = UINT128_MAX;
        }

        u = v;
        s++;
    }

    if (eptr)
        *eptr = (char *) s;

    if (minus)
        u = u128neg(u);

    return u;
}

extern uint128_t u128init(uint64_t up, uint64_t lw);

extern uint128_t tou128(uint64_t u);

extern uint64_t u128upper(uint128_t u);

extern uint64_t u128lower(uint128_t u);

extern uint128_t u128add(uint128_t a, uint128_t b);

extern uint128_t u128addu(uint128_t a, uint64_t b);

extern uint128_t u128sub(uint128_t a, uint128_t b);

extern uint128_t u128subu(uint128_t a, uint64_t b);

extern uint128_t u128neg(uint128_t u);

#if defined(__GNUC__) && !defined(ISOLARIO_C_UINT128_T)

extern uint128_t u128mul(uint128_t a, uint128_t b);

#else

uint128_t u128mul(uint128_t a, uint128_t b)
{
    // split values into 4 32-bit parts
    uint64_t top[4] = {
        a.upper >> 32,
        a.upper & 0xffffffff,
        a.lower >> 32,
        a.lower & 0xffffffff
    };
    uint64_t bottom[4] = {
        b.upper >> 32,
        b.upper & 0xffffffff,
        b.lower >> 32,
        b.lower & 0xffffffff
    };

    uint64_t products[4][4];

    for (int y = 3; y > -1; y--) {
        for (int x = 3; x > -1; x--)
            products[3 - x][y] = top[x] * bottom[y];
    }

    // initial row
    uint64_t fourth32 = products[0][3] & 0xffffffff;
    uint64_t third32 = (products[0][2] & 0xffffffff) + (products[0][3] >> 32);
    uint64_t second32 = (products[0][1] & 0xffffffff) + (products[0][2] >> 32);
    uint64_t first32 = (products[0][0] & 0xffffffff) + (products[0][1] >> 32);

    // second row
    third32 += products[1][3] & 0xffffffff;
    second32 += (products[1][2] & 0xffffffff) + (products[1][3] >> 32);
    first32 += (products[1][1] & 0xffffffff) + (products[1][2] >> 32);

    // third row
    second32 += products[2][3] & 0xffffffff;
    first32 += (products[2][2] & 0xffffffff) + (products[2][3] >> 32);

    // fourth row
    first32 += products[3][3] & 0xffffffff;

    // combines the values, taking care of carry over
    uint128_t x = u128from(first32 << 32, 0);
    uint128_t y = u128from(third32 >> 32, third32 << 32);
    uint128_t z = u128from(second32, 0);
    uint128_t w = tou128(fourth32);

    uint128_t r = u128add(x, y);
    r           = u128add(r, z);
    r           = u128add(r, w);
    return r;
}

#endif

extern uint128_t u128mulu(uint128_t a, uint64_t b);

extern uint128_t u128muladd(uint128_t a, uint128_t b, uint128_t c);

extern uint128_t u128muladdu(uint128_t a, uint64_t b, uint64_t c);

#if defined(__GNUC__) && !defined(ISOLARIO_C_UINT128_T)

extern udiv128_t u128divqr(uint128_t a, uint128_t b);

#else

udiv128_t u128divqr(uint128_t a, uint128_t b)
{
    udiv128_t qr = {
        .quot = 0,
        .rem = b
    };

    // keep dreaming about trivial cases...
    if (unlikely(u128cmp(b, UINT128_ONE) == 0)) {
        qr.quot = a;
        qr.rem = 0;
        return qr;
    }
    if (unlikely(u128cmp(a, b) == 0)) {
        qr.quot = 1;
        qr.rem = 0;
        return qr;
    }
    if (u128cmp(a, UINT128_ZERO) == 0 || u128cmp(a, b) < 0) {
        qr.quot = 0;
        qr.rem = b;
        return qr;
    }

    // sorry state of affairs...
    int abits = u128bits(a);
    int bbits = u128bits(b);
    uint128_t copyd = u128shl(b, abits - bbits);
    uint128_t adder = u128shl(UINT128_ONE, abits - bbits);
    if (u128cmp(copyd, qr.rem) > 0) {
        u128shru(copyd, 1);
        u128shru(adder, 1);
    }
    while (u128cmp(qr.rem, b) >= 0) {
        if (u128cmp(qr.rem, copyd) >= 0) {
            qr.rem = u128sub(qr.rem, copyd);
            qr.quot = u128or(qr.quot, adder);
        }
        u128shr(copyd, 1);
        u128shr(adder, 1);
    }
    return qr;
}

#endif

extern uint128_t u128div(uint128_t a, uint128_t b);

extern uint128_t u128divu(uint128_t a, uint64_t b);

extern uint128_t u128mod(uint128_t a, uint128_t b);

extern uint128_t u128modu(uint128_t a, uint64_t b);

extern udiv128_t u128divqr(uint128_t a, uint128_t b);

extern udiv128_t u128divqru(uint128_t a, uint64_t b);

extern uint128_t u128and(uint128_t a, uint128_t b);

extern uint128_t u128andu(uint128_t a, uint64_t b);

extern uint128_t u128or(uint128_t a, uint128_t b);

extern uint128_t u128oru(uint128_t a, uint64_t b);

extern uint128_t u128xor(uint128_t a, uint128_t b);

extern uint128_t u128xoru(uint128_t a, uint64_t b);

extern uint128_t u128cpl(uint128_t u);

extern uint128_t u128shl(uint128_t u, int bits);

extern uint128_t u128shr(uint128_t u, int bits);

extern int u128bits(uint128_t u);

extern int u128cmp(uint128_t a, uint128_t b);

extern int u128cmpu(uint128_t a, uint64_t b);

char *u128tos(uint128_t u, int base)
{
    static _Thread_local char buf[2 + digsof(u) + 1];

    if (base < 2 || base > 36)
        base = 0;

    if (base == 0)
        base = 10;

    udiv128_t qr = {
        .quot = u,
        .rem  = 0
    };

    const char *digs = "0123456789abcdefghijklmnopqrstuvwxyz";
    char *ptr = buf + sizeof(buf) - 1;

    *ptr = '\0';
    do {
        qr = u128divqru(qr.quot, base);

        *--ptr = digs[u128lower(qr.rem)];
    } while (u128cmp(qr.quot, UINT128_ZERO) != 0);

    if (base == 16) {
        *--ptr = 'x';
        *--ptr = '0';
    }
    if (base == 8 && u128cmp(u, UINT128_ZERO) != 0)
        *--ptr = '0';

    return ptr;
}

