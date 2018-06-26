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
 * @file isolario/uint128_t.h
 *
 * @brief 128-bits precision unsigned integer types and functions.
 *
 * @note This file is guaranteed to include \a stdint.h, it may include
 *       additional standard or isolario-specific headers in the interest of
 *       providing inline implementations of its functions.
 */

#ifndef ISOLARIO_UINT128_T_H_
#define ISOLARIO_UINT128_T_H_

#include <assert.h>
#include <isolario/bits.h>
#include <isolario/branch.h>
#include <isolario/funcattribs.h>
#include <stdint.h>

/**
 * @brief Opaque structure representing an unsigned integer with 128-bits
 *        precision.
 *
 * @warning This structure's fields are to be considered private and
 *          must not be accessed directly.
 */
typedef struct {
#if defined(__GNUC__) && !defined(ISOLARIO_C_UINT128_T)
    // use compiler supplied type
    __uint128_t u128;
#else
    // emulate in plain C
    uint64_t upper, lower;
#endif
} uint128_t;

static_assert(sizeof(uint128_t) == 16, "Unsupported platform");

/// @brief Result returned by u128divqr().
typedef struct {
    uint128_t quot, rem;  ///< Quotient and remainder.
} udiv128_t;

#if defined(__GNUC__) && !defined(ISOLARIO_C_UINT128_T)

static const uint128_t UINT128_ZERO = { .u128 = 0  };
static const uint128_t UINT128_ONE  = { .u128 = 1  };
static const uint128_t UINT128_TEN  = { .u128 = 10 };
static const uint128_t UINT128_MAX  = { .u128 = -1 };

#else

static const uint128_t UINT128_ZERO = { .upper = 0, .lower = 0  };
static const uint128_t UINT128_ONE  = { .upper = 0, .lower = 1  };
static const uint128_t UINT128_TEN  = { .upper = 0, .lower = 10 };
static const uint128_t UINT128_MAX  = {
    .upper = UINT64_MAX,
    .lower = UINT64_MAX
};

#endif

inline constfunc uint128_t u128from(uint64_t up, uint64_t lw)
{
    uint128_t r;
#if defined(__GNUC__) && !defined(ISOLARIO_C_UINT128_T)
    r.u128 =   up;
    r.u128 <<= 64;
    r.u128 |=  lw;
#else
    r.upper = up;
    r.lower = lw;
#endif
    return r;
}

inline constfunc uint128_t tou128(uint64_t u)
{
    uint128_t r;
#if defined(__GNUC__) && !defined(ISOLARIO_C_UINT128_T)
    r.u128 = u;
#else
    r.upper = 0;
    r.lower = u;
#endif
    return r;
}

inline constfunc uint64_t u128upper(uint128_t u)
{
#if defined(__GNUC__) && !defined(ISOLARIO_C_UINT128_T)
    return u.u128 >> 64;
#else
    return u.upper;
#endif
}

inline constfunc uint64_t u128lower(uint128_t u)
{
#if defined(__GNUC__) && !defined(ISOLARIO_C_UINT128_T)
    return u.u128 & 0xffffffffffffffffull;
#else
    return u.lower;
#endif
}

inline constfunc uint128_t u128add(uint128_t a, uint128_t b)
{
#if defined(__GNUC__) && !defined(ISOLARIO_C_UINT128_T)

    a.u128 += b.u128;

#else

    uint64_t t = a.lower + b.lower;

    a.upper += (b.upper + (t < a.lower));
    a.lower = t;

#endif
    return a;
}

inline constfunc uint128_t u128addu(uint128_t a, uint64_t b)
{
    return u128add(a, tou128(b));
}

inline constfunc uint128_t u128sub(uint128_t a, uint128_t b)
{
#if defined(__GNUC__) && !defined(ISOLARIO_C_UINT128_T)

    a.u128 -= b.u128;

#else

    uint64_t t = a.lower - b.lower;
    a.upper -= (b.upper - (t > a.lower));
    a.lower = t;

#endif
    return a;
}

