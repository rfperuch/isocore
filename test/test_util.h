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

#ifndef ISOLARIO_TEST_UTIL_H_
#define ISOLARIO_TEST_UTIL_H_

#include <CUnit/CUnit.h>
#include <stdio.h>
#include <string.h>

#define CU_ASSERT_VERBOSE(cond, line, file, func, fatal, fmt, ...) \
    do { \
        char buf__[snprintf(NULL, 0, fmt, __VA_ARGS__) + strlen(#cond) + 3]; \
        int off__ = sprintf(buf__, "%s: ", #cond); \
        sprintf(buf__ + off__, fmt, __VA_ARGS__); \
        CU_assertImplementation((cond), line, buf__, file, func, fatal); \
    } while (0)


#define CU_ASSERT_EX(cond, fmt, ...) \
    CU_ASSERT_VERBOSE(cond, __LINE__, __FILE__, __func__, 0, fmt, __VA_ARGS__)


#define CU_ASSERT_STRING_EQUAL_EX(r, s, fmt, ...) \
    CU_ASSERT_VERBOSE(strcmp(r, s) == 0, __LINE__, __FILE__, __func__, 0, fmt, __VA_ARGS__)

#endif
