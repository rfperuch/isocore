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

#ifndef ISOLARIO_BITS_H_
#define ISOLARIO_BITS_H_
#include <limits.h>

inline unsigned int popcount(unsigned int bits)
{
#ifdef __GNUC__

    return __builtin_popcount(bits);

#else

    if (sizeof(bits) * CHAR_BIT == 32) {
        // https://graphics.stanford.edu/~seander/bithacks.html
        bits = bits - ((bits >> 1) & 0x55555555);
        bits = (bits & 0x33333333) + ((bits >> 2) & 0x33333333);
        return ((bits + (bits >> 4) & 0xf0f0f0f) * 0x1010101) >> 24;
    }

    unsigned int c;
    for (c = 0; bits != 0; c++)
        bits &= bits - 1;

    return c;

#endif
}

inline unsigned long long popcountll(unsigned long long bits)
{
#ifdef __GNUC__

    return __builtin_popcountll(bits);

#else

    if (sizeof(bits) * CHAR_BIT <= 128) {
        // https://graphics.stanford.edu/~seander/bithacks.html
        bits = bits - ((bits >> 1) & (~0ull / 3ull));
        bits = (bits & (~0ull / 15ull * 3ull)) + ((bits >> 2) & (~0ull / 15ull * 3ull));
        bits = (bits + (bits >> 4)) & (~0ull / 255ull * 15ull);
        return (bits * (~0ull / 255ull)) >> (sizeof(bits) - 1ull) * CHAR_BIT;
    }

    unsigned long long c;
    for (c = 0; bits != 0; c++)
        bits &= bits - 1;

    return c;

#endif
}

inline unsigned long popcountl(unsigned long bits)
{
    if (sizeof(bits) == sizeof(unsigned int))
        return popcount(bits);
    if (sizeof(bits) == sizeof(unsigned long long))
        return popcountll(bits);

    unsigned long c;
    for (c = 0; bits != 0; c++)
        bits &= bits - 1;

    return c;
}

#endif
