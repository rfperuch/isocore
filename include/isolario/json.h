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

#include <stddef.h>
#include <sys/types.h>

typedef struct {
    ssize_t len, cap;
    char text[];
} json_t;

json_t *jsonalloc(size_t n);

inline int jsonerror(json_t *json)
{
    return json->len < 0;
}

void jsonensure(json_t **pjson, size_t n);

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

// TODO: decoder and void jsonprettyp(json_t **pjson);

#endif

