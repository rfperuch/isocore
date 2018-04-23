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
 * @file isolario/endian.h
 *
 * @brief Endian utilities and byte swapping.
 *
 * @note This file is guaranteed to include standard \a stdint.h.
 *       It may include additional standard and isolario-specific headers to
 *       provide its functionality.
 */

#ifndef ISOLARIO_ENDIAN_H_
#define ISOLARIO_ENDIAN_H_

#include <stdint.h>

/**
 * @defgroup FloatBytes Floating point types to raw bytes conversion.
 *
 * @brief Union types useful for accessing floating point data types byte-level
 *        representation.
 *
 * @{
 */

#ifndef __STDC_IEC_559__
#error endian.h requires IEEE 754 floating point
#endif

/**
 * @brief Raw bytes to \a double union.
 *
 * An Union value to convert bit patterns to their \a double representation and
 * vice-versa. Mostly useful to swap \a double values and to examine the
 * bit-level representation of \a doubles.
 *
 * @see byteswap64(), frombig64(), tobig64(), fromlittle64(), tolittle64()
 */
typedef union {
    double d;    ///< The \a double value corresponding to the current bit pattern.
    uint64_t u;  ///< The quadword corresponding to the current \a double value.
} doubleint_t;

/**
 * @brief Raw bytes to \a float union, same concept as @ref doubleint_t applied
 *        to \a float types.
 *
 * @see byteswap32(), frombig32(), tobig32(), fromlittle32(), tolittle32()
 */
typedef union {
    float f;     ///< The \a float value corresponding to the current bit pattern.
    uint32_t u;  ///< The longword corresponding to the current \a float value.
} floatint_t;

/** @} */

/**
 * @defgroup Byteswap Endian independent byte-swapping
 *
 * @brief Basic functions to swap bytes of fixed-size primitives.
 *
 * @{
 */

/**
 * @brief Swap bytes of a 16-bits word.
 *
 * @param [in] w The 16-bits word to be swapped.
 *
 * @return The resulting byte-swapped word.
 */
inline uint16_t byteswap16(uint16_t w)
{
#ifdef __GNUC__
    return __builtin_bswap16(w);
#else
    return ((w & 0xff00) >> 8) | ((w & 0x00ff) << 8);
#endif
}

/**
 * @brief Swap bytes of a 32-bits longword.
 *
 * @param [in] l The 32-bits longword to be swapped.
 *
 * @return The resulting byte-swapped longword.
 */
inline uint32_t byteswap32(uint32_t l)
{
#ifdef __GNUC__
    return __builtin_bswap32(l);
#else
    return ((l & 0xff000000) >> 24) | ((l & 0x00ff0000) >> 8) |
           ((l & 0x0000ff00) << 8)  | ((l & 0x000000ff) << 24);
#endif
}

/**
 * @brief Swap bytes of a 64-bits quadword.
 *
 * @param [in] q The 64-bits quadword to be swapped.
 *
 * @return The resulting byte-swapped quadword.
 */
inline uint64_t byteswap64(uint64_t q)
{
#ifdef __GNUC__
    return __builtin_bswap64(q);
#else
    return ((q & 0xff00000000000000ull) >> 56) | ((q & 0x00ff000000000000ull) >> 40) |
           ((q & 0x0000ff0000000000ull) >> 24) | ((q & 0x000000ff00000000ull) >> 8)  |
           ((q & 0x00000000ff000000ull) << 8)  | ((q & 0x0000000000ff0000ull) << 24) |
           ((q & 0x000000000000ff00ull) << 40) | ((q & 0x00000000000000ffull) << 56);
#endif
}

/** @} */

/**
 * @defgroup EndianConv Host to endian and endian to host conversion
 *
 * @brief Endian related utilities and functions.
 *
 * This group provides basic functions to convert fixed-size data types from
 * host byte ordering to different endianness, as well as various constants
 * to inspect host byte ordering at compile time.
 *
 * @{
 */

/** @def ENDIAN_BIG
 *
 * @brief A macro constant identifying the big endian byte ordering,
 *
 * @see #ENDIAN_NATIVE
 */

