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

#include <CUnit/Basic.h>
#include <stdlib.h>
#include "test.h"

int main(void)
{
    if (CU_initialize_registry() != CUE_SUCCESS)
        return CU_get_error();

    CU_pSuite suite = CU_add_suite("core", NULL, NULL);
    if (!suite)
        goto error;

    if (!CU_add_test(suite, "simple uint128_t iteration", testu128iter))
        goto error;

    if (!CU_add_test(suite, "uint128_t to string and string to uint128_t conversion", testu128conv))
        goto error;

    if (!CU_add_test(suite, "simple hexdump", testhexdump))
        goto error;
        
    if (!CU_add_test(suite, "test log", testlog))
        goto error;
    
    if (!CU_add_test(suite, "test for splitstr() and joinstr()", testsplitjoinstr))
        goto error;

    if (!CU_add_test(suite, "test for joinstrv()", testjoinstrv))
        goto error;

    if (!CU_add_test(suite, "test for trimwhites()", testtrimwhites))
        goto error;

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    unsigned int num_failures = CU_get_number_of_failures();
    CU_cleanup_registry();
    return (num_failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;

error:
    CU_cleanup_registry();
    return CU_get_error();
}
