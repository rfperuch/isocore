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
 * @file isolario/util.h
 *
 * @brief General utility macros.
 *
 * @note This file is guaranteed to include \a limits.h, it may include
 *       additional standard files in order to implement its functionality.
 */

#ifndef ISOLARIO_UTIL_H_
#define ISOLARIO_UTIL_H_

#include <limits.h>

/**
 * @def stringify
 *
 * @brief Convert macro argument to string constant.
 *
 * Invokes the preprocessor to convert the provided argument to a string
 * constant, it may also be used with expressions such as:
 *
 * @code{.c}
 *    stringify(x == 0);  // produces: "x == 0"
 * @endcode
 *
 * @param [in] x Argument to be stringified.
 *
 * @return String constant containing the stringification results.
 */
#ifndef stringify
#define stringify(x) #x
#endif

/**
 * @def xstringify
 *
 * @brief Convert macro argument to string constant, expending macro arguments
 *        to their underlying values.
 *
 * Variant of stringify() that also expands macro arguments to their
 * underlying values before stringification.
 *
 * @param [in] x Argument to be stringified.
 *
 * @return String constant containing the stringification results.
 */
#ifndef xstringify
#define xstringify(x) stringify(x)
#endif

/**
 * @def min(a, b)
 *
 * @brief Compute minimum value between two arguments, by comparing them with
 *        \a <.
 *
 * @warning This macro is unsafe and may evaluate its arguments more than once.
 *          *Only provide arguments with no side-effects*.
 */
#if !defined(min) && !defined(__cplusplus)
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

/**
 * @def max(a, b)
 *
 * @brief Compute maximum value between two arguments, by comparing them with
 *        \a <.
 *
 * @warning This macro is unsafe and may evaluate its arguments more than once.
 *          *Only provide arguments with no side-effects*.
 */
#if !defined(max) && !defined(__cplusplus)
#define max(a, b) (((a) < (b)) ? (b) : (a))
#endif

/**
 * @def clamp(x, a, b)
 *
 * @brief Clamp value to a range (inclusive).
 *
 * Takes a value \a x and clamps it to the range defined by [\a a, \a b].
 * Behavior is undefined if \a a is less than \a b.
 *
 * @param [in] x Value to be clamped.
 * @param [in] a Range minimum value, inclusive.
 * @param [in] b Range maximum value, inclusive.
 *
 * @return Clamped value \a y, such that:
 *         * y = a iff. x <= a
 *         * y = b iff. x >= b
 *         * y = x otherwise
 *
 * @warning This macro is unsafe and may evaluate its arguments more than once.
 *          *Only provide arguments with no side-effects*.
 */
#if !defined(clamp) && !defined(__cplusplus)
#define clamp(x, a, b) max(a, min(x, b))
#endif

/**
 * @def nelems(array)
 *
 * @brief Number of elements in array.
 *
 * Compute the number of elements in an array as a \a size_t compile time
 * constant.
 *
 * @note This macro only computes the number of elements for *arrays*, this
 *       example code is not going to produce the expected result,
 *       as it operates on a pointer:
 *
 *       @code{.c}
 *           #include <stdio.h>
 *
 *           void wrong(const int arr[])
 *           {
 *               printf("%zu\n", nelems(arr));  // WRONG! arr is a pointer
 *           }
 *       @endcode
 *
 * @warning This macro is unsafe and may evaluate its arguments more than once.
 *          *Only provide arguments with no side-effects*.
 */
#ifndef nelems
#define nelems(array) (sizeof(array) / sizeof((array)[0]))
#endif

/**
 * @def digsof(typ)
 *
 * @brief Digits required to represent a type.
 *
 * Returns a compile time constant whose value is the number of digits
 * required to represent the largest value of a type, the returned value is an
 * upperbound, suitable to allocate buffers of char to store the string
 * representation in digits of that type.
 */
#ifndef digsof
#define digsof(typ) (sizeof(typ) * CHAR_BIT)
#endif

#endif
