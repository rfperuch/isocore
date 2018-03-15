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

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <isolario/bgpattribs.h>
#include <isolario/strutil.h>
#include <isolario/util.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

extern uint32_t getv4addrglobal(ex_community_t ecomm);

extern uint64_t getopaquevalue(ex_community_t ecomm);

extern int bgpattrcode(const void *buf);

extern int bgpattrflags(const void *buf);

extern int isbgpattrext(const void *buf);

extern size_t bgpattrhdrsize(const void *buf);

extern size_t bgpattrlen(const void *buf);

extern int getbgporigin(const void *buf);

extern void *makebgporigin(void *buf, int flags, int origin);

extern void *makeaspath(void *buf, int flags);

extern void *makeas4path(void *buf, int flags);

extern uint32_t getoriginatorid(const void *buf);

extern void *makeoriginatorid(void *buf, int flags, uint32_t id);

extern void *makeatomicaggregate(void *buf, int flags);

extern struct in_addr getnexthop(const void *buf);

extern void *makenexthop(void *buf, int flags, struct in_addr in);

extern uint32_t getmultiexitdisc(const void *buf);

extern void *makemultiexitdisc(void *buf, int flags, uint32_t disc);

extern uint32_t getlocalpref(const void *buf);

extern void *makelocalpref(void *buf, int flags, uint32_t pref);

void *stobgporigin(void *buf, int flags, const char *s)
{
    if (strcasecmp(s, "i") == 0 || strcasecmp(s, "igp") == 0)
        return makebgporigin(buf, flags, ORIGIN_IGP);
    else if (strcasecmp(s, "e") == 0 || strcasecmp(s, "egp") == 0)
        return makebgporigin(buf, flags, ORIGIN_EGP);
    else if (strcmp(s, "?") == 0 || strcasecmp(s, "incomplete") == 0)
        return makebgporigin(buf, flags, ORIGIN_INCOMPLETE);
    else
        return NULL;
}


void *putasseg32(void *buf, int seg_type, const uint32_t *seg, size_t count)
{
    assert(bgpattrcode(buf) == AS_PATH_CODE || bgpattrcode(buf) == AS4_PATH_CODE);

    unsigned char *ptr = buf;
    int extended = isbgpattrext(buf);

    ptr += ATTR_LENGTH_OFFSET;

    unsigned char *lenptr = ptr;

    size_t len = *ptr++;
    size_t max = ATTR_LENGTH_MAX;
    if (extended) {
        len <<= 8;
        len |= *ptr++;

        max = ATTR_EXTENDED_LENGTH_MAX;
    }

    size_t size = count * sizeof(*seg);

    size += AS_SEGMENT_HEADER_SIZE;
    if (unlikely(len + size > max))
        return NULL;  // would overflow attribute length
    if (unlikely(count > AS_SEGMENT_COUNT_MAX))
        return NULL;  // would overflow segment size

    ptr += len;
    *ptr++ = seg_type;  // segment type
    *ptr++ = count;     // AS count
    for (size_t i = 0; i < count; i++) {
        uint32_t as32 = tobig32(*seg++);
        memcpy(ptr, &as32, sizeof(as32));
        ptr += sizeof(as32);
    }

    // write updated length
    len += size;
    if (extended) {
        *lenptr++ = (len >> 8);
        len &= 0xff;
    }
    *lenptr++ = len;
    return buf;
}

void *putasseg16(void *buf, int type, const uint16_t *seg, size_t count)
{
    assert(bgpattrcode(buf) == AS_PATH_CODE);

    unsigned char *ptr = buf;
    int extended = isbgpattrext(buf);

    ptr += ATTR_LENGTH_OFFSET;

    unsigned char *lenptr = ptr;

    size_t len = *ptr++;
    size_t max = ATTR_LENGTH_MAX;
    if (extended) {
        len <<= 8;
        len |= *ptr++;

        max = ATTR_EXTENDED_LENGTH_MAX;
    }

    size_t size = count * sizeof(*seg);

    size += AS_SEGMENT_HEADER_SIZE;
    if (unlikely(len + size > max))
        return NULL;  // would overflow attribute length
    if (unlikely(count > AS_SEGMENT_COUNT_MAX))
        return NULL;  // would overflow segment size

    ptr += len;
    *ptr++ = type;   // segment type
    *ptr++ = count;  // AS count
    for (size_t i = 0; i < count; i++) {
        uint16_t as16 = tobig16(*seg++);
        memcpy(ptr, &as16, sizeof(as16));
        ptr += sizeof(as16);
    }

    // write updated length
    len += size;
    if (extended) {
        *lenptr++ = (len >> 8);
        len &= 0xff;
    }
    *lenptr++ = len;
    return buf;
}

