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

#include <locale.h>
#include <stdlib.h>

#include "bench.h"

int main(void)
{
    setlocale(LC_ALL, "");

    if (cbench_initialize() != CB_SUCCESS)
        return EXIT_FAILURE;

    cbench_suite_t *suite = cbench_add_suite("strutil");
    if (!suite)
        goto out;

    if (!cbench_add_bench(suite, "bcommsprintf", bcommsprintf, NULL))
        goto out;

    if (!cbench_add_bench(suite, "bsprintf", bsprintf, NULL))
        goto out;

    if (!cbench_add_bench(suite, "bcommulltoa", bcommulltoa, NULL))
        goto out;

    if (!cbench_add_bench(suite, "ulltoa", bulltoa, NULL))
        goto out;

    if (!cbench_add_bench(suite, "splitstr", bsplit, NULL))
        goto out;

    if (!cbench_add_bench(suite, "joinstrv", bjoinv, NULL))
        goto out;

    if (!cbench_add_bench(suite, "joinstr", bjoin, NULL))
        goto out;

    cbench_run();

out:
    cbench_cleanup();
    return cbench_get_error();
}

