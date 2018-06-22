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
 * @file isolario/strutil.h
 *
 * @brief String utility functions.
 *
 * @note This file is guaranteed to include \a stdarg.h and \a stddef.h,
 *       it may include additional standard headers in the interest of providing
 *       inline implementations of its functions.
 */

#ifndef ISOLARIO_STRUTILS_H_
#define ISOLARIO_STRUTILS_H_

#include <ctype.h>
#include <isolario/util.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

inline unsigned long djb2(const char *s)
{
    unsigned long h = 5381;
    int c;

    const unsigned char *ptr = (const unsigned char *) s;
    while ((c = *ptr++) != '\0')
        h = ((h << 5) + h) + c; // hash * 33 + c

    return h;
}

inline unsigned long memdjb2(const void *p, size_t n)
{
    unsigned long h = 5381;

    const unsigned char *ptr = p;
    for (size_t i = 0; i < n; i++)
        h = ((h << 5) + h) + *ptr++; // hash * 33 + c

    return h;
}

inline unsigned long sdbm(const char *s)
{
    unsigned long h = 0;
    int c;

    const unsigned char *ptr = (const unsigned char *) s;
    while ((c = *ptr++) != '\0')
        h = c + (h << 6) + (h << 16) - h;

    return h;
}

inline unsigned long memsdbm(const void *p, size_t n)
{
    unsigned long h = 0;

    const unsigned char *ptr = p;
    for (size_t i = 0; i < n; i++)
        h = *ptr++ + (h << 6) + (h << 16) - h;

    return h;
}

inline char *itoa(char *dst, char **endp, int i)
{
    char buf[1 + digsof(i) + 1];

    char *ptr = buf + sizeof(buf);

    int val = abs(i);

    *--ptr = '\0';

    char *end = ptr;
    do {
        *--ptr = "0123456789"[val % 10];
        val /= 10;
    } while (val > 0);

    if (i < 0)
        *--ptr = '-';

    size_t n = end - ptr;
    if (endp)
        *endp = &dst[n];

    return memcpy(dst, ptr, n + 1);
}

inline char *utoa(char *dst, char **endp, unsigned int u)
{
    char buf[digsof(u) + 1];

    char *ptr = buf + sizeof(buf);

    *--ptr = '\0';

    char *end = ptr;
    do {
        *--ptr = "0123456789"[u % 10];
        u /= 10;
    } while (u > 0);

    size_t n = end - ptr;
    if (endp)
        *endp = &dst[n];

    return memcpy(dst, ptr, n + 1);
}


inline char *ltoa(char *dst, char **endp, long l)
{
    char buf[1 + digsof(l) + 1];

    char *ptr = buf + sizeof(buf);

    long val = labs(l);

    *--ptr = '\0';

    char *end = ptr;
    do {
        *--ptr = "0123456789"[val % 10];
        val /= 10;
    } while (val > 0);

    if (l < 0)
        *--ptr = '-';

    size_t n = end - ptr;
    if (endp)
        *endp = &dst[n];

    return memcpy(dst, ptr, n + 1);
}

inline char *ultoa(char *dst, char **endp, unsigned long u)
{
    char buf[digsof(u) + 1];

    char *ptr = buf + sizeof(buf);

    *--ptr = '\0';

    char *end = ptr;
    do {
        *--ptr = "0123456789"[u % 10];
        u /= 10;
    } while (u > 0);

    size_t n = end - ptr;
    if (endp)
        *endp = &dst[n];

    return memcpy(dst, ptr, n + 1);
}

inline char *lltoa(char *dst, char **endp, long long ll)
{
    char buf[1 + digsof(ll) + 1];

    char *ptr = buf + sizeof(buf);

    long long val = llabs(ll);

    *--ptr = '\0';

    char *end = ptr;
    do {
        *--ptr = "0123456789"[val % 10];
        val /= 10;
    } while (val > 0);

    if (ll < 0)
        *--ptr = '-';

    size_t n = end - ptr;
    if (endp)
        *endp = &dst[n];

    return memcpy(dst, ptr, n + 1);
}

inline char *ulltoa(char *dst, char **endp, unsigned long long u)
{
    char buf[digsof(u) + 1];

    char *ptr = buf + sizeof(buf);

    *--ptr = '\0';

    char *end = ptr;
    do {
        *--ptr = "0123456789"[u % 10];
        u /= 10;
    } while (u > 0);

    size_t n = end - ptr;
    if (endp)
        *endp = &dst[n];

    return memcpy(dst, ptr, n + 1);
}

/**
 * @brief Split string against a delimiter.
 *
 * Returns a buffer of strings splitted against a delimiter string.
 * When such buffer is no longer useful, it must be free()d.
 *
 * @param [in]  s     String to be splitted into substrings, may not be \a NULL.
 * @param [in]  delim Delimiter string for each substring, may be \a NULL.
 * @param [out] pn    A pointer to an unsigned variable where the length of
 *                    the returned buffer is to be stored, the terminating
 *                    \a NULL pointer stored in the buffer is not accounted
 *                    for in the returned value. This argument may be \a NULL
 *                    if such information is unimportant for the caller.
 *
 * @return A dynamically allocated and \a NULL-terminated buffer of substrings
 *         obtained by repeatedly splitting \a str against the delimiter string
 *         \a delim. The returned buffer must be free()d by the caller.
 *
 * @note The caller must only free() the returned pointer, each string
 *       in the buffer share the same memory and is consequently free()d by
 *       such call.
 */
char **splitstr(const char *s, const char *delim, size_t *pn);

/**
 * @brief Join a string buffer on a delimiter.
 *
 * @return A dynamically allocated string that must be free()d by the caller.
 */
char *joinstr(const char *delim, char **strings, size_t n);

/**
 * @brief Join a \a va_list of strings on a delimiter.
 *
 * @return A dynamically allocated string that must be free()d by the caller.
 */
char *joinstrvl(const char *delim, va_list va);

/**
 * @brief Join a variable list of strings on a delimiter.
 *
 * This function expects a variable number of \a const \a char pointers
 * and joins them on a delimiter.
 *
 * @param [in] ... A variable number of \a const \a char pointers, this list
 *                 *must* be terminated by a \a NULL \a char pointer, as in:
 *                 @verbatim (char *) NULL @endverbatim.
 *
 * @return A dynamically allocated string that must be free()d by the caller.
 */
char *joinstrv(const char *delim, ...);

/// @brief Trim leading and trailing whitespaces from string, operates in-place.
char *trimwhites(char *s);

/// @brief Return file extension, including leading '.', \a NULL if no extension is found.
char *strpathext(const char *name);

/// @brief Check whether a string starts with a specific prefix.
inline int startswith(const char *s, const char *prefix)
{
    size_t slen = strlen(s);
    size_t preflen = strlen(prefix);
    return (slen >= preflen) && memcmp(s, prefix, preflen) == 0;
}

/// @brief Check whether a string ends with a specific suffix.
inline int endswith(const char *s, const char *suffix)
{
    size_t slen = strlen(s);
    size_t suflen = strlen(suffix);
    return (slen >= suflen) && memcmp(s + slen - suflen, suffix, suflen) == 0;
}

/// @brief Make string uppercase, operates in-place.
inline char *strupper(char *s)
{
    char c;

    char *ptr = s;
    while ((c = *ptr) != '\0')
        *ptr++ = toupper((unsigned char) c);

    return s;
}

/// @brief Make string lowercase, operates in-place.
inline char *strlower(char *s)
{
    char c;

    char *ptr = s;
    while ((c = *ptr) != '\0')
        *ptr++ = tolower((unsigned char) c);

    return s;
}

#endif
