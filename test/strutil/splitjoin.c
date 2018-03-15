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
#include <isolario/strutil.h>
#include <isolario/util.h>
#include <stdlib.h>

#include "test.h"

void testjoinstrv(void)
{
    char *joined = joinstrv(" ", "a", "fine", "sunny", "day", (char *) NULL);
    CU_ASSERT_STRING_EQUAL(joined, "a fine sunny day");
    free(joined);

    joined = joinstrv(" not ", "this is", "funny", (char *) NULL);
    CU_ASSERT_STRING_EQUAL(joined, "this is not funny");
    free(joined);

    joined = joinstrv(" ", (char *) NULL);
    CU_ASSERT_STRING_EQUAL(joined, "");
    free(joined);

    joined = joinstrv(" ", "trivial", (char *) NULL);
    CU_ASSERT_STRING_EQUAL(joined, "trivial");
    free(joined);

    joined = joinstrv("", "no", " changes", " to", " be", " seen", " here", (char *) NULL);
    CU_ASSERT_STRING_EQUAL(joined, "no changes to be seen here");
    free(joined);

    joined = joinstrv(NULL, "no", " changes", " here", " either", (char *) NULL);
    CU_ASSERT_STRING_EQUAL(joined, "no changes here either");
    free(joined);
}

void testsplitjoinstr(void)
{
    const struct {
        const char *input;
        const char *delim;
        size_t n;
        const char *expected[16];
    } table[] = {
        {
            "a whitespace separated string",
            " ", 4,
            { "a", "whitespace", "separated", "string" }
        }, {
            "",
            NULL, 0
        }, {
            "",
            "", 0
        }
    };
    for (size_t i = 0; i < nelems(table); i++) {
        size_t n;
        char **s = splitstr(table[i].input, table[i].delim, &n);

        CU_ASSERT_EQUAL_FATAL(n, table[i].n);
        for (size_t j = 0; j < n; j++)
            CU_ASSERT_STRING_EQUAL(s[j], table[i].expected[j]);

        CU_ASSERT_PTR_EQUAL(s[n], NULL);

        char *sj = joinstr(table[i].delim, s, n);
        CU_ASSERT_STRING_EQUAL(table[i].input, sj);

        free(s);
        free(sj);
    }
}

