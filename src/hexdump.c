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
#include <isolario/hexdump.h>
#include <isolario/util.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

// paranoid...
#if CHAR_BIT != 8
#error Code assumes 8-bit bytes
#endif

typedef struct {
    FILE *out;        ///< Output stream
    size_t actual;    ///< Data *successfully* wrote to \a out in bytes
    size_t wr;        ///< Data that should have been written to \a out, in bytes
    size_t nbytes;    ///< Consumed input bytes count
    size_t col;       ///< Current column
    size_t grouping;  ///< Byte grouping
    size_t cols;      ///< Desired column limit hint
    char sep;         ///< Separator character
    char format;      ///< Byte format
    char alter;       ///< Alternative format flag
} hexstate_t;

static int closingsep(int sep)
{
    switch (sep) {
    case '{':
        return '}';
    case '[':
        return ']';
    case '(':
        return ')';
    default:
        return sep;
    }
}

static int isparensep(int sep)
{
    return sep == '{' || sep == '[' || sep == '(';
}

static int ismodeformat(int c)
{
    return c == 'x' || c == 'X' || c == 'o' || c == 'O' || c == 'b' || c == 'B';
}

static int ismodesep(int c)
{
    return c == '{' || c == '[' || c == '(' || c == '|' ||
           c == '/' || c == ',' || c == ' ';
}

static size_t formatsize(const hexstate_t *state)
{
    size_t alter_overhead = 0;
    size_t size = 0;
    switch (state->format) {
    default:
    case 'x':
    case 'X':
        alter_overhead += 2;
        size += 2;
        break;
    case 'o':
    case 'O':
        alter_overhead++;
        size += 3;
        break;
    case 'b':
    case 'B':
        alter_overhead++;
        size += 8;
        break;
    }

    if (state->grouping != SIZE_MAX)
        size *= state->grouping;

    if (state->sep == '|')
        size += 2;
    if (state->sep == ',')
        size++;

    if (state->alter == '#')
        size += alter_overhead;

    return size;
}

static void out(hexstate_t *state, int c)
{
    if (fputc(c, state->out) != EOF)
        state->actual++;

    state->wr++;
    state->col++;
    if (c == '\n')
        state->col = 0;
}

static void openparen(hexstate_t *state)
{
    if (isparensep(state->sep)) {
        if (state->col >= state->cols)
            out(state, '\n');

        out(state, state->sep);
    }
}

static void closeparen(hexstate_t *state)
{
    if (isparensep(state->sep)) {
        // newline if space + paren would exceed column limit
        out(state, (state->col + 2 > state->cols) ? '\n' : ' ');
        out(state, closingsep(state->sep));
    }
}

static void putsep(hexstate_t *state)
{
    if (state->sep == '|') {
        out(state, ' ');
        out(state, '|');
    } else if (state->sep != ' ')
        out(state, ',');
}

static void putbyte(hexstate_t *state, int byt)
{
    const char *digs = "0123456789abcdef";

    switch (state->format) {
    case 'X':
        digs = "0123456789ABCDEF";
        // fallthrough
    case 'x':
    default:
        out(state, digs[byt >> 4]);
        out(state, digs[byt & 0xf]);
        break;
    case 'O':
    case 'o':
        out(state, digs[byt >> 6]);
        out(state, digs[(byt >> 3) & 0x3]);
        out(state, digs[byt & 0x3]);
        break;
    case 'B':
    case 'b':
        for (int i = 0; i < 8; i++) {
            out(state, digs[!!(byt & 0x80)]);
            byt <<= 1;
        }
        break;
    }

    state->nbytes++;
}

static void opengroup(hexstate_t *state)
{
    // newline if space + group would exceed column limit
    if (state->col > 0)
        out(state, (state->col + formatsize(state) + 1 > state->cols) ? '\n' : ' ');

    if (state->alter != '#')
        return;

    switch (state->format) {
    default:
    case 'x':
    case 'X':
        out(state, '0');
        // fallthrough
    case 'b':
    case 'B':
        out(state, state->format);
        break;
    case 'o':
    case 'O':
        out(state, '0');
        break;
    }
}

static void dohexdump(hexstate_t *state, const void *data, size_t n, const char *mode, va_list vl)
{
    // parse mode string
    if (!mode)
        mode = "";

    if (ismodeformat(*mode))
        state->format = *mode++;
    if (*mode == '#')
        state->alter = *mode++;
    if (ismodesep(*mode))
        state->sep = *mode++;

    if (isdigit(*mode) || *mode == '*') {
        if (*mode == '*') {
            int len = va_arg(vl, int);
            state->grouping = max(len, 0);
        } else {
            state->grouping = atoll(mode);
        }

        // NOTE: this does the right thing for valid mode strings, and
        // tolerates malformed strings such as: "/*40"
        do mode++; while (isdigit(*mode));
    }
    if (state->sep != '\0' && *mode == closingsep(state->sep)) {
        mode++;
        if (*mode == '*') {
            int len = va_arg(vl, int);
            state->cols = max(len, 0);
        } else if (isdigit(*mode)) {
            state->cols = atoll(mode);
        }
    }

    const unsigned char *ptr = data;
    const unsigned char *end = ptr + n;

    // normalize arguments and apply defaults
    if (state->grouping == 0)
        state->grouping = SIZE_MAX;
    if (state->cols == 0)
        state->cols = SIZE_MAX;
    if (state->sep == '\0')
        state->sep = ' ';
    if (state->sep == '/')
        state->sep = ',';
    if (state->format == '\0')
        state->format = 'x';

    // format bytes into output
    openparen(state);

    while (ptr < end) {
        if ((state->nbytes % state->grouping) == 0) {
            if (state->nbytes > 0)
                putsep(state);

            opengroup(state);
        }

        putbyte(state, *ptr++);
    }

    closeparen(state);
}

size_t hexdump(FILE *out, const void *data, size_t n, const char *mode, ...)
{
    hexstate_t state = { .out = out };

    va_list vl;

    va_start(vl, mode);
    dohexdump(&state, data, n, mode, vl);
    va_end(vl);

    return state.actual;
}

size_t hexdumps(char *dst, size_t size, const void *data, size_t n, const char *mode, ...)
{
    char dummy;
    if (size == 0) {
        // POSIX: fmemopen() may fail with a size of 0, give it a dummy buffer
        dst  = &dummy;
        size = sizeof(dummy);
    }

    FILE *f = fmemopen(dst, size, "w");
    if (!f)
        return 0;

    setbuf(f, NULL);

    hexstate_t state = { .out = f };

    va_list vl;
    va_start(vl, mode);
    dohexdump(&state, data, n, mode, vl);
    va_end(vl);

    fclose(f);
    return state.wr + 1;  // report we need an additional byte for '\0'
}

