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
 * @file isolario/funcattribs.h
 *
 * @brief Compiler-specific optimization and diagnostic hints.
 */

#ifndef ISOLARIO_FUNCATTRIBS_H_
#define ISOLARIO_FUNCATTRIBS_H_

/**
 * @defgroup Funcattribs Compiler-specific optimization and diagnostic hints
 *
 * @brief Function diagnostic and optimizing attributes.
 *
 * These additional attributes are intended to decorate function declarations with
 * additional optimization and diagnostic hints, enabling the compiler to operate
 * more aggressive optimizations and checks on code.
 *
 * @warning Placing wrong hints in a function declaration may produce unexpected bugs within
 *          the compiled code, use these hints only when you know what you are doing!
 *
 * @see [The GCC manual](https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#Common-Function-Attributes)
 *
 * @{
 */

/**
 * @def sentinel(idx)
 *
 * @brief Check for the presence of a \a NULL sentinel in a variadic function.
 *
 * This function attribute ensures that a parameter in a function call is an explicit \a NULL.
 * The attribute is only valid on variadic functions.
 * When \a idx is 0, the sentinel must be present in the last parameter of the function call.
 * Any other integer position signifies that the sentinel must be located at position \a idx
 * counting backwards from the end of the argument list.
 *
 * A valid \a NULL in this context is defined as zero with any pointer type.
 * If the \a NULL macro is defined with an integer type then you need to add an explicit cast.
 * GCC replaces \a stddef.h with a copy that redefines \a NULL appropriately.
 * In GCC, the warnings for missing or incorrect sentinels are enabled with \a -Wformat.
 */

/**
 * @def malloclike
 *
 * @brief Mark function as malloc()-like.
 *
 * This tells the compiler that a function is malloc()-like,
 * i.e., that the pointer returned by the function cannot alias any other pointer
 * valid when the function returns, and moreover pointers to valid objects occur
 * in any storage addressed by it.
 *
 * Using this attribute can improve optimization.
 * Functions like malloc() and calloc() have this property
 * because they return a pointer to uninitialized or zeroed-out storage.
 * However, functions like realloc() do not have this property, as they can return a pointer to storage containing pointers.
 */

/**
 * @def purefunc
 *
 * @brief Mark function as pure.
 *
 * Many functions have no effects except the return value and their return value depends
 * only on the parameters and/or global variables.
 * Calls to such functions can be subject to common subexpression elimination and loop optimization just as an arithmetic operator would be.
 * These functions should be declared with the attribute pure.
 * For example:
 *
 * @code{.c}
 *     purefunc int square(int);
 * @endcode
 *
 * says that the hypothetical function square is safe to call fewer times than the program says.
 * Some common examples of pure functions are strlen() or memcmp().
 * Interesting non-pure functions are functions with infinite loops or those depending on volatile
 * memory or other system resource, that may change between two consecutive calls (such as feof in a multithreading environment).
 *
 * The pure attribute imposes similar but looser restrictions on a function’s defintion than the const attribute:
 * it allows the function to read global variables. Decorating the same function with both the pure and the
 * const attribute is diagnosed.
 * Because a pure function cannot have any side effects it does not make sense for such a function to return void.
 * Declaring such a function is diagnosed.
 */

/**
 * @def constfunc
 *
 * @brief Mark function as const.
 *
 * Many functions do not examine any values except their arguments,
 * and have no effects except to return a value.
 * Calls to such functions lend themselves to optimization such as common
 * subexpression elimination. The const attribute imposes greater restrictions on a function’s definition than
 * the similar pure attribute because it prohibits the function from reading global variables.
 * Consequently, the presence of the attribute on a function declaration allows GCC to emit more efficient code for some calls to the function.
 * Decorating the same function with both the const and the pure attribute is diagnosed.
 *
 * @note A function that has pointer arguments and examines the data pointed to must not be declared const.
 * Likewise, a function that calls a non-const function usually must not be const.
 * Because a const function cannot have any side effects it does not make sense for such a function to return void.
 * Declaring such a function is diagnosed.
 */

#ifdef __GNUC__

#define printflike(fmtidx, argsidx) __attribute__((format(printf, fmtidx, argsidx)))
#define scanflike(fmtidx, argsidx)  __attribute__((format(scanf, fmtidx, argsidx)))
#define sentinel(idx)               __attribute__((sentinel(idx)))
#define nonnull(...)                __attribute__((nonnull(__VA_ARGS__)))
#define malloclike                  __attribute__((malloc))
#define nonullreturn                __attribute__((returns_nonnull))
#define purefunc                    __attribute__((pure))
#define constfunc                   __attribute__((const))
#define wur                         __attribute__((warn_unused_result))

#else

#define printflike(fmtidx, argsidx)
#define scanflike(fmtidx, argsidx)
#define sentinel(idx)
#define nonnull(...)
#define malloclike
#define nonullreturn
#define purefunc
#define constfunc
#define wur

#endif

/** @} */

#endif
