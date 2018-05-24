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

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <isolario/branch.h>
#include <isolario/json.h>
#include <isolario/util.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    JSON_INITSIZ = 128,
    JSON_GROWSTEP = 64,
    JSON_LEVELS_MAX = 8,
    JSON_INDENT_SPACES = 3
};

/*
static int json_flags = 0;
static char json_seps[JSON_LEVELS_MAX];
static int indent_level = 0;

static void indent(char sep, int level)
{
    if (sep == '\0')
        return;

    if ((json_flags & JSON_PRETTY_PRINT) == 0) {
        // print only non-space separators and return
        if (!isspace(sep))
            fputc(sep, json_output);

        return;
    }

    fputc(sep, json_output);

    // apply spacing to particular separators
    switch (sep) {
    case ':':
        fputc(' ', json_output);
        break;
    case ',':
        // go to newline and indent
        fputc('\n', json_output);

        // fallthrough
    case '\n':
        // indent line
        for (int i = 0; i < level; i++) {
            for(int j = 0; j < JSON_INDENT_SPACES; j++)
                fputc(' ', json_output);
        }

        break;

    default:
        break;
    }
}
*/

static size_t escapestring(char *restrict dst, const char *restrict src)
{
    static const char escapechar[CHAR_MAX + 1] = {
        ['"']  = '"',
        ['\\'] = '\\',
        ['/']  = '/',  // allows embedding JSON in a <script>
        ['\b'] = 'b',
        ['\f'] = 'f',
        ['\n'] = 'n',
        ['\r'] = 'r',
        ['\t'] = 't',
        ['\v'] = 'n'   // paranoid, remap \v to \n, incorrect but still better than nothing
    };

    // TODO escape chars < ' ' as '\u000'hex(c)

    int c;

    char *ptr = dst;
    while ((c = *src++) != '\0') {
        char e = escapechar[c];
        if (e != '\0') {
            *ptr++ = '\\';
            c = e;
        }

        *ptr++ = c;
    }

    *ptr = '\0';
    return ptr - dst;
}

static int valueneedscomma(json_t *json)
{
    ssize_t i;
    for (i = json->len - 1; i >= 0; i--) {
        if (!isspace(json->text[i]))
            break;
    }

    return i >= 0 && json->text[i] != ':' && json->text[i] != '[';
}

static int isfirstfield(json_t *json)
{
    ssize_t i;
    for (i = json->len - 1; i >= 0; i--) {
        if (!isspace(json->text[i]))
            break;
    }

    return i >= 0 && json->text[i] == '{';
}

json_t *jsonalloc(size_t n)
{
    if (n < JSON_INITSIZ)
        n = JSON_INITSIZ;

    json_t *json = malloc(sizeof(*json) + n);
    if (unlikely(!json))
        return NULL;

    json->len = 0;
    json->cap = n;
    json->text[0] = '\0';

    return json;
}

extern int jsonerror(json_t *json);

void jsonensure(json_t **pjson, size_t n)
{
    json_t *json = *pjson;
    if (unlikely(jsonerror(json)))
        return;  // JSON encoder had error, propagate

    if (json->len + (ssize_t) n >= json->cap) {
        n += json->len;
        n += JSON_GROWSTEP;
        n++;  // account for '\0'

        json = realloc(json, sizeof(*json) + n);
        if (unlikely(!json)) {
            (*pjson)->len = -1;
            return;
        }

        json->cap = n;
    }

    *pjson = json;
}

void newjsonobj(json_t **pjson)
{

    int comma = valueneedscomma(*pjson);

    jsonensure(pjson, comma + 1);

    json_t *json = *pjson;
    if (unlikely(jsonerror(json)))
        return;

    if (comma)
        json->text[json->len++] = ',';

    json->text[json->len++] = '{';
    json->text[json->len]   = '\0';
}

