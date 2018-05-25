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
#include <errno.h>
#include <isolario/branch.h>
#include <isolario/json.h>
#include <isolario/util.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    JSON_GROWSTEP = 64,
    JSON_INDENT_SPACES = 3
};

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
        if (!isspace((unsigned char) json->text[i]))
            break;
    }

    return i >= 0 && json->text[i] != ':' && json->text[i] != '[';
}

static int isfirstfield(json_t *json)
{
    ssize_t i;
    for (i = json->len - 1; i >= 0; i--) {
        if (!isspace((unsigned char) json->text[i]))
            break;
    }

    return i >= 0 && json->text[i] == '{';
}

json_t *jsonalloc(size_t n)
{
    if (n < JSON_BUFSIZ)
        n = JSON_BUFSIZ;

    json_t *json = malloc(sizeof(*json) + n);
    if (unlikely(!json))
        return NULL;

    json->len = 0;
    json->cap = n;
    json->text[0] = '\0';

    return json;
}

extern int jsonerror(json_t *json);

extern void jsonclear(json_t *json);

json_t *jsonensure(json_t **pjson, size_t n)
{
    json_t *json = *pjson;
    if (unlikely(jsonerror(json)))
        return NULL;  // JSON encoder had error, propagate

    if (json->len + (ssize_t) n >= json->cap) {
        n += json->len;
        n += JSON_GROWSTEP;
        n++;  // account for '\0'

        json = realloc(json, sizeof(*json) + n);
        if (unlikely(!json)) {
            (*pjson)->len = -1;
            return NULL;
        }

        json->cap = n;
    }

    *pjson = json;
    return json;
}

void newjsonobj(json_t **pjson)
{
    int comma = valueneedscomma(*pjson);

    json_t *json = jsonensure(pjson, comma + 1);
    if (unlikely(!json))
        return;

    if (comma)
        json->text[json->len++] = ',';

    json->text[json->len++] = '{';
    json->text[json->len]   = '\0';
}

void newjsonarr(json_t **pjson)
{
    int comma = valueneedscomma(*pjson);

    json_t *json = jsonensure(pjson, comma + 1);
    if (unlikely(!json))
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

    json_t *json = jsonensure(pjson, comma + n + 3);
    if (unlikely(!json))
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

    json_t *json = jsonensure(pjson, comma + n + 2);
    if (unlikely(!json))
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

    json_t *json = jsonensure(pjson, comma + n);
    if (unlikely(!json))
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

    json_t *json = jsonensure(pjson, comma + n);
    if (unlikely(!json))
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

    json_t *json = jsonensure(pjson, comma + n);
    if (unlikely(!json))
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

    json_t *json = jsonensure(pjson, comma + n);
    if (unlikely(!json))
        return;

    if (comma)
        json->text[json->len++] = ',';

    memcpy(&json->text[json->len], val, n);
    json->len += n;
    json->text[json->len] = '\0';
}

void closejsonarr(json_t **pjson)
{
    json_t *json = jsonensure(pjson, 1);
    if (likely(json)) {
        json->text[json->len++] = ']';
        json->text[json->len]   = '\0';
    }
}

void closejsonobj(json_t **pjson)
{
    json_t *json = jsonensure(pjson, 1);
    if (likely(json)) {
        json->text[json->len++] = '}';
        json->text[json->len]   = '\0';
    }
}

static void indent(json_t **pjson, int level)
{
    level *= JSON_INDENT_SPACES;

    json_t *json = jsonensure(pjson, level);
    if (unlikely(!json))
        return;

    for (int i = 0; i < level; i++)
        json->text[json->len++] = ' ';
}