inline constfunc uint128_t u128subu(uint128_t a, uint64_t b)
{
    return u128sub(a, tou128(b));
}

inline constfunc uint128_t u128neg(uint128_t u)
{
#if defined(__GNUC__) && !defined(ISOLARIO_C_UINT128_T)
    u.u128 = -u.u128;
#else
    // don't use UINT128_ZERO, otherwise it wouldn't be constfunc!
    const uint128_t zero = { .upper = 0, .lower = 0 };
    u = u128sub(zero, u);

#endif
    return u;
}

#if defined(__GNUC__) && !defined(ISOLARIO_C_UINT128_T)

inline constfunc uint128_t u128mul(uint128_t a, uint128_t b)
{
    a.u128 *= b.u128;
    return a;
}

#else

constfunc  uint128_t u128mul(uint128_t a, uint128_t b);

#endif

inline constfunc uint128_t u128mulu(uint128_t a, uint64_t b)
{
    return u128mul(a, tou128(b));
}

/**
 * @brief Convenience for multiply-add operation.
 *
 * Shorthand for:
 * @code
 *    u128add(u128mul(a, b), c);
 * @endcode
 */
inline constfunc uint128_t u128muladd(uint128_t a, uint128_t b, uint128_t c)
{
    return u128add(u128mul(a, b), c);
}

/**
 * @brief Multiply-add between \a uint128_t and plain unsigned integers.
 *
 * Shorthand for:
 * @code
 *    u128muladd(a, tou128(b), tou128(c));
 * @endcode
 */
inline constfunc uint128_t u128muladdu(uint128_t a, uint64_t b, uint64_t c)
{
    return u128muladd(a, tou128(b), tou128(c));
}

#if defined(__GNUC__) && !defined(ISOLARIO_C_UINT128_T)

inline constfunc udiv128_t u128divqr(uint128_t a, uint128_t b)
{
    udiv128_t qr;
    qr.quot.u128 = a.u128 / b.u128;
    qr.rem.u128  = a.u128 % b.u128;
    return qr;
}

#else

constfunc udiv128_t u128divqr(uint128_t a, uint128_t b);

#endif

inline constfunc udiv128_t u128divqru(uint128_t a, uint64_t b)
{
    return u128divqr(a, tou128(b));
}

inline constfunc uint128_t u128div(uint128_t a, uint128_t b)
{
#if defined(__GNUC__) && !defined(ISOLARIO_C_UINT128_T)

    a.u128 /= b.u128;
    return a;

#else

    return u128divqr(a, b).quot;

#endif
}

inline constfunc uint128_t u128divu(uint128_t a, uint64_t b)
{
    return u128div(a, tou128(b));
}

inline constfunc uint128_t u128mod(uint128_t a, uint128_t b)
{
#if defined(__GNUC__) && !defined(ISOLARIO_C_UINT128_T)

    a.u128 %= b.u128;
    return a;

#else

    return u128divqr(a, b).rem;

#endif
}

inline constfunc uint128_t u128modu(uint128_t a, uint64_t b)
{
    return u128mod(a, tou128(b));
}

inline constfunc uint128_t u128and(uint128_t a, uint128_t b)
{
#if defined(__GNUC__) && !defined(ISOLARIO_C_UINT128_T)
    a.u128 &= b.u128;
#else
    a.upper &= b.upper;
    a.lower &= b.lower;
#endif
    return a;
}

inline constfunc uint128_t u128andu(uint128_t a, uint64_t b)
{
    return u128and(a, tou128(b));
}

inline constfunc uint128_t u128or(uint128_t a, uint128_t b)
{
#if defined(__GNUC__) && !defined(ISOLARIO_C_UINT128_T)
    a.u128 |= b.u128;
#else
    a.upper |= b.upper;
    a.lower |= b.lower;
#endif
    return a;
}

