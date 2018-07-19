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
 * @file isolario/progutil.h
 *
 * @brief General utilities for command line tools.
 *
 * @note This header is guaranteed to include standard \a stdarg.h and \a stdnoreturn.h,
 *       it may include other standard, POSIX or isolario-specific headers, but includers
 *       should not rely on it.
 */

#ifndef ISOLARIO_PROGUTIL_H_
#define ISOLARIO_PROGUTIL_H_

#include <isolario/funcattribs.h>
#include <stdarg.h>
#include <stdnoreturn.h>

/**
 * @brief External variable referencing the name of this program.
 *
 * @note This variable is initially \a NULL, and is only set by setprogramnam().
 */
extern const char *programnam;

/**
 * @brief Initialize the \a programnam variable.
 *
 * Extracts the program name from the first command line argument
 * (\a argv[0]), saving it to \a programnam.
 * Functions in this group use that value to format diagnostic messages.
 *
 * @param [in] argv0 This argument should always be the main() function
 *                   \a argv[0] value, it is used to retrieve the name
 *                   of this program, it must not be \a NULL.
 *
 * @note This function may alter the contents of \a argv0, and
 *       \a programnam shall reference a substring of this argument,
 *       hence the program should rely no more on the contents of the
 *       argument string.
 */
nonnull(1) void setprogramnam(char *argv0);

nonnull(1) void evprintf(const char *fmt, va_list va);

printflike(1, 2) nonnull(1) void eprintf(const char *fmt, ...);

noreturn nonnull(2) void exvprintf(int code, const char *fmt, va_list va);

noreturn printflike(2, 3) nonnull(2) void exprintf(int code, const char *fmt, ...);

#endif