static int dump(json_t **pjson, const char *text, jsontok_t *tok, int level)
{
    int err = jsonparse(text, tok);
    if (err != JSON_SUCCESS)
        return err;

    json_t *json;
    int count;
    size_t n = tok->end - tok->start;
    int type = tok->type;
    switch (type) {
    case JSON_NUM:
    case JSON_BOOL:
        json = jsonensure(pjson, n);
        if (unlikely(!json))
            return JSON_NOMEM;

        memcpy(&json->text[json->len], tok->start, n);
        json->len += n;
        return JSON_SUCCESS;

    case JSON_STR:
        json = jsonensure(pjson, n + 2);
        if (unlikely(!json))
            return JSON_NOMEM;

        json->text[json->len++] = '"';
        memcpy(&json->text[json->len], tok->start, n);
        json->len += n;
        json->text[json->len++] = '"';
        return JSON_SUCCESS;

    case JSON_OBJ:
    case JSON_ARR:
        json = jsonensure(pjson, 2);
        if (unlikely(!json))
            return JSON_NOMEM;

        json->text[json->len++] = (type == JSON_OBJ) ? '{' : '[';
        json->text[json->len++] = '\n';

        level++;

        count = tok->size;
        for (int i = 0; i < count; i++) {
            indent(pjson, level);
            err = dump(pjson, text, tok, level);
            if (err != JSON_SUCCESS)
                return err;

            if (type == JSON_OBJ) {
                json = jsonensure(pjson, 2);
                if (unlikely(!json))
                    return JSON_NOMEM;

                json->text[json->len++] = ':';
                json->text[json->len++] = ' ';
                err = dump(pjson, text, tok, level);
                if (err != JSON_SUCCESS)
                    return err;
            }

            n = 1;
            if (i + 1 != count)
                n++;

            json = jsonensure(pjson, n);
            if (unlikely(!json))
                return JSON_NOMEM;

            if (i + 1 != count)
                json->text[json->len++] = ',';

            json->text[json->len++] = '\n';
        }

        level--;
        indent(pjson, level);

        json = jsonensure(pjson, 1);
        json->text[json->len++] = (type == JSON_OBJ) ? '}' : ']';
        json->text[json->len]   = '\0';
        return JSON_SUCCESS;

    default:
        assert(false);
        return JSON_BAD_SYNTAX; // should never happen
    }
}

int jsonprettyp(json_t **pjson, const char *restrict text)
{
    jsonclear(*pjson);

    jsontok_t tok;
    memset(&tok, 0, sizeof(tok));
    return dump(pjson, text, &tok, 0);
}

static int jsonnum(char *text, jsontok_t *tok)
{
    errno = 0;

    char *start = text;
    char *end   = text;

    double d = strtod(start, &end);
    if (errno != 0 || start == end)
        return JSON_BAD_SYNTAX;

    tok->type   = JSON_NUM;
    tok->numval = d;
    tok->start  = start;
    tok->end    = end;
    tok->next   = end;
    return JSON_SUCCESS;
}

static int jsonstring(char *text, jsontok_t *tok)
{
    tok->type = JSON_STR;
    tok->start  = text;

    int c;
    while ((c = *text) != '\0') {
        // FIXME should check escape sequences validity
        if (c == '"' && text[-1] != '\\')
            break;

        text++;
    }

    if (c == '\0')
        return JSON_INCOMPLETE;

    tok->end  = text;  // do not include quotes
    tok->next = text + 1;
    return JSON_SUCCESS;
}

enum {
    ALLOW_PRIMITIVES = 1 << 0,
    ALLOW_PUNCT      = 1 << 1,
    ALLOW_END        = 1 << 2
};

static int jsonparserec(char *text, jsontok_t *tok, int flags);

