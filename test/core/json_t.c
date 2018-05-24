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
#include <stdio.h>
#include <stdlib.h>

void testjsonsimple(void)
{
    json_t *json = jsonalloc(0);
    newjsonobj(&json);
    newjsonfield(&json, "makoto");
    newjsonvals(&json, "auuu");

    newjsonfield(&json, "empty");
    newjsonobj(&json);
    closejsonobj(&json);

    newjsonfield(&json, "obj");
    newjsonobj(&json);
    for (int i = 0; i < 10; i++) {
        char name[64];

        sprintf(name, "value%d", i);
        newjsonfield(&json, name);
        newjsonobj(&json);
        for (int j = 0; j < 10; j++) {
            sprintf(name, "field%d", j);
            newjsonfield(&json, name);
            newjsonvalf(&json, (double) j / 1000000.0);
        }
        closejsonobj(&json);
    }
    closejsonobj(&json);

    newjsonfield(&json, "arr");

    newjsonarr(&json);
    for (int i = 0; i < 4; i++) {
        newjsonarr(&json);
        for (int j = 0; j < 100; j++)
            newjsonvald(&json, j);

        closejsonarr(&json);
    }
    closejsonarr(&json);

    closejsonobj(&json);

    printf("%s\n", json->text);
    CU_ASSERT_STRING_EQUAL(json->text, "{\"makoto\":\"auuu\"}");

    free(json);
}

