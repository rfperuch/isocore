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

#ifndef ISOLARIO_ENDIAN_H_
#define ISOLARIO_ENDIAN_H_

#include <stdint.h>

typedef enum {
#ifdef _WIN32

    ENDIAN_BIG = 0,
#define ENDIAN_BIG 0

    ENDIAN_LITTLE = 1,
#define ENDIAN_LITTLE 1

    ENDIAN_NATIVE = ENDIAN_LITTLE
#define ENDIAN_NATIVE ENDIAN_LITTLE

#else

    ENDIAN_BIG = __ORDER_BIG_ENDIAN__,
#define ENDIAN_BIG __ORDER_BIG_ENDIAN__

    ENDIAN_LITTLE = __ORDER_LITTLE_ENDIAN__,
#define ENDIAN_LITTLE __ORDER_LITTLE_ENDIAN__

    ENDIAN_NATIVE = __BYTE_ORDER__
#define ENDIAN_NATIVE __BYTE_ORDER__

#endif
} byte_order_t;

// paranoid checks...
#ifndef __STDC_IEC_559__
#error endian.h requires IEEE 754 floating point
#endif
#if (ENDIAN_NATIVE != ENDIAN_LITTLE && ENDIAN_NATIVE != ENDIAN_BIG)
#error endian.h requires either a little endian or big endian host
#endif

typedef union {
    float f;
    uint32_t u;
} floatint_t;

typedef union {
    double d;
    uint64_t u;
} doubleint_t;

#if ENDIAN_NATIVE == ENDIAN_BIG

#define BIG16_INIT(C) (C)
#define LITTLE16_INIT(C) ((((C) & 0xff) << 8) | (((C) & 0xff00) >> 8))

#define BIG32_INIT(C) (C)
#define LITTLE32_INIT(C) ( \
    (((C) & 0xff) << 24) | (((C) & 0xff00) << 16) | \
    (((C) & 0xff0000) << 8) | (((C) & 0xff000000) >> 24) \
)

#define BIG64_INIT(C) (C)
#define LITTLE64_INIT(C) ( \
    ((C) & 0xffull << 56) | (((C) & 0xff00ull) << 48) | \
    (((C) & 0xff0000ull) << 40) | (((C) & 0xff000000ull) << 32) | \
    (((C) & 0xff00000000ull) << 24) | (((C) & 0xff0000000000ull) << 16) | \
    (((C) & 0xff00000000000000ull) << 8) | (((C) & 0xff00000000000000ull) >> 56) \
)

#else

#define BIG16_INIT(C) ((((C) & 0xff) << 8) | (((C) & 0xff00) >> 8))
#define LITTLE16_INIT(C) (C)

#define BIG32_INIT(C) ( \
    (((C) & 0xff) << 24) | (((C) & 0xff00) << 16) | \
    (((C) & 0xff0000) << 8) | (((C) & 0xff000000) >> 24) \
)
#define LITTLE32_INIT(C) (C)

#define BIG64_INIT(C) ( \
    ((C) & 0xffull << 56) | (((C) & 0xff00ull) << 48) | \
    (((C) & 0xff0000ull) << 40) | (((C) & 0xff000000ull) << 32) | \
    (((C) & 0xff00000000ull) << 24) | (((C) & 0xff0000000000ull) << 16) | \
    (((C) & 0xff00000000000000ull) << 8) | (((C) & 0xff00000000000000ull) >> 56) \
)
#define LITTLE64_INIT(C) (C)

#endif

inline uint16_t byteswap16(uint16_t w)
{
#ifdef __GNUC__
    return __builtin_bswap16(w);
#else
    return ((w & 0xff00) >> 8) | ((w & 0x00ff) << 8);
#endif
}

inline uint32_t byteswap32(uint32_t l)
{
#ifdef __GNUC__
    return __builtin_bswap32(l);
#else
    return ((l & 0xff000000) >> 24) | ((l & 0x00ff0000) >> 8) |
           ((l & 0x0000ff00) << 8)  | ((l & 0x000000ff) << 24);
#endif
}

inline uint64_t byteswap64(uint64_t ll)
{
#ifdef __GNUC__
    return __builtin_bswap64(ll);
#else
    return ((ll & 0xff00000000000000ull) >> 56) | ((ll & 0x00ff000000000000ull) >> 40) |
           ((ll & 0x0000ff0000000000ull) >> 24) | ((ll & 0x000000ff00000000ull) >> 8)  |
           ((ll & 0x00000000ff000000ull) << 8)  | ((ll & 0x0000000000ff0000ull) << 24) |
           ((ll & 0x000000000000ff00ull) << 40) | ((ll & 0x00000000000000ffull) << 56);
#endif
}

/// @brief Swap from 16-bits little endian.
inline uint16_t fromlittle16(uint16_t w)
{
    if (ENDIAN_NATIVE != ENDIAN_LITTLE)
        w = byteswap16(w);

    return w;
}

/// @brief Swap from 32-bits little endian.
inline uint32_t fromlittle32(uint32_t l)
{
    if (ENDIAN_NATIVE != ENDIAN_LITTLE)
        l = byteswap32(l);

    return l;
}

/// @brief Swap from 64-bits little endian.
inline uint64_t fromlittle64(uint64_t ll)
{
    if (ENDIAN_NATIVE != ENDIAN_LITTLE)
        ll = byteswap64(ll);

    return ll;
}

/// @brief Swap to 16-bits little endian.
inline uint16_t tolittle16(uint16_t w)
{
    if (ENDIAN_NATIVE != ENDIAN_LITTLE)
        w = byteswap16(w);

    return w;
}

/// @brief Swap to 32-bits little endian.
inline uint32_t tolittle32(uint32_t l)
{
    if (ENDIAN_NATIVE != ENDIAN_LITTLE)
        l = byteswap32(l);

    return l;
}

/// @brief Swap to 64-bits little endian.
inline uint64_t tolittle64(uint64_t ll)
{
    if (ENDIAN_NATIVE != ENDIAN_LITTLE)
        ll = byteswap64(ll);

    return ll;
}

/// @brief Swap from 16-bits big endian.
inline uint16_t frombig16(uint16_t w)
{
    if (ENDIAN_NATIVE != ENDIAN_BIG)
        w = byteswap16(w);

    return w;
}

/// @brief Swap from 32-bits big endian.
inline uint32_t frombig32(uint32_t l)
{
    if (ENDIAN_NATIVE != ENDIAN_BIG)
        l = byteswap32(l);

    return l;
}

/// @brief Swap from 64-bits big endian.
inline uint64_t frombig64(uint64_t ll)
{
    if (ENDIAN_NATIVE != ENDIAN_BIG)
        ll = byteswap64(ll);

    return ll;
}

/// @brief Swap to 16-bits big endian.
inline uint16_t tobig16(uint16_t w)
{
    if (ENDIAN_NATIVE != ENDIAN_BIG)
        w = byteswap16(w);

    return w;
}

/// @brief Swap to 32-bits big endian.
inline uint32_t tobig32(uint32_t l)
{
    if (ENDIAN_NATIVE != ENDIAN_BIG)
        l = byteswap32(l);

    return l;
}

/// @brief Swap to 64-bits big endian.
inline uint64_t tobig64(uint64_t ll)
{
    if (ENDIAN_NATIVE != ENDIAN_BIG)
        ll = byteswap64(ll);

    return ll;
}

#endif

