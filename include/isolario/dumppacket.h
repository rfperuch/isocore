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

#ifndef ISOLARIO_DUMPPACKET_H_
#define ISOLARIO_DUMPPACKET_H_

#include <isolario/funcattribs.h>
#include <isolario/bgp.h>
#include <isolario/mrt.h>
#include <stdarg.h>
#include <stdio.h>

nonnull(1, 2, 3) void printbgpv_r(FILE *out, bgp_msg_t *pkt, const char *fmt, va_list va);

nonnull(1, 2, 3) void printbgp_r(FILE *out, bgp_msg_t *pkt, const char *fmt, ...);

nonnull(1, 2) void printbgpv(FILE *out, const char *fmt, va_list va);

nonnull(1, 2) void printbgp(FILE *out, const char *fmt, ...);

nonnull(1, 2, 3) void printpeerentv(FILE *out, const peer_entry_t *ent, const char *fmt, va_list va);

nonnull(1, 2, 3) void printpeerent(FILE *out, const peer_entry_t *ent, const char *fmt, ...);

nonnull(1, 2, 3) void printstatechangev(FILE *out, const bgp4mp_header_t *bgphdr, const char *fmt, va_list va);

nonnull(1, 2, 3) void printstatechange(FILE *out, const bgp4mp_header_t *bgphdr, const char *fmt, ...);

#endif