inline constfunc uint128_t u128oru(uint128_t a, uint64_t b)
{
    return u128or(a, tou128(b));
}

inline constfunc uint128_t u128xor(uint128_t a, uint128_t b)
{
#if defined(__GNUC__) && !defined(ISOLARIO_C_UINT128_T)
    a.u128 ^= b.u128;
#else
    a.upper ^= b.upper;
    a.lower ^= b.upper;
#endif
    return a;
}

inline constfunc uint128_t u128xoru(uint128_t a, uint64_t b)
{
    return u128xor(a, tou128(b));
}

inline constfunc uint128_t u128cpl(uint128_t u)
{
#if defined(__GNUC__) && !defined(ISOLARIO_C_UINT128_T)
    u.u128 = ~u.u128;
#else
    u.upper = ~u.upper;
    u.lower = ~u.lower;
#endif
    return u;
}

inline constfunc uint128_t u128shl(uint128_t u, int bits)
{
#if defined(__GNUC__) && !defined(ISOLARIO_C_UINT128_T)

    u.u128 <<= bits;
    return u;

#else

    // don't access UINT128_ZERO, otherwise it wouldn't be constfunc!
    if (unlikely(bits < 0 || bits >= 128))
        return uint128_t{ .upper = 0, .lower = 0};

    if (bits < 64) {
        u.upper = (u.upper << bits) | (u.lower >> (64 - bits));
        u.lower <<= bits;
    } else {
        u.upper = (u.lower << (bits - 64));
        u.lower = 0;
    }
    return u;

#endif
}

inline constfunc uint128_t u128shr(uint128_t u, int bits)
{
#if defined(__GNUC__) && !defined(ISOLARIO_C_UINT128_T)

    u.u128 >>= bits;
    return u;

#else

    // don't access UINT128_ZERO, otherwise it wouldn't be constfunc!
    if (unlikely(bits < 0 || bits >= 128))
        return uint128_t{ .upper = 0, .lower = 0};

    if (bits < 64) {
        u.lower = (u.upper << (64 - bits)) | (u.lower >> bits);
        u.upper = (u.upper >> bits);
    } else {
        u.lower = u.upper >> (bits - 64);
        u.upper = 0;
    }

    return u;

#endif
}

/// @brief Calculate the number of bits necessary to represent a value.
inline constfunc int u128bits(uint128_t u)
{
    uint64_t up = u128upper(u);
    uint64_t lw = u128lower(u);

    // XXX improve, up != 0 and cup == 64 are redundant
    int cup = (up != 0) ? bitclzll(up) : 64;
    int clw = (lw != 0) ? bitclzll(lw) : 64;
    int lz = (cup == 64) ? cup + clw : cup;
    return 128 - lz;
}

inline constfunc int u128cmp(uint128_t a, uint128_t b)
{
#if defined(__GNUC__) && !defined(ISOLARIO_C_UINT128_T)

    return (a.u128 > b.u128) - (a.u128 < b.u128);

#else

    if (a.upper != b.upper)
        return (a.upper > b.upper) - (a.upper < b.upper);

    return (a.lower > b.lower) - (a.lower < b.lower);

#endif
}

inline constfunc int u128cmpu(uint128_t a, uint64_t b)
{
    return u128cmp(a, tou128(b));
}

/**
 * @brief Convert a \a uint128_t to its string representation.
 *
 * @return The string representation of the provided unsigned integer in the
 *         requested base, the returned pointer references a possibly statically
 *         allocated storage managed by the library, it must not be free()d.
 */
nonullreturn char *u128tos(uint128_t u, int base);

/**
 * @brief Convert string to \a uint128_t.
 *
 * @return The integer value in string as a \a uint128_t, sets \a errno
 *         on error.
 */
purefunc nonnull(1) uint128_t stou128(const char *s, char **eptr, int base);

#endif