/** @def ENDIAN_LITTLE
 *
 * @brief A macro constant identifying the little endian byte ordering.
 *
 * @see #ENDIAN_NATIVE
 */

/** @def ENDIAN_NATIVE
 *
 * @brief Macro identifying host machine endianness.
 *
 * This macro expands to either \a ENDIAN_BIG or \a ENDIAN_LITTLE at preprocessing
 * time.
 *
 * @note This macro differs from its enumeration counterpart because it allows
 *       accessing the host byte ordering information at preprocessor level,
 *       which is seldom useful, whenever possible such test should be made
 *       inside a regular if at the C level, which is going to be optimized
 *       away by the compiler anyway. Though, when absolutely necessary, the
 *       macro variant offers the possibility of implementing preprocessor logic
 *       depending on such information.
 *
 * @see byte_order_t
 */

/** @enum byte_order_t
 *
 * @brief Byte ordering enumeration constants.
 *
 * This enumeration contains type-safe constants counterparts to the
 * #ENDIAN_BIG, #ENDIAN_LITTLE and #ENDIAN_NATIVE macros, this enumeration
 * should always be preferred to such macros when possible.
 */
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

#if (ENDIAN_NATIVE != ENDIAN_LITTLE && ENDIAN_NATIVE != ENDIAN_BIG)
#error endian.h requires either a little endian or big endian host
#endif

/**
 * @def BIG16_C(C)
 *
 * @brief Portable initialization of a 16-bits word in big-endian format.
 *
 * Macro used to initialize a 16-bits word to a constant encoded in big-endian.
 *
 * @param [in] C A compile-time constant to be encoded in big-endian.
 *
 * @return The constant in big-endian format, suitable to be used in an assignment
 *         to initialize a variable.
 *
 * @warning This macro is unsafe and may evaluate \a C more than once, only
 *          use this macro with compile-time constant arguments, otherwise
 *          always use tobig16().
 *
 * @def BIG32_C(C)
 *
 * @brief Portable initialization of a 32-bits longword in big-endian format.
 *
 * Macro used to initialize a 32-bits longword to a constant encoded in big-endian.
 *
 * @param [in] C A compile-time constant to be encoded in big-endian.
 *
 * @return The constant in big-endian format, suitable to be used in an
 *         assignment to initialize a variable.
 *
 * @warning This macro is unsafe and may evaluate \a C more than once, only
 *          use this macro with compile-time constant arguments, otherwise
 *          always use tobig32().
 *
 * @def BIG64_C(C)
 *
 * @brief Portable initialization of a 64-bits quadword in big-endian format.
 *
 * Macro used to initialize a 64-bits quadword to a constant encoded in big-endian.
 *
 * @param [in] C A compile-time constant to be encoded in big-endian.
 *
 * @return The constant in big-endian format, suitable to be used in an
 *         assignment to initialize a variable.
 *
 * @warning This macro is unsafe and may evaluate \a C more than once, only
 *          use this macro with compile-time constant arguments, otherwise
 *          always use tobig64().
 *
 * @def LITTLE16_C(C)
 *
 * @brief Portable initialization of a 16-bits word in little-endian format.
 *
 * Macro used to initialize a 16-bits word to a constant encoded in little-endian.
 *
 * @param [in] C A compile-time constant to be encoded in little-endian.
 *
 * @return The constant in little-endian format, suitable to be used in an assignment
 *         to initialize a variable.
 *
 * @warning This macro is unsafe and may evaluate \a C more than once, only
 *          use this macro with compile-time constant arguments, otherwise
 *          always use tolittle16().
 *
 * @def LITTLE32_C(C)
 *
 * @brief Portable initialization of a 32-bits longword in little-endian format.
 *
 * Macro used to initialize a 32-bits longword to a constant encoded in little-endian.
 *
 * @param [in] C A compile-time constant to be encoded in little-endian.
 *
 * @return The constant in little-endian format, suitable to be used in an
 *         assignment to initialize a variable.
 *
 * @warning This macro is unsafe and may evaluate \a C more than once, only
 *          use this macro with compile-time constant arguments, otherwise
 *          always use tolittle32().
 *
 * @def LITTLE64_C(C)
 *
 * @brief Portable initialization of a 64-bits quadword in little-endian format.
 *
 * Macro used to initialize a 64-bits quadword to a constant encoded in little-endian.
 *
 * @param [in] C A compile-time constant to be encoded in little-endian.
 *
 * @return The constant in little-endian format, suitable to be used in an
 *         assignment to initialize a variable.
 *
 * @warning This macro is unsafe and may evaluate \a C more than once, only
 *          use this macro with compile-time constant arguments, otherwise
 *          always use tolittle64().
 */