static void appendsegment(void *buf, size_t *poff, size_t n, int type, const void *ases, size_t as_size, size_t count)
{
    if (count == 0)
        return;

    size_t size = AS_SEGMENT_HEADER_SIZE + count * as_size;
    size_t off = *poff;

    if (off + size <= n) {
        if (as_size == sizeof(uint32_t))
            putasseg32(buf, type, ases, count);
        else
            putasseg16(buf, type, ases, count);
    }

    off += size;
    *poff = off;
}

enum {
    PARSE_AS_NONE = -1,
    PARSE_AS_ERR  = -2
};

static long long parseas(const char *s, size_t as_size, char **eptr)
{
        unsigned long long as_max = (1ull << as_size * CHAR_BIT) - 1;

        int err = errno;  // don't pollute errno

        errno = 0;

        long long val = strtoll(s, eptr, 10);
        if (s == *eptr) {
            // this is not an error per se,
            // we just made it to the end of the parsable string
            errno = err;  // some POSIX set this to EINVAL
            return PARSE_AS_NONE;
        }
        if (val < 0 || (unsigned long long) val > as_max)
            errno = ERANGE; // range error to us
        if (errno != 0)
            return PARSE_AS_ERR;

        errno = err;  // all good, restore errno
        return val;
}

static void parseasset(void *buf, size_t *poff, size_t n, size_t as_size, const char *s, char **eptr)
{
    union {
        uint32_t as32[AS_SEGMENT_COUNT_MAX];
        uint16_t as16[AS_SEGMENT_COUNT_MAX];
    } segbuf;

    size_t count = 0;
    size_t off = *poff;
    bool commas = false;

    char *epos;
    while (true) {
        while (isspace(*s)) s++;

        char c = *s++;
        if (c == '}') {
            s++;
            break;
        }

        if (c == ',') {
            commas |= (count == 1);   // enable commas only after first AS
            if (!commas) {
                // unexpected comma
                errno = EINVAL;
                goto out;
            }

            s++;
            while (isspace(*s)) s++;

            if (*s == '\0') {
                // unexpected end of string
                errno = EINVAL;
                goto out;
            }

        } else if (commas) {
            // missing comma
            errno = EINVAL;
            goto out;
        }

        if (count == AS_SEGMENT_COUNT_MAX) {
            errno = ERANGE;
            goto out;
        }

        long long as = parseas(s, as_size, &epos);
        if (as == PARSE_AS_ERR)
            goto out;
        if (as == PARSE_AS_NONE)
            break;

        if (as_size == sizeof(uint32_t))
            segbuf.as32[count++] = as;
        else
            segbuf.as16[count++] = as;

        s = epos;
    }

    if (count == 0) {
        errno = EINVAL;
        goto out;
    }

    // output segment
    appendsegment(buf, &off, n, AS_SEGMENT_SEQ, &segbuf, as_size, count);

out:
    *poff = off;
    *eptr = (char *) s;
}

