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
#include <isolario/bgpattribs.h>
#include <isolario/util.h>
#include <errno.h>
#include <string.h>
#include "test.h"

void testcommunityconv(void)
{
    static const struct {
        const char *str;
        community_t expect;
    } comm2str[] = {
        {"PLANNED_SHUT",               COMMUNITY_PLANNED_SHUT},
        {"ROUTE_FILTER_TRANSLATED_V6", COMMUNITY_ROUTE_FILTER_TRANSLATED_V6},
        {"ROUTE_FILTER_TRANSLATED_V4", COMMUNITY_ROUTE_FILTER_TRANSLATED_V4},
        {"ROUTE_FILTER_V6",            COMMUNITY_ROUTE_FILTER_V6},
        {"ROUTE_FILTER_V4",            COMMUNITY_ROUTE_FILTER_V4},
        {"LLGR_STALE",                 COMMUNITY_LLGR_STALE},
        {"ACCEPT_OWN",                 COMMUNITY_ACCEPT_OWN},
        {"NO_LLGR",                    COMMUNITY_NO_LLGR},
        {"BLACKHOLE",                  COMMUNITY_BLACKHOLE},
        {"NO_EXPORT_SUBCONFED",        COMMUNITY_NO_EXPORT_SUBCONFED},
        {"NO_EXPORT",                  COMMUNITY_NO_EXPORT},
        {"NO_ADVERTISE",               COMMUNITY_NO_ADVERTISE},
        {"ACCEPT_OWN_NEXTHOP",         COMMUNITY_ACCEPT_OWN_NEXTHOP},
        {"NO_PEER",                    COMMUNITY_NO_PEER},

        {"0",          0},
        {"4294967295", UINT32_MAX},
        {"12345",      12345}
    };

    errno = 0;

    char *eptr;
    for (size_t i = 0; i < nelems(comm2str); i++) {
        community_t c = stocommunity(comm2str[i].str, &eptr);
        CU_ASSERT_EQUAL(errno, 0);
        CU_ASSERT_EQUAL(*eptr, '\0');
        CU_ASSERT_EQUAL(c, comm2str[i].expect);
        CU_ASSERT_STRING_EQUAL(communitytos(c), comm2str[i].str);
    }
}

void testlargecommunityconv(void)
{
    static const struct {
        const char *str;
        large_community_t expect;
    } largecomm2str[] = {
        {"0:0:0", {0, 0, 0}},
        {
            "4294967295:4294967295:4294967295",
            {UINT32_MAX, UINT32_MAX, UINT32_MAX}
        },
        {"123:456:789", {123, 456, 789}}
    };

    char *eptr;
    for (size_t i = 0; i < nelems(largecomm2str); i++) {
        large_community_t c = stolargecommunity(largecomm2str[i].str, &eptr);

        CU_ASSERT_EQUAL(*eptr, '\0');
        CU_ASSERT(memcmp(&c, &largecomm2str[i].expect, sizeof(c)) == 0);
        CU_ASSERT_STRING_EQUAL(largecommunitytos(c), largecomm2str[i].str);
    }
}

void testaspathconv(void)
{

/* FIXME
    const char *s = "10 20 30 40 50 {10 20 30} {20, 10, 0} 50 1 2";
    char *eptr;

    unsigned char buf[200];
    size_t n = stoaspath16(buf, sizeof(buf), 0, s, &eptr);
    printf("would need: %zd, now got: %zd\n", n, bgpattrhdrsize(buf) + bgpattrlen(buf));
    printf("eptr = \"%s\", errno: %s", eptr, strerror(errno));
    CU_ASSERT(n <= sizeof(buf));
    CU_ASSERT(*eptr == '\0');
*/
}

