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

#include <isolario/branch.h>
#include <isolario/strutil.h>
#include <stdbool.h>
#include <stdlib.h>

extern unsigned long djb2(const char *s);

extern unsigned long memdjb2(const void *p, size_t n);

extern unsigned long sdbm(const char *s);

extern unsigned long memsdbm(const void *p, size_t n);

extern char *itoa(char *dst, char **end, int i);

extern char *utoa(char *dst, char **end, unsigned int u);

extern char *ltoa(char *dst, char **end, long l);

extern char *ultoa(char *dst, char **end, unsigned long u);

extern char *lltoa(char *dst, char **end, long long ll);

extern char *ulltoa(char *dst, char **end, unsigned long long u);

static size_t substrcnt(const char *s, const char *substr, size_t *pn)
{
    if (unlikely(!substr))
        substr = "";

    size_t n = 0;
    size_t count = 0;

    const char *ptr = s;
    size_t step = strlen(substr);
    while (*ptr != '\0') {
        const char *tok = strstr(ptr, substr);
        if (!tok) {
            n += strlen(ptr);
            break;
        }

        count++;

        n += tok - ptr;
        ptr = tok + step;
    }

    if (likely(pn))
        *pn = n;

    return count;
}

char **splitstr(const char *s, const char *delim, size_t *pn)
{
    if (unlikely(!delim))
        delim = "";

    size_t size;
    size_t n = substrcnt(s, delim, &size);

    char **res = malloc((n + 2) * sizeof(*res) + size + n + 1);
    if (unlikely(!res))
        return NULL;

    char *buf = (char *) (res + n + 2);
    const char *ptr = s;
    size_t step = strlen(delim);

    size_t i = 0;
    while (*ptr != '\0') {
        const char *tok = strstr(ptr, delim);
        size_t len = (tok) ? (size_t) (tok - ptr) : strlen(ptr);

        memcpy(buf, ptr, len);

        res[i++] = buf;
        buf += len;

        *buf++ = '\0';
        if (!tok)
            break;

        ptr = tok + step;
    }

    res[i] = NULL;
    if (pn)
        *pn = i;

    return res;
}

char *joinstr(const char *delim, char **strings, size_t n)
{
    if (unlikely(!delim))
        delim = "";

    size_t counts[n];
    size_t dlen = strlen(delim);
    size_t bufsiz = 0;
    for (size_t i = 0; i < n; i++) {
        size_t len = strlen(strings[i]);
        counts[i] = len;
        bufsiz += len;
    }

    bufsiz += dlen * (n - 1);

    char *buf = malloc(bufsiz + 1);
    if (unlikely(!buf))
        return NULL;

    char *ptr = buf;
    char *end = buf + bufsiz;
    for (size_t i = 0; i < n; i++) {
        size_t len = counts[i];
        memcpy(ptr, strings[i], len);
        ptr += len;
        if (ptr < end) {
            memcpy(ptr, delim, dlen);
            ptr += dlen;
        }
    }

    *ptr = '\0';
    return buf;
}

char *joinstrvl(const char *delim, va_list va)
{
    if (unlikely(!delim))
        delim = "";

    va_list vc;
    size_t bufsiz = 0;
    size_t n      = 0;
    size_t dlen   = strlen(delim);

    va_copy(vc, va);
    while (true) {
        const char *s = va_arg(vc, const char *);
        if (!s)
            break;

        bufsiz += strlen(s);
        n++;
    }
    va_end(vc);

    if (likely(n > 0))
        bufsiz += dlen * (n - 1);

    char *buf = malloc(bufsiz + 1);
    if (unlikely(!buf))
        return NULL;

    char *ptr = buf;
    char *end = buf + bufsiz;
    for (size_t i = 0; i < n; i++) {
        const char *s = va_arg(va, const char *);
        size_t n = strlen(s);

        memcpy(ptr, s, n);
        ptr += n;
        if (ptr < end) {
            memcpy(ptr, delim, dlen);
            ptr += dlen;
        }
    }

    *ptr = '\0';
    return buf;
}

char *joinstrv(const char *delim, ...)
{
    va_list va;

    va_start(va, delim);
    char *s = joinstrvl(delim, va);
    va_end(va);
    return s;
}

char *trimwhites(char *s)
{
    char *start = s;
    while (isspace(*start)) start++;

    if (unlikely(*start == '\0')) {
        *s = '\0';
        return s;
    }

    size_t n = strlen(start);
    char *end = start + n - 1;
    while (isspace(*end)) end--;

    n = (end - start) + 1;
    memmove(s, start, n);
    s[n] = '\0';
    return s;
}

char *strpathext(const char *name)
{
    const char *ext = NULL;

    int c;
    while ((c = *name) != '\0') {
       if (c == '/')
           ext = NULL;
       if (c == '.')
           ext = name;

        name++;
    }
    if (!ext)
        ext = name;

    return (char *) ext;
}

extern int startswith(const char *s, const char *prefix);

extern int endswith(const char *s, const char *suffix);

extern char *strupper(char *s);

extern char *strlower(char *s);

