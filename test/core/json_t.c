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

#include <CUnit/CUnit.h>
#include <isolario/json.h>
#include <isolario/util.h>
#include <stdio.h>
#include <stdlib.h>

static void jsonstreq(const jsontok_t *tok, const char *expect)
{
    CU_ASSERT_EQUAL_FATAL(tok->type, JSON_STR);
    size_t len = strlen(expect);
    CU_ASSERT_EQUAL_FATAL(tok->end - tok->start, len);
    CU_ASSERT_FATAL(strncmp(tok->start, expect, len) == 0);
}

void testjsonsimple(void)
{
    json_t *json = jsonalloc(JSON_BUFSIZ);
    newjsonobj(&json);
    newjsonfield(&json, "myString");
    newjsonvals(&json, "Hello, World!");

    newjsonfield(&json, "emptyObject");
    newjsonobj(&json);
    closejsonobj(&json);

    newjsonfield(&json, "myArray");
    newjsonarr(&json);
    for (int i = 0; i < 10; i++)
        newjsonvald(&json, i);

    closejsonarr(&json);
    closejsonobj(&json);

    const char *expected_json = "{\"myString\":\"Hello, World!\",\"emptyObject\":{},\"myArray\":[0,1,2,3,4,5,6,7,8,9]}";

    CU_ASSERT_FALSE_FATAL(jsonerror(json));
    CU_ASSERT_TRUE_FATAL(json->cap > json->len);
    CU_ASSERT_EQUAL_FATAL(strlen(expected_json), json->len);

    CU_ASSERT_STRING_EQUAL_FATAL(json->text, expected_json);

    jsontok_t tok;
    memset(&tok, 0, sizeof(tok));
    int err = jsonparse(json->text, &tok);
    CU_ASSERT_EQUAL_FATAL(err, JSON_SUCCESS);
    CU_ASSERT_EQUAL_FATAL(tok.type, JSON_OBJ);
    CU_ASSERT_EQUAL_FATAL(tok.size, 3);

    err = jsonparse(json->text, &tok);
    CU_ASSERT_EQUAL_FATAL(err, JSON_SUCCESS);
    jsonstreq(&tok, "myString");

    err = jsonparse(json->text, &tok);
    CU_ASSERT_EQUAL_FATAL(err, JSON_SUCCESS);
    jsonstreq(&tok, "Hello, World!");

    err = jsonparse(json->text, &tok);
    CU_ASSERT_EQUAL_FATAL(err, JSON_SUCCESS);
    jsonstreq(&tok, "emptyObject");

    err = jsonparse(json->text, &tok);
    CU_ASSERT_EQUAL_FATAL(err, JSON_SUCCESS);
    CU_ASSERT_EQUAL_FATAL(tok.type, JSON_OBJ);
    CU_ASSERT_EQUAL_FATAL(tok.size, 0);

    err = jsonparse(json->text, &tok);
    CU_ASSERT_EQUAL_FATAL(err, JSON_SUCCESS);
    jsonstreq(&tok, "myArray");

    err = jsonparse(json->text, &tok);
    CU_ASSERT_EQUAL_FATAL(err, JSON_SUCCESS);
    CU_ASSERT_EQUAL_FATAL(tok.type, JSON_ARR);
    CU_ASSERT_EQUAL_FATAL(tok.size, 10);
    for (int i = 0; i < 10; i++) {
        char buf[digsof(i) + 1];

        err = jsonparse(json->text, &tok);
        CU_ASSERT_EQUAL_FATAL(err, JSON_SUCCESS);
        CU_ASSERT_EQUAL_FATAL(tok.type, JSON_NUM);
        CU_ASSERT_EQUAL_FATAL(tok.numval, (double) i);

        int n1 = sprintf(buf, "%d", i);
        int n2 = tok.end - tok.start;
        CU_ASSERT_EQUAL_FATAL(n1, n2);
        CU_ASSERT_FATAL(strncmp(tok.start, buf, n1) == 0);
    }

    err = jsonparse(json->text, &tok);
    CU_ASSERT_EQUAL_FATAL(err, JSON_END);

    free(json);
}

