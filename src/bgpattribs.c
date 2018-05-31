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

extern size_t getattrlenextended(const bgpattr_t *attr);

extern void setattrlenextended(bgpattr_t *attr, size_t len);

extern int getbgporigin(const bgpattr_t *attr);

extern bgpattr_t *setbgporigin(bgpattr_t *attr, int origin);

extern uint32_t getoriginatorid(const bgpattr_t *attr);

extern bgpattr_t *setoriginatorid(bgpattr_t *attr, uint32_t id);

extern struct in_addr getnexthop(const bgpattr_t *attr);

extern bgpattr_t *setnexthop(bgpattr_t *attr, struct in_addr in);

extern uint32_t getmultiexitdisc(const bgpattr_t *attr);

extern bgpattr_t *setmultiexitdisc(bgpattr_t *attr, uint32_t disc);

extern uint32_t getlocalpref(const bgpattr_t *attr);

extern bgpattr_t *setlocalpref(bgpattr_t *attr, uint32_t pref);

extern uint32_t getaggregatoras(const bgpattr_t *attr);

extern struct in_addr getaggregatoraddress(const bgpattr_t *attr);

extern bgpattr_t *setaggregator(bgpattr_t *attr, uint32_t as, size_t as_size, struct in_addr in);

void *getmpnlri(bgpattr_t *attr, size_t *pn)
{
    assert(attr->code == MP_REACH_NLRI_CODE);

    unsigned char *ptr = &attr->len;

    size_t len = *ptr++;
    if (attr->flags & ATTR_EXTENDED_LENGTH) {
        len <<= 8;
        len |= *ptr++;
    }

    ptr += sizeof(uint16_t) + sizeof(uint8_t);
    if (likely(pn))
        *pn = *ptr;

    return ptr + 2 * sizeof(uint8_t);
}

void *getmpnexthop(bgpattr_t *attr, size_t *pn)
{
    assert(attr->code == MP_REACH_NLRI_CODE);

    unsigned char *ptr = &attr->len;

    size_t len = *ptr++;
    if (attr->flags & ATTR_EXTENDED_LENGTH) {
        len <<= 8;
        len |= *ptr++;
    }

    ptr += sizeof(uint16_t) + sizeof(uint8_t);
    if (likely(pn))
        *pn = *ptr;

    return ptr + 2 * sizeof(uint8_t);
}

int stobgporigin(const char *s)
{
    if (strcasecmp(s, "i") == 0 || strcasecmp(s, "igp") == 0)
        return ORIGIN_IGP;
    else if (strcasecmp(s, "e") == 0 || strcasecmp(s, "egp") == 0)
        return ORIGIN_EGP;
    else if (strcmp(s, "?") == 0 || strcasecmp(s, "incomplete") == 0)
        return ORIGIN_INCOMPLETE;
    else
        return ORIGIN_BAD;
}

bgpattr_t *putasseg32(bgpattr_t *attr, int seg_type, const uint32_t *seg, size_t count)
{
    assert(attr->code == AS_PATH_CODE || attr->code == AS4_PATH_CODE);

    int extended = attr->flags & ATTR_EXTENDED_LENGTH;

    unsigned char *ptr = &attr->len;

    size_t len   = *ptr++;
    size_t limit = ATTR_LENGTH_MAX;
    if (extended) {
        len <<= 8;
        len |= *ptr++;

        limit = ATTR_EXTENDED_LENGTH_MAX;
    }

    size_t size = count * sizeof(*seg);

    size += AS_SEGMENT_HEADER_SIZE;
    if (unlikely(len + size > limit))
        return NULL;  // would overflow attribute length
    if (unlikely(count > AS_SEGMENT_COUNT_MAX))
        return NULL;  // would overflow segment size

    ptr   += len;
    *ptr++ = seg_type;  // segment type
    *ptr++ = count;     // AS count
    for (size_t i = 0; i < count; i++) {
        uint32_t as32 = tobig32(*seg++);
        memcpy(ptr, &as32, sizeof(as32));
        ptr += sizeof(as32);
    }

    // write updated length
    len += size;
    ptr  = &attr->len;
    if (extended) {
        *ptr++ = (len >> 8);
        len &= 0xff;
    }
    *ptr++ = len;
    return attr;
}

