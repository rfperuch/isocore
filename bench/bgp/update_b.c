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

#include <isolario/bgp.h>
#include <isolario/bgpattribs.h>
#include <isolario/util.h>
#include "bench.h"

void bupdategen(cbench_state_t *state)
{
    unsigned char buf[256];

    bgpattr_t *attr = (bgpattr_t *) buf;

    const uint32_t asseq[] = { 1, 2, 3, 4, 5, 6, 7, 9, 11 };
    const uint32_t asset[] = { 22, 0x11111, 93495 };

    while (cbench_next_iteration(state)) {
        setbgpwrite(BGP_UPDATE);

        startbgpattribs();
        {
            attr->code  = ORIGIN_CODE;
            attr->flags = DEFAULT_ORIGIN_FLAGS;
            attr->len   = ORIGIN_LENGTH;
            setorigin(attr, ORIGIN_IGP);
            putbgpattrib(attr);

            attr->code  = AS_PATH_CODE;
            attr->flags = DEFAULT_AS_PATH_FLAGS;
            attr->len   = 0;
            putasseg32(attr, AS_SEGMENT_SEQ, asseq, nelems(asseq));
            putasseg32(attr, AS_SEGMENT_SET, asset, nelems(asset));
            putbgpattrib(attr);
        }
        endbgpattribs();
        bgpfinish(NULL);
        bgpclose();
    }
}