static void parseaspath(void *buf, size_t *poff, size_t n, const char *s, size_t as_size, char **eptr)
{
    union {
        uint32_t as32[AS_SEGMENT_COUNT_MAX];
        uint16_t as16[AS_SEGMENT_COUNT_MAX];
    } segbuf;

    size_t count = 0;
    size_t off = *poff;

    char *epos;
    while (true) {

        while (isspace(*s)) s++;

        if (*s == '\0')
            break;

        if (*s == '{') {
            // AS set segment
            s++;

            // append SEQ segment so far, NOP if count == 0
            appendsegment(buf, &off, n, AS_SEGMENT_SEQ, &segbuf, as_size, count);
            count = 0;

            // go on parsing the SET segment
            parseasset(buf, &off, n, as_size, s, &epos);
        } else {
            // additional AS sequence segment entry
            if (count == AS_SEGMENT_COUNT_MAX) {
                errno = ERANGE;
                goto out;
            }

            long long as = parseas(s, as_size, &epos);
            if (as == PARSE_AS_ERR)
                goto out;
            if (as == PARSE_AS_NONE)
                break;

            if (as_size == sizeof(uint32_t))
                segbuf.as32[count++] = as;
            else
                segbuf.as16[count++] = as;
        }

        s = epos;
    }

    // flush last sequence
    appendsegment(buf, &off, n, AS_SEGMENT_SEQ, &segbuf, as_size, count);

out:
    *eptr = (char *) s;
    *poff = off;
}

size_t stoaspath32(void *buf, size_t n, int flags, const char *s, char **eptr)
{
    char *dummy;
    size_t off = 0;

    if (!eptr)
        eptr = &dummy;

    size_t hdrsize = ATTR_HEADER_SIZE;
    if (flags & ATTR_EXTENDED_LENGTH)
        hdrsize = ATTR_EXTENDED_HEADER_SIZE;

    if (n >= hdrsize)
        makeaspath(buf, flags);

    off += hdrsize;
    parseaspath(buf, &off, n, s, sizeof(uint32_t), eptr);
    return off;
}

size_t stoaspath16(void *buf, size_t n, int flags, const char *s, char **eptr)
{
    char *dummy;
    size_t off = 0;

    if (!eptr)
        eptr = &dummy;

    size_t hdrsize = ATTR_HEADER_SIZE;
    if (flags & ATTR_EXTENDED_LENGTH)
        hdrsize = ATTR_EXTENDED_HEADER_SIZE;

    if (n >= hdrsize)
        makeaspath(buf, flags);

    off += hdrsize;
    parseaspath(buf, &off, n, s, sizeof(uint16_t), eptr);
    return off;
}

size_t stoas4path(void *buf, size_t n, int flags, const char *s, char **eptr)
{
    char *dummy;
    size_t off = 0;

    if (!eptr)
        eptr = &dummy;

    size_t hdrsize = ATTR_HEADER_SIZE;
    if (flags & ATTR_EXTENDED_LENGTH)
        hdrsize = ATTR_EXTENDED_HEADER_SIZE;

    if (n >= hdrsize)
        makeas4path(buf, flags);

    off += hdrsize;
    parseaspath(buf, &off, n, s, sizeof(uint32_t), eptr);
    return off;
}

extern void *makecommunity(void *attr, int flags);

static void *appendcommunities(void *attr, const void *comms, size_t n)
{
    int flags = bgpattrflags(attr);
    size_t len = bgpattrlen(attr);

    unsigned char *ptr = attr;
    ptr += ATTR_LENGTH_OFFSET;
    unsigned char *lenptr = ptr;

    size_t len_max = ATTR_LENGTH_MAX;
    if (flags & ATTR_EXTENDED_LENGTH) {
        len_max = ATTR_EXTENDED_LENGTH_MAX;
        ptr++;  // skip extended length
    }

    ptr++;  // skip length

    if (unlikely(len + n > len_max))
        return NULL;

    memcpy(ptr, comms, n);  // TODO byte-swap?
    len += n;
    if (flags & ATTR_EXTENDED_LENGTH) {
        *lenptr++ = len >> 8;
        len &= 0xff;
    }

    *lenptr++ = len;
    return attr;
}

void *putcommunities(void *attr, const community_t *comms, size_t count)
{
    unsigned char *ptr = attr;

    ptr += bgpattrhdrsize(attr);
    return attr;

}

