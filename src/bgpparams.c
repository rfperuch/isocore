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

#include <isolario/bgpparams.h>

// extern declarations for inline functions in isolario/bgpparams.h

extern uint32_t getasn32bit(const bgpcap_t *cap);

extern bgpcap_t *setasn32bit(bgpcap_t *cap, uint32_t as);

extern void *setmultiprotocol(bgpcap_t *buf, afi_t afi, safi_t safi);

extern afi_safi_t getmultiprotocol(const bgpcap_t *cap);

extern bgpcap_t *setgracefulrestart(bgpcap_t *cap, int flags, int secs);

extern bgpcap_t *putgracefulrestarttuple(bgpcap_t *cap, afi_t afi, safi_t safi, int flags);

extern int getgracefulrestarttime(const bgpcap_t *cap);

extern int getgracefulrestartflags(const bgpcap_t *cap);

size_t getgracefulrestarttuples(afi_safi_t *dst, size_t n, const bgpcap_t *cap)
{
    assert(cap->code == GRACEFUL_RESTART_CODE);

    const afi_safi_t *src = (const afi_safi_t *) (&cap->data[GRACEFUL_RESTART_TUPLES_OFFSET]);

    // XXX: signal non-zeroed flags as an error or mask them?
    size_t size = cap->len - GRACEFUL_RESTART_TUPLES_OFFSET;
    size /= sizeof(*src);
    if (n > size)
        n = size;

    // copy and swap bytes
    for (size_t i = 0; i < n; i++) {
        dst[i].afi   = frombig16(src[i].afi);
        dst[i].safi  = src[i].safi;
        dst[i].flags = src[i].flags;
    }
    return size;
}