#if ENDIAN_NATIVE == ENDIAN_BIG

#define BIG16_C(C) (C)
#define LITTLE16_C(C) ((((C) & 0xff) << 8) | (((C) & 0xff00) >> 8))

#define BIG32_C(C) (C)
#define LITTLE32_C(C) (                                      \
    (((C) & 0xff000000) >> 24) | (((C) & 0x00ff0000) >> 8) | \
    (((C) & 0x0000ff00) << 8)  | (((C) & 0x000000ff) << 24)  \
)

#define BIG64_C(C) (C)
#define LITTLE64_C(C) (                                                             \
    (((C) & 0xff00000000000000ull) >> 56) | (((C) & 0x00ff000000000000ull) >> 40) | \
    (((C) & 0x0000ff0000000000ull) >> 24) | (((C) & 0x000000ff00000000ull) >> 8)  | \
    (((C) & 0x00000000ff000000ull) << 8)  | (((C) & 0x0000000000ff0000ull) << 24) | \
    (((C) & 0x000000000000ff00ull) << 40) | (((C) & 0x00000000000000ffull) << 56)   \
)

#else

#define BIG16_C(C) ((((C) & 0xff) << 8) | (((C) & 0xff00) >> 8))
#define LITTLE16_C(C) (C)

#define BIG32_C(C) (                                         \
    (((C) & 0xff000000) >> 24) | (((C) & 0x00ff0000) >> 8) | \
    (((C) & 0x0000ff00) << 8)  | (((C) & 0x000000ff) << 24)  \
)
#define LITTLE32_C(C) (C)

#define BIG64_C(C) (                                                                \
    (((C) & 0xff00000000000000ull) >> 56) | (((C) & 0x00ff000000000000ull) >> 40) | \
    (((C) & 0x0000ff0000000000ull) >> 24) | (((C) & 0x000000ff00000000ull) >> 8)  | \
    (((C) & 0x00000000ff000000ull) << 8)  | (((C) & 0x0000000000ff0000ull) << 24) | \
    (((C) & 0x000000000000ff00ull) << 40) | (((C) & 0x00000000000000ffull) << 56)   \
)
#define LITTLE64_C(C) (C)

#endif

/**
 * @brief Convert a 16-bits little endian word to a host word.
 *
 * @param [in] w A little endian 16-bits word.
 *
 * @return The corresponding 16-bits word in host byte order.
 *
 * @see tolittle16()
 */
inline uint16_t fromlittle16(uint16_t w)
{
    if (ENDIAN_NATIVE != ENDIAN_LITTLE)
        w = byteswap16(w);

    return w;
}

/**
 * @brief Convert a 32-bits little endian longword to a host longword.
 *
 * @param [in] l A little endian 32-bits longword.
 *
 * @return The corresponding 32-bits longword in host byte order.
 *
 * @see tolittle32()
 */
inline uint32_t fromlittle32(uint32_t l)
{
    if (ENDIAN_NATIVE != ENDIAN_LITTLE)
        l = byteswap32(l);

    return l;
}

/**
 * @brief Convert a 64-bits little endian quadword to a host quadword.
 *
 * @param [in] q A little endian 64-bits quadword.
 *
 * @return The corresponding 64-bits quadword in host byte order.
 *
 * @see tolittle64()
 */
