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
 * @file isolario/backtrace.h
 *
 * @brief Compiler specific backtrace utilities for debugging and signal handling.
 */

#ifndef ISOLARIO_BACKTRACE_H_
#define ISOLARIO_BACKTRACE_H_

#include <stdio.h>

/**
 * @defgroup Backtrace Compiler backtrace utilities
 *
 * @brief Functions to obtain a backtrace and human readable symbol names.
 *
 * This group includes a number of utility function to retrieve basic
 * information about backtrace (also known as stacktrace) at runtime.
 * This is especially useful for postmortem debugging and logging, as well
 * as collecting hints about possible causes of failure.
 * To achieve maximum generality, these functions in this group strive to
 * minimize dynamic allocation and generally take a best-effort approach,
 * the overall interface is generally low-level, still easy to use.
 * This interface has two main concepts:
 *
 * * A symbol that identifies a function in the backtrace, corresponding to a \a void pointer.
 * * A human readable representation of a symbol, identified as a \a char pointer.
 *
 * A backtrace is simply an array of symbols, sorted in a most-recent to least-recent
 * call fashion.
 * Since these functions strive to keep their footprint to a minimum, due to the
 * unpredictable conditions in which they might be called, static buffers and
 * stack allocations may be employed to try and reduce dynamically allocated
 * buffers in heap, as a consequence symbol names may be truncated or partial;
 * Debug informations may still be precious and significant though.
 * On unsupported compilers or compiling configurations that stripped away any
 * backtrace information, this interface is still usable and simply no information
 * is returned.
 *
 * @warning Functions in this group rely both on compiler and on the
 *          specific configuration used during compilation.
 *
 * @{
 */

/**
 * @brief Retrieve a backtrace into the specified buffer.
 *
 * The returned backtrace is sorted in most-recent to least-recent call,
 * so that the zero-entry in buffer is this function's caller.
 *
 * @param [out] buf Buffer in which the retrieved backtrace is to be
 *                  stored.
 * @param [in]  n   Buffer size, this function shall not write past the
 *                  specified buffer size.
 *
 * @return Count of symbols written to \a buf, which shall not exceed \a n.
 *         A zero return value indicates that no backtrace information is
 *         available.
 *
 * @warning A return value of exactly \a n may indicate a truncated
 *          backtrace information, unfortunately there is no reliable way
 *          of knowing how large the buffer argument should be beforehand.
 *
 * @see symname()
 */
size_t btrace(void **buf, size_t n);

/**
 * @example test/compiler/backtrace_iterate_example.c
 *
 * A simple example demonstrating how to use backtrace().
 */

/**
 * @brief Retrieve the symbol belonging to a specific caller.
 *
 * This function returns the symbol corresponding to a caller of the
 * calling function.
 *
 * @param [in] depth The depth of the caller whose symbol information is to
 *                   be obtained. A value of zero indicates the function
 *                   itself, a value of one indicates the caller of the
 *                   calling function, and so on...
 *                   The default argument shall try to obtain the information
 *                   regarding the caller of the calling function, which is
 *                   the most typical scenario.
 *
 * @return On success, the symbol belonging to the requested caller is returned,
 *         \a NULL is returned in case the information is unavailable, or
 *         such caller does not exist.
 *
 * @see symname()
 */
void *caller(int depth);

/**
 * @example test/compiler/backtrace_caller_example.c
 *
 * A simple example demonstrating how to use caller().
 */

/**
 * @brief Translate a symbol to human readable string
 *        (C++ name demangling is performed as needed).
 *
 * @param [in] sym A symbol obtained from btrace() or caller(),
 *                 may be \a NULL.
 *
 * @return Upon success, a NUL-terminated string containing a human-readable
 *         description of \a sym is returned, useful for debugging purposes.
 *         The returned buffer belongs to a thread local static buffer, hence
 *         it must not be freed.
 *         The returned buffer is valid until the next call of this function
 *         in the same thread.
 *         Upon failure \a NULL is returned, this usually indicates that
 *         no information is available.
 *
 * @see btrace()
 * @see caller()
 */
char *symname(void *sym);

/** @} */

#endif
