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
#include <isolario/hexdump.h>
#include <isolario/util.h>
#include <test_util.h>

#include "test.h"

void testhexdump(void)
{
    const struct {
        unsigned char input[256];
        size_t input_size;
        const char* format;
        const char* expected;
    } table[] = {
        {{0x40, 0x01, 0x01, 0x01}, 4, "x", "40010101"},
        {{0x40, 0x01, 0x01, 0x01}, 4, "x#{1}", "{ 0x40, 0x01, 0x01, 0x01 }"},
        {{0x40, 0x01, 0x01, 0x01}, 4, "x# 1", "0x40 0x01 0x01 0x01"},
        {{0x40, 0x01, 0x01, 0x01}, 4, "x# 1 9", "0x40 0x01\n0x01 0x01"},
        {{0x40, 0x01, 0x01, 0x01}, 4, "x#|1", "0x40 | 0x01 | 0x01 | 0x01"},
        {{0x40, 0x01, 0x01, 0x01}, 4, "x 1", "40 01 01 01"},
        {{0x40, 0x01, 0x01, 0x01}, 4, "x|1", "40 | 01 | 01 | 01"},
        {{0x40, 0x01, 0x01, 0x01}, 4, "b", "01000000000000010000000100000001"},
        {{0x40, 0x01, 0x01, 0x01}, 4, "b# 1", "b01000000 b00000001 b00000001 b00000001"},
        {{0x40, 0x01, 0x01, 0x01}, 4, BINARY_PLAIN, "01000000, 00000001, 00000001, 00000001"},
        {{0x40, 0x01, 0x01, 0x01}, 4, HEX_C_ARRAY, "{ 0x40, 0x01, 0x01, 0x01 }"},
        {{0x40, 0x01, 0x01, 0x01}, 4, HEX_PLAIN, "0x40, 0x01, 0x01, 0x01"},
    };
    
    for (size_t i = 0; i < nelems(table); i++) {
        char output[256];
        size_t n = hexdumps(output, sizeof(output), table[i].input, table[i].input_size, table[i].format);
        CU_ASSERT_EX(strlen(table[i].expected)+1 == n, "with i = %zu", i);
        CU_ASSERT_STRING_EQUAL_EX(output, table[i].expected,
                                  "with i = %zu: \"%s\" != \"%s\"",
                                  i, output, table[i].expected);
    }
    
}