void newjsonarr(json_t **pjson)
{
    int comma = valueneedscomma(*pjson);

    jsonensure(pjson, comma + 1);

    json_t *json = *pjson;
    if (unlikely(jsonerror(json)))
        return;

    if (comma)
        json->text[json->len++] = ',';

    json->text[json->len++] = '[';
    json->text[json->len] = '\0';
}

void newjsonfield(json_t **pjson, const char *name)
{
    size_t n     = strlen(name);
    int comma    = !isfirstfield(*pjson);

    jsonensure(pjson, comma + n + 3);

    json_t *json = *pjson;
    if (unlikely(jsonerror(json)))
        return;

    if (comma)
        json->text[json->len++] = ',';

    json->text[json->len++] = '"';
    memcpy(&json->text[json->len], name, n);
    json->len += n;
    json->text[json->len++] = '"';
    json->text[json->len++] = ':';
    json->text[json->len]   = '\0';
}

void newjsonvals(json_t **pjson, const char *val)
{
    size_t n  = strlen(val);
    int comma = valueneedscomma(*pjson);

    char buf[n * 2 + 1];
    n = escapestring(buf, val);

    jsonensure(pjson, comma + n + 2);

    json_t *json = *pjson;
    if (unlikely(jsonerror(json)))
        return;

    if (comma)
        json->text[json->len++] = ',';

    json->text[json->len++] = '"';
    memcpy(&json->text[json->len], buf, n);
    json->len += n;
    json->text[json->len++] = '"';
    json->text[json->len]   = '\0';
}

void newjsonvalu(json_t **pjson, unsigned long val)
{
    char buf[digsof(val) + 1];

    int n = sprintf(buf, "%lu", val);
    int comma = valueneedscomma(*pjson);

    jsonensure(pjson, comma + n);

    json_t *json = *pjson;
    if (unlikely(jsonerror(json)))
        return;

    if (comma)
        json->text[json->len++] = ',';

    memcpy(&json->text[json->len], buf, n);
    json->len += n;

    json->text[json->len] = '\0';
}

void newjsonvald(json_t **pjson, long val)
{
    char buf[digsof(val) + 2];

    int n = sprintf(buf, "%ld", val);
    int comma = valueneedscomma(*pjson);

    jsonensure(pjson, comma + n);

    json_t *json = *pjson;
    if (unlikely(jsonerror(json)))
        return;

    if (comma)
        json->text[json->len++] = ',';

    memcpy(&json->text[json->len], buf, n);
    json->len += n;

    json->text[json->len] = '\0';
}

void newjsonvalf(json_t **pjson, double val)
{
    int n = snprintf(NULL, 0, "%f", val);
    int comma = valueneedscomma(*pjson);

    char buf[n + 1];
    n = sprintf(buf, "%g", val);

    jsonensure(pjson, comma + n);

    json_t *json = *pjson;
    if (unlikely(jsonerror(json)))
        return;

    if (comma)
        json->text[json->len++] = ',';

    memcpy(&json->text[json->len], buf, n);
    json->len += n;
    json->text[json->len] = '\0';
}

void newjsonvalb(json_t **pjson, int boolean)
{
    const char *val = (boolean) ? "true" : "false";
    size_t n        = strlen(val);
    int comma       = valueneedscomma(*pjson);

    jsonensure(pjson, n);

    json_t *json = *pjson;
    if (unlikely(jsonerror(json)))
        return;

    if (comma)
        json->text[json->len++] = ',';

    memcpy(&json->text[json->len], val, n);
    json->len += n;
    json->text[json->len] = '\0';
}

void closejsonarr(json_t **pjson)
{
    jsonensure(pjson, 1);

    json_t *json = *pjson;
    if (likely(!jsonerror(json))) {
        json->text[json->len++] = ']';
        json->text[json->len]   = '\0';
    }
}

void closejsonobj(json_t **pjson)
{
    jsonensure(pjson, 1);

    json_t *json = *pjson;
    if (likely(!jsonerror(json))) {
        json->text[json->len++] = '}';
        json->text[json->len]   = '\0';
    }
}