bgpattr_t *putasseg16(bgpattr_t *attr, int type, const uint16_t *seg, size_t count)
{
    assert(attr->code == AS_PATH_CODE);

    unsigned char *ptr = &attr->len;

    int extended = attr->flags & ATTR_EXTENDED_LENGTH;
    size_t len   = *ptr++;
    size_t limit = ATTR_LENGTH_MAX;
    if (extended) {
        len <<= 8;
        len |= *ptr++;

        limit = ATTR_EXTENDED_LENGTH_MAX;
    }

    size_t size = count * sizeof(*seg);
    size += AS_SEGMENT_HEADER_SIZE;
    if (unlikely(len + size > limit))
        return NULL;  // would overflow attribute length
    if (unlikely(count > AS_SEGMENT_COUNT_MAX))
        return NULL;  // would overflow segment size

    ptr   += len;
    *ptr++ = type;   // segment type
    *ptr++ = count;  // AS count
    for (size_t i = 0; i < count; i++) {
        uint16_t as16 = tobig16(*seg++);
        memcpy(ptr, &as16, sizeof(as16));
        ptr += sizeof(as16);
    }

    // write updated length
    ptr  = &attr->len;
    len += size;
    if (extended) {
        *ptr++ = (len >> 8);
        len &= 0xff;
    }
    *ptr++ = len;
    return attr;
}

bgpattr_t *setmpafisafi(bgpattr_t *dst, afi_t afi, safi_t safi)
{
    assert(dst->code == MP_REACH_NLRI_CODE || dst->code == MP_UNREACH_NLRI_CODE);

    unsigned char *ptr = &dst->len;

    ptr++;
    if (dst->flags & ATTR_EXTENDED_LENGTH)
        ptr++;

    uint16_t t = tobig16(afi);
    memcpy(ptr, &t, sizeof(t));
    ptr += sizeof(t);

    *ptr = safi;
    return dst;
}

bgpattr_t *putmpnexthop(bgpattr_t *dst, int family, const void *addr)
{
    assert(dst->code == MP_REACH_NLRI_CODE);
    assert(family == AF_INET || family == AF_INET6);

    unsigned char *ptr = &dst->len;

    int extended = dst->flags & ATTR_EXTENDED_LENGTH;
    size_t len   = *ptr++;
    size_t limit = ATTR_LENGTH_MAX;
    if (extended) {
        len <<= 8;
        len |= *ptr++;

        limit = ATTR_EXTENDED_LENGTH_MAX;
    }

    size_t n = (family == AF_INET) ? sizeof(struct in_addr) : sizeof(struct in6_addr);
    if (unlikely(len + n > limit))
        return NULL;  // would overflow attribute length

    unsigned char *nhlen = ptr + sizeof(uint16_t) + sizeof(uint8_t); // keep a pointer to NEXT_HOP length
    if (unlikely(*nhlen + n > 0xff))
        return NULL;  // would overflow next-hop length
    if (unlikely(nhlen + sizeof(uint8_t) + *nhlen != &ptr[len]))
        return NULL;  // attribute already has a NLRI field!

    memcpy(&ptr[len], addr, n);

    // write updated NEXT HOP length
    *nhlen += n;

    // write updated length
    ptr  = &dst->len;
    len += n;
    if (extended) {
        *ptr++ = (len >> 8);
        len &= 0xff;
    }
    *ptr++ = len;
    return dst;
}

bgpattr_t *putmpnlri(bgpattr_t *dst, const netaddr_t *addr)
{
    assert(dst->code == MP_REACH_NLRI_CODE || dst->code == MP_UNREACH_NLRI_CODE);

    unsigned char *ptr = &dst->len;

    int extended = dst->flags & ATTR_EXTENDED_LENGTH;
    size_t len   = *ptr++;
    size_t limit = ATTR_LENGTH_MAX;
    if (extended) {
        len <<= 8;
        len |= *ptr++;

        limit = ATTR_EXTENDED_LENGTH_MAX;
    }

    size_t n = naddrsize(addr);
    if (unlikely(len + n + 1 > limit))
        return NULL;  // would overflow attribute length

    ptr   += len;
    *ptr++ = addr->bitlen;
    memcpy(ptr, addr->bytes, n);

    // write updated length
    ptr  = &dst->len;
    len += n + 1;
    if (extended) {
        *ptr++ = (len >> 8);
        len &= 0xff;
    }
    *ptr++ = len;
}