static int jsonparseaggregate(char *text, jsontok_t *tok, int type)
{
    jsontok_t tmp;
    memset(&tmp, 0, sizeof(tmp));
    tmp.next = text;

    int closing = (type == JSON_OBJ) ? '}' : ']';

    int count = 0;
    char *start = text;
    while (true) {
        int err = jsonparserec(tmp.next, &tmp, ALLOW_PRIMITIVES | ALLOW_PUNCT);
        if (err != JSON_SUCCESS)
            return err;

        if (tmp.type == '}' || tmp.type == ']')
            break;

        if (count > 0) {
            // any element other than the last must be followed by comma
            if (tmp.type != ',')
                return JSON_BAD_SYNTAX;

            err = jsonparserec(tmp.next, &tmp, ALLOW_PRIMITIVES);
            if (err != JSON_SUCCESS)
                return err;
        }

        if (type == JSON_OBJ) {
            // object, expected format: '"key":'
            if (tmp.type != JSON_STR)
                return JSON_BAD_SYNTAX;

            err = jsonparserec(tmp.next, &tmp, ALLOW_PUNCT);
            if (err != JSON_SUCCESS)
                return err;
            if (tmp.type != ':')
                return JSON_BAD_SYNTAX;

            // value
            err = jsonparserec(tmp.next, &tmp, ALLOW_PRIMITIVES);
            if (err != JSON_SUCCESS)
                return err;
        }
        if (tmp.type == JSON_OBJ || tmp.type == JSON_ARR)
            tmp.next = tmp.end + 1;  // skip nested aggregates

        count++;
    }

    if (tmp.type != closing)
        return JSON_BAD_SYNTAX;

    tok->type  = type;
    tok->size  = count;
    tok->start = start;
    tok->end   = tmp.start;
    tok->next  = start;  // start parsing the object itself
    return JSON_SUCCESS;
}


static int jsonparserec(char *text, jsontok_t *tok, int flags)
{
    int c;
    while (true) {
        c = (unsigned char) *text++;  // so we can use ctype.h safely
        if (c == '\0')
            return (flags & ALLOW_END) ? JSON_END : JSON_INCOMPLETE;

        if (!isspace(c))
            break;
    }

    switch (c) {
    case '{':  // object
        return jsonparseaggregate(text, tok, JSON_OBJ);

    case '[':  // array
        return jsonparseaggregate(text, tok, JSON_ARR);

    case '}':
    case ']':
    case ':':
    case ',':
        if ((flags & ALLOW_PUNCT) == 0)
            return JSON_BAD_SYNTAX;

        tok->type  = c;
        tok->start = text - 1;
        tok->end   = text;
        tok->next  = text;
        return JSON_SUCCESS;

    case '"':  // key or string
        if ((flags & ALLOW_PRIMITIVES) == 0)
            return JSON_BAD_SYNTAX;

        return jsonstring(text, tok);

    default:
        if ((flags & ALLOW_PRIMITIVES) == 0)
            return JSON_BAD_SYNTAX;

        // boolean or number
        text--;  // don't skip the first character!
        if (strncmp(text, "true", 4) == 0) {
            tok->type    = JSON_BOOL;
            tok->boolval = true;
            tok->start   = text;
            tok->end     = text + 4;
            tok->next    = text + 4;
            return JSON_SUCCESS;
        }
        if (strncmp(text, "false", 5) == 0) {
            tok->type    = JSON_BOOL;
            tok->boolval = false;
            tok->start   = text;
            tok->end     = text + 5;
            tok->next    = text + 5;
            return JSON_SUCCESS;
        }

        // assume number
        return jsonnum(text, tok);
    }
}

int jsonparse(const char *text, jsontok_t *tok)
{
    char *ptr = tok->next;
    int flags = ALLOW_PRIMITIVES | ALLOW_PUNCT | ALLOW_END;
    if (unlikely(!ptr)) {
        ptr = (char *) text;  // we won't modify, promise
        flags = ALLOW_END;    // first element must be object or array
    }

    int err;
    while ((err = jsonparserec(ptr, tok, flags)) == JSON_SUCCESS) {
        // since root element must be an object or array, if we get a
        // punctuation, we definitely can't fail the next parse
        // (since objects and arrays are syntax-checked on their first parse)
        int t = tok->type;
        if (t != ',' && t != ':' && t != '}' && t != ']')
            break;

        ptr = tok->next;
    }
    return err;
}