static const struct {
    const char *str;
    community_t community;
} str2wellknown[] = {
    {"PLANNED_SHUT", COMMUNITY_PLANNED_SHUT},
    {"ACCEPT_OWN_NEXTHOP", COMMUNITY_ACCEPT_OWN_NEXTHOP},  // NOTE: must come BEFORE "ACCEPT_OWN", see communitytos()
    {"ACCEPT_OWN", COMMUNITY_ACCEPT_OWN},
    {"ROUTE_FILTER_TRANSLATED_V4", COMMUNITY_ROUTE_FILTER_TRANSLATED_V4},
    {"ROUTE_FILTER_V4", COMMUNITY_ROUTE_FILTER_V4},
    {"ROUTE_FILTER_TRANSLATED_V6", COMMUNITY_ROUTE_FILTER_TRANSLATED_V6},
    {"ROUTE_FILTER_V6", COMMUNITY_ROUTE_FILTER_V6},
    {"LLGR_STALE", COMMUNITY_LLGR_STALE},
    {"NO_LLGR", COMMUNITY_NO_LLGR},
    {"BLACKHOLE", COMMUNITY_BLACKHOLE},
    {"NO_EXPORT_SUBCONFED", COMMUNITY_NO_EXPORT_SUBCONFED},  // NOTE: must come BEFORE "NO_EXPORT", see communitytos()
    {"NO_EXPORT", COMMUNITY_NO_EXPORT} ,
    {"NO_ADVERTISE", COMMUNITY_NO_ADVERTISE},
    {"NO_PEER", COMMUNITY_NO_PEER}
};

char *communitytos(community_t c)
{
    static _Thread_local char buf[digsof(uint32_t) + 1];

    for (size_t i = 0; i < nelems(str2wellknown); i++) {
        if (str2wellknown[i].community == c)
            return (char *) str2wellknown[i].str;
    }

    sprintf(buf, "%lu", (unsigned long) c);
    return buf;
}

char *largecommunitytos(large_community_t c)
{
    static _Thread_local char buf[3 * digsof(uint32_t) + 2 + 1];

    sprintf(buf, "%" PRIu32 ":%" PRIu32 ":%" PRIu32, c.global, c.hilocal, c.lolocal);
    return buf;
}

static uint32_t parsecommfield(const char *s, char **eptr)
{
    // don't accept sign before field
    const char *ptr = s;
    while (isspace(*ptr)) ptr++;

    if (!isdigit(*ptr)) {
        // no conversion, invalid community
        *eptr = (char *) s;
        return 0;
    }
    if (*ptr == '0') {
        // RFC only allows exactly one 0 for community values
        *eptr = (char *) (ptr + 1);
        return 0;
    }

    unsigned long long val = strtoull(ptr, eptr, 10);
    if (val > UINT32_MAX) {
        errno = ERANGE;
        return UINT32_MAX;
    }

    // since isdigit(*ptr) is true, a conversion certainly took place,
    // and we don't need to restore errno, since EINVAL can't happen

    return (uint32_t) val;
}

community_t stocommunity(const char *s, char **eptr)
{
    for (size_t i = 0; i < nelems(str2wellknown); i++) {
        if (startswith(s, str2wellknown[i].str)) {
            if (eptr)
                *eptr = (char *) (s + strlen(str2wellknown[i].str));

            return str2wellknown[i].community;
        }
    }

    char *epos;
    community_t c = parsecommfield(s, &epos);

    if (eptr)
        *eptr = epos;

    return c;
}

large_community_t stolargecommunity(const char *s, char **eptr)
{
    const large_community_t zero = {0};

    large_community_t comm;

    const char *ptr = s;
    uint32_t *dst = &comm.global;

    char *epos;
    for (int i = 0; i < 3; i++) {
        *dst++ = parsecommfield(ptr, &epos);
        if (ptr == epos) {
            // no conversion happened, restore position and bail out
            epos = (char *) s;
            comm = zero;
            break;
        }

        ptr = epos;
        if (i != 2 && *ptr != ':') {
            // bad format, restore position and bail out
            epos = (char *) s;
            comm = zero;
            break;
        }

        ptr++;
    }

    if (eptr)
        *eptr = epos;

    return comm;
}

/* TODO
ex_community_v6_t stoexcommunityv6(const char *s, char **eptr)
{
}
*/