/*
static size_t appendsegment(unsigned char **pptr, unsigned char *end, int type, const void *ases, size_t as_size, size_t count)
{
    if (count == 0)
        return 0;

    unsigned char *ptr = *pptr;

    size_t size = count * as_size;
    size_t total = AS_SEGMENT_HEADER_SIZE + size;
    if (likely(ptr + total <= end)) {
        *ptr++ = type;
        *ptr++ = count;
        memcpy(ptr, ases, size);

        ptr += size;
    } else {
        ptr = end; // don't append anymore
    }

    *pptr = ptr;
    return total;
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

static size_t parseasset(unsigned char **pptr, unsigned char *end, size_t as_size, const char *s, char **eptr)
{
    union {
        uint32_t as32[AS_SEGMENT_COUNT_MAX];
        uint16_t as16[AS_SEGMENT_COUNT_MAX];
    } segbuf;

    size_t count = 0;
    size_t len   = 0;
    bool commas  = false;

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

        if (unlikely(count == AS_SEGMENT_COUNT_MAX)) {
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
        ptr   = end;
        goto out;
    }

    // output segment
    len = appendsegment(pptr, end, AS_SEGMENT_SEQ, &segbuf, as_size, count);

out:
    return len;
}

static size_t parseaspath(unsigned char **pptr, unsigned char *end, const char *s, size_t as_size, char **eptr)
{
    union {
        uint32_t as32[AS_SEGMENT_COUNT_MAX];
        uint16_t as16[AS_SEGMENT_COUNT_MAX];
    } segbuf;

    size_t count = 0;
    size_t len = 0;

    char *epos;
    while (true) {
        while (isspace(*s)) s++;

        if (*s == '\0')
            break;

        if (*s == '{') {
            // AS set segment
            s++;

            // append SEQ segment so far, NOP if count == 0
            len += appendsegment(pptr, end, AS_SEGMENT_SEQ, &segbuf, as_size, count);
            count = 0;

            // go on parsing the SET segment
            len += parseasset(pptr, end, as_size, s, &epos);
        } else {
            // additional AS sequence segment entry
            if (unlikely(count == AS_SEGMENT_COUNT_MAX) {
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
    len += appendsegment(pptr, end, AS_SEGMENT_SEQ, &segbuf, as_size, count);

out:
    *eptr = (char *) s;
    return len;
}

size_t stoaspath(bgpattr_t *attr, size_t n, int code, int flags, size_t as_size, const char *s, char **eptr)
{
    char *dummy;
    size_t off = 0;

    if (!eptr)
        eptr = &dummy;

    unsigned char *ptr = &attr->code;
    unsigned char *end = ptr + n;
    if (ptr < end)
        *ptr++ = code;
    if (ptr < end)
        *ptr++ = flags;
    if (ptr < end)
        *ptr++ = 0;
    if ((flags & ATTR_EXTENDED_LENGTH) != 0 && ptr < end)
        *ptr++ = 0;

    size_t len  = hdrsize;
    len        += parseaspath(&ptr, end, s, sizeof(uint32_t), eptr);
    return len;
}

size_t stoaspath16(bgpattr_t *attr, size_t n, int flags, const char *s, char **eptr)
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

size_t stoas4path(bgpattr_t *attr, size_t n, int flags, const char *s, char **eptr)
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
*/

static bgpattr_t *appendcommunities(bgpattr_t *attr, const void *comms, size_t n)
{
    unsigned char *ptr = &attr->len;
    int extended       = attr->flags & ATTR_EXTENDED_LENGTH;

    size_t limit = ATTR_LENGTH_MAX;
    size_t len   = *ptr++;
    if (extended) {
        limit = ATTR_EXTENDED_LENGTH_MAX;
        len <<= 8;
        len |= *ptr++;
    }
    if (unlikely(len + n > limit))
        return NULL;

    memcpy(ptr + len, comms, n);
    len += n;

    ptr = &attr->len;
    if (extended) {
        *ptr++  = len >> 8;
        len    &= 0xff;
    }

    *ptr = len;
    return attr;
}

bgpattr_t *putcommunities(bgpattr_t *attr, const community_t *comms, size_t count)
{
    assert(attr->code == COMMUNITY_CODE);

    return appendcommunities(attr, comms, count * sizeof(*comms));
}

bgpattr_t *putexcommunities(bgpattr_t *attr, const ex_community_t *comms, size_t count)
{
    assert(attr->code == EXTENDED_COMMUNITY_CODE);

    return appendcommunities(attr, comms, count * sizeof(*comms));
}

bgpattr_t *putlargecommunities(bgpattr_t *attr, const large_community_t *comms, size_t count)
{
    assert(attr->code == LARGE_COMMUNITY_CODE);

    return appendcommunities(attr, comms, count * sizeof(*comms));
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

    sprintf(buf, "%" PRIu32, (uint32_t) c);
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

