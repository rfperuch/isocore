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

#include <ctype.h>
#include <errno.h>
#include <isolario/branch.h>
#include <isolario/parse.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *name;
    unsigned int lineno;

    char buf[TOK_LEN_MAX + 1];
    char unget[TOK_LEN_MAX + 1];

    parse_err_callback_t err;
} parser_t;

static _Thread_local parser_t parser = { NULL };

void parsingerr(const char *msg, ...)
{
    // don't use VLA!!!
    // parser.err() might longjmp() causing leaks!
    if (parser.err) {
        char buf[4096];
        va_list vl;

        va_start(vl, msg);
        vsnprintf(buf, sizeof(buf), msg, vl);
        va_end(vl);

        // always report line 0 if session is unavailable
        unsigned int lineno = (parser.name) ? parser.lineno : 0;
        parser.err(parser.name, lineno, buf);
    }
}

parse_err_callback_t setperrcallback(parse_err_callback_t cb)
{
    parse_err_callback_t old = parser.err;

    parser.err = cb;
    return old;
}

void startparsing(const char *name, unsigned int start_line)
{
    if (start_line < 1)
        start_line = 1;  // line 0 makes little sense...

    parser.name = name;
    parser.lineno = start_line;
}

void skiptonextline(FILE *f)
{
    parser.unget[0] = '\0';  // forget any unget token

    int c;
    do c = getc_unlocked(f); while (c != '\n' && c != EOF);

    parser.lineno++;
}

char *parse(FILE *f)
{
    if (parser.unget[0] != '\0') {
        strcpy(parser.buf, parser.unget);
        parser.unget[0] = '\0';
        return parser.buf;
    }

    int c;
    do {
        c = getc_unlocked(f);

        // skip to newline in case of comment
        if (c == '#')
            skiptonextline(f);
        if (c == '\n' || c == EOF)
            parser.lineno++;

    } while (isspace(c) || c == '\0');

    int i = 0;
    ungetc(c, f);
    while (true) {
        c = getc_unlocked(f);
        if (isspace(c) || c == EOF || c == '\0' || c == '#') {
            ungetc(c, f);
            break;
        }
        if (unlikely(i == TOK_LEN_MAX)) {
            parsingerr("'%.*s'...: token too long", i, parser.buf);
            ungetc(c, f);
            break;
        }

        parser.buf[i++] = c;
    }

    parser.buf[i] = '\0';
    return (i > 0) ? parser.buf : NULL;
}

void ungettoken(const char *tok)
{
    if (likely(tok))
        strcpy(parser.unget, tok);
}

char *expecttoken(FILE *f, const char *what)
{
    char *tok = parse(f);
    if (!tok) {
        parsingerr("unexpected end of parse");
        return NULL;
    } else if (what && strcmp(tok, what) != 0) {
        parsingerr("expecting '%s', got '%s'", what, tok);
        return NULL;
    } else {
        return tok;
    }
}

int iexpecttoken(FILE *f)
{
    char *tok = expecttoken(f, NULL);
    char *eptr;

    if (!tok)
        return 0;

    errno = 0;
    long v = strtol(tok, &eptr, 0);
    if (tok == eptr || *eptr != '\0') {
        parsingerr("got '%s', but integer value expected", tok);
        return 0;
    }

    // handle out of range integers
    if (unlikely(v < INT_MIN || v > INT_MAX))
        errno = ERANGE;

    if (errno != 0) {
        parsingerr("got '%s': %s", tok, strerror(errno));
        return 0;
    } else {
        return (int) v;
    }
}

double fexpecttoken(FILE *f)
{
    char *tok = expecttoken(f, NULL);
    char *eptr;

    if (!tok)
        return 0.0;

    errno = 0;
    double v = strtod(tok, &eptr);
    if (tok == eptr || *eptr != '\0') {
        parsingerr("got '%s', but floating point value expected", tok);
        return 0.0;
    } else if (errno != 0) {
        parsingerr("'%s': %s", tok, strerror(errno));
        return 0.0;
    } else {
        return v;
    }
}