inline uint64_t fromlittle64(uint64_t q)
{
    if (ENDIAN_NATIVE != ENDIAN_LITTLE)
        q = byteswap64(q);

    return q;
}

/**
 * @brief Convert a 16-bits host word to a little endian word.
 *
 * @param [in] w A 16-bits word in host byte ordering.
 *
 * @return The corresponding 16-bits word in little endian byte order.
 *
 * @see tolittle16()
 */
inline uint16_t tolittle16(uint16_t w)
{
    if (ENDIAN_NATIVE != ENDIAN_LITTLE)
        w = byteswap16(w);

    return w;
}

/**
 * @brief Convert a 32-bits host longword to a little endian longword.
 *
 * @param [in] l A 32-bits longword in host byte ordering.
 *
 * @return The corresponding 32-bits longword in little endian byte order.
 *
 * @see fromlittle32()
 */
inline uint32_t tolittle32(uint32_t l)
{
    if (ENDIAN_NATIVE != ENDIAN_LITTLE)
        l = byteswap32(l);

    return l;
}

/**
 * @brief Convert a 64-bits host quadword to a little endian quadword.
 *
 * @param [in] q A 64-bits quadword in host byte ordering.
 *
 * @return The corresponding 64-bits quadword in little endian byte order.
 *
 * @see fromlittle64()
 */
inline uint64_t tolittle64(uint64_t q)
{
    if (ENDIAN_NATIVE != ENDIAN_LITTLE)
        q = byteswap64(q);

    return q;
}


/**
 * @brief Convert a 16-bits host word to a big endian word.
 *
 * @param [in] w A 16-bits word in host byte ordering.
 *
 * @return The corresponding 16-bits word in big endian byte order.
 *
 * @see tobig16()
 */
inline uint16_t frombig16(uint16_t w)
{
    if (ENDIAN_NATIVE != ENDIAN_BIG)
        w = byteswap16(w);

    return w;
}

/**
 * @brief Convert a 32-bits big endian longword to a host longword.
 *
 * @param [in] l A big endian 32-bits longword.
 *
 * @return The corresponding 32-bits longword in host byte order.
 *
 * @see tobig32()
 */
inline uint32_t frombig32(uint32_t l)
{
    if (ENDIAN_NATIVE != ENDIAN_BIG)
        l = byteswap32(l);

    return l;
}

/**
 * @brief Convert a 64-bits big endian quadword to a host quadword.
 *
 * @param [in] q A big endian 64-bits quadword.
 *
 * @return The corresponding 64-bits quadword in host byte order.
 *
 * @see tolittle64()
 */
inline uint64_t frombig64(uint64_t q)
{
    if (ENDIAN_NATIVE != ENDIAN_BIG)
        q = byteswap64(q);

    return q;
}

/**
 * @brief Convert a 16-bits host word to a big endian word.
 *
 * @param [in] w A 16-bits word in host byte ordering.
 *
 * @return The corresponding 16-bits word in big endian byte order.
 *
 * @see tobig16()
 */
inline uint16_t tobig16(uint16_t w)
{
    if (ENDIAN_NATIVE != ENDIAN_BIG)
        w = byteswap16(w);

    return w;
}

/**
 * @brief Convert a 32-bits host longword to a big endian longword.
 *
 * @param [in] l A 32-bits longword in host byte ordering.
 *
 * @return The corresponding 32-bits longword in big endian byte order.
 *
 * @see frombig32()
 */
inline uint32_t tobig32(uint32_t l)
{
    if (ENDIAN_NATIVE != ENDIAN_BIG)
        l = byteswap32(l);

    return l;
}

/**
 * @brief Convert a 64-bits host quadword to a big endian quadword.
 *
 * @param [in] q A 64-bits quadword in host byte ordering.
 *
 * @return The corresponding 64-bits quadword in big endian byte order.
 *
 * @see frombig64()
 */
inline uint64_t tobig64(uint64_t q)
{
    if (ENDIAN_NATIVE != ENDIAN_BIG)
        q = byteswap64(q);

    return q;
}

/** @} */

#endif

