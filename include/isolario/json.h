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

#ifndef ISOLARIO_JSON_H_
#define ISOLARIO_JSON_H_

#include <sys/types.h>  // for ssize_t and size_t

enum {
    JSON_NONE = '\0',

    JSON_BOOL = 'b',
    JSON_NUM  = 'f',
    JSON_STR  = 's',
    JSON_ARR  = '[',
    JSON_OBJ  = '{'
};

enum {
    JSON_BUFSIZ = 128
};

enum {
    JSON_END = -1,
    JSON_SUCCESS,
    JSON_INCOMPLETE,
    JSON_BAD_SYNTAX,
    JSON_NOMEM
};

typedef struct {
    int type;  ///< Current element types.
    union {
        double numval;   ///< For \a JSON_NUM, number value.
        int    boolval;  ///< For \a JSON_BOOL, boolean value.
        int    size;     ///< For \a JSON_OBJ and JSON_ARR, elements count.
    };
    char *start, *end, *next;
} jsontok_t;

typedef struct {
    ssize_t len, cap;
    char text[];
} json_t;

// Encoding ====================================================================

json_t *jsonalloc(size_t n);

inline int jsonerror(json_t *json)
{
    return json->len < 0;
}

inline void jsonclear(json_t *json)
{
    json->len = 0;
    json->text[0] = '\0';
}

json_t *jsonensure(json_t **pjson, size_t n);

void newjsonobj(json_t **pjson);

void newjsonarr(json_t **pjson);

void newjsonfield(json_t **pjson, const char *name);

void newjsonvals(json_t **pjson, const char *val);

void newjsonvalu(json_t **pjson, unsigned long val);

void newjsonvald(json_t **pjson, long val);

void newjsonvalf(json_t **pjson, double val);

void newjsonvalb(json_t **pjson, int boolean);

void closejsonarr(json_t **pjson);

void closejsonobj(json_t **pjson);

// Pretty-print ================================================================

int jsonprettyp(json_t **pjson, const char *restrict text);

// Parsing =====================================================================

int jsonparse(const char *text, jsontok_t *tok);

#endif

