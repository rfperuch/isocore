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

/**
 * @file isolario/bgpattribs.h
 *
 * @brief Utilities for BGP attributes creation and reading.
 */

#ifndef ISOLARIO_BGPATTRIBS_H_
#define ISOLARIO_BGPATTRIBS_H_

#include <assert.h>
#include <arpa/inet.h>
#include <isolario/branch.h>
#include <isolario/endian.h>
#include <isolario/netaddr.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/// @brief https://www.iana.org/assignments/bgp-parameters/bgp-parameters.xhtml#bgp-parameters-2
enum {
    /// @brief Not a valid attribute code, error indicator value.
    ATTR_BAD_CODE = -1,

    /// @brief Origin attribute code, see [RFC 4271](https://datatracker.ietf.org/doc/rfc4271/).
    ORIGIN_CODE                                   = 1,
    /// @brief AS Path attribute code, see [RFC 4271](https://datatracker.ietf.org/doc/rfc4271/).
    AS_PATH_CODE                                  = 2,
    /// @brief Next Hop attribute code, see [RFC 4271](https://datatracker.ietf.org/doc/rfc4271/).
    NEXT_HOP_CODE                                 = 3,
    MULTI_EXIT_DISC_CODE                          = 4,    ///< RFC 4271
    LOCAL_PREF_CODE                               = 5,    ///< RFC 4271
    ATOMIC_AGGREGATE_CODE                         = 6,    ///< RFC 4271
    AGGREGATOR_CODE                               = 7,    ///< RFC 4271
    COMMUNITY_CODE                                = 8,    ///< RFC 1997
    ORIGINATOR_ID_CODE                            = 9,    ///< RFC 4456
    CLUSTER_LIST_CODE                             = 10,   ///< RFC 4456
    DPA_CODE                                      = 11,   ///< RFC 6938
    ADVERTISER_CODE                               = 12,   ///< RFC 1863, RFC 4223 and RFC 6938
    RCID_PATH_CLUSTER_ID_CODE                     = 13,   ///< RFC 1863, RFC 4223 and RFC 6938
    MP_REACH_NLRI_CODE                            = 14,   ///< RFC 4760
    MP_UNREACH_NLRI_CODE                          = 15,   ///< RFC 4760
    EXTENDED_COMMUNITY_CODE                       = 16,   ///< RFC 4360
    AS4_PATH_CODE                                 = 17,   ///< RFC 6793
    AS4_AGGREGATOR_CODE                           = 18,   ///< RFC 6793
    SAFI_SSA_CODE                                 = 19,   ///< https://tools.ietf.org/html/draft-kapoor-nalawade-idr-bgp-ssa-02 and https://tools.ietf.org/html/draft-wijnands-mt-discovery-01
    CONNECTOR_CODE                                = 20,   ///< RFC 6037
    AS_PATHLIMIT_CODE                             = 21,   ///< https://tools.ietf.org/html/draft-ietf-idr-as-pathlimit-03
    PMSI_TUNNEL_CODE                              = 22,   ///< RFC 6514
    TUNNEL_ENCAPSULATION_CODE                     = 23,   ///< RFC 5512
    TRAFFIC_ENGINEERING_CODE                      = 24,   ///< RFC 5543
    IPV6_ADDRESS_SPECIFIC_EXTENDED_COMMUNITY_CODE = 25,   ///< RFC 5701
    AIGP_CODE                                     = 26,   ///< RFC 7311
    PE_DISTINGUISHER_LABELS_CODE                  = 27,   ///< RFC 6514
    BGP_ENTROPY_LEVEL_CAPABILITY_CODE             = 28,   ///< RFC 6790 and RFC 7447
    BGP_LS_CODE                                   = 29,   ///< RFC 7752
    LARGE_COMMUNITY_CODE                          = 32,   ///< RFC 8092
    BGPSEC_PATH_CODE                              = 33,   ///< https://tools.ietf.org/html/draft-ietf-sidr-bgpsec-protocol-22
    BGP_COMMUNITY_CONTAINER_CODE                  = 34,   ///< https://tools.ietf.org/html/draft-ietf-idr-wide-bgp-communities-04
    BGP_PREFIX_SID_CODE                           = 40,   ///< https://tools.ietf.org/html/draft-ietf-idr-bgp-prefix-sid-06
    ATTR_SET_CODE                                 = 128,  ///< RFC 6368
    RESERVED_CODE                                 = 255   ///< RFC 2042
};

/// @brief Bit constants for the attribute flags fields.
enum {
    ATTR_EXTENDED_LENGTH = 1 << 4,  ///< Attribute length has an additional byte.
    ATTR_PARTIAL         = 1 << 5,  ///< Attribute is partial.
    ATTR_TRANSITIVE      = 1 << 6,  ///< Attribute is transitive.
    ATTR_OPTIONAL        = 1 << 7   ///< Attribute is optional.
};

enum {
    ORIGIN_BAD        = -1,

    ORIGIN_IGP        = 0,
    ORIGIN_EGP        = 1,
    ORIGIN_INCOMPLETE = 2
};

/**
 * @brief Constants relevant for AS Path segments.
 *
 * @see putasseg16(), putasseg32()
 */
enum {
    AS_SEGMENT_HEADER_SIZE = 2,
    AS_SEGMENT_COUNT_MAX = 0xff,

    AS_SEGMENT_BAD = -1,

    AS_SEGMENT_SET = 1,
    AS_SEGMENT_SEQ = 2
};

enum {
    ATTR_HEADER_SIZE          = 3 * sizeof(uint8_t),
    ATTR_EXTENDED_HEADER_SIZE = 2 * sizeof(uint8_t) + sizeof(uint16_t),

    ATTR_LENGTH_MAX          = 0xff,
    ATTR_EXTENDED_LENGTH_MAX = 0xffff,

    /// Origin attribute offset, relative to BGP attribute header size.
    ORIGIN_LENGTH           = sizeof(uint8_t),
    ORIGINATOR_ID_LENGTH    = sizeof(uint32_t),
    ATOMIC_AGGREGATE_LENGTH = 0,
    NEXT_HOP_LENGTH         = sizeof(struct in_addr),
    MULTI_EXIT_DISC_LENGTH  = sizeof(uint32_t),
    LOCAL_PREF_LENGTH       = sizeof(uint32_t),

    AGGREGATOR_AS32_LENGTH = sizeof(uint32_t) + sizeof(struct in_addr),
    AGGREGATOR_AS16_LENGTH = sizeof(uint16_t) + sizeof(struct in_addr),

    AS4_AGGREGATOR_LENGTH  = sizeof(uint32_t) + sizeof(struct in_addr)
};

/**
 * @brief Well known communities constants.
 *
 * @see [IANA Well known communities list](https://www.iana.org/assignments/bgp-well-known-communities/bgp-well-known-communities.xhtml)
 */
enum {
    /**
     * Planned Shut well known community.
     *
     * @see [Graceful BGP session shutdown](https://datatracker.ietf.org/doc/draft-francois-bgp-gshut/)
     */
    COMMUNITY_PLANNED_SHUT               = (int) 0xffff0000,
    COMMUNITY_ACCEPT_OWN                 = (int) 0xffff0001,  ///< RFC 7611
    COMMUNITY_ROUTE_FILTER_TRANSLATED_V4 = (int) 0xffff0002,  ///< https://tools.ietf.org/html/draft-francois-bgp-gshut-01
    COMMUNITY_ROUTE_FILTER_V4            = (int) 0xffff0003,  ///< https://tools.ietf.org/html/draft-l3vpn-legacy-rtc-00
    COMMUNITY_ROUTE_FILTER_TRANSLATED_V6 = (int) 0xffff0004,  ///< https://tools.ietf.org/html/draft-l3vpn-legacy-rtc-00
    COMMUNITY_ROUTE_FILTER_V6            = (int) 0xffff0005,  ///< https://tools.ietf.org/html/draft-l3vpn-legacy-rtc-00
    COMMUNITY_LLGR_STALE                 = (int) 0xffff0006,  ///< https://tools.ietf.org/html/draft-uttaro-idr-bgp-persistence-03
    COMMUNITY_NO_LLGR                    = (int) 0xffff0007,  ///< https://tools.ietf.org/html/draft-uttaro-idr-bgp-persistence-03
    COMMUNITY_ACCEPT_OWN_NEXTHOP         = (int) 0xffff0008,  // ?
    /**
     * @brief BLACKHOLE community.
     *
     * @see [RFC 7999](https://datatracker.ietf.org/doc/rfc7999/)
     */
    COMMUNITY_BLACKHOLE                  = (int) 0xffff029a,
    /**
     *
     * @see [RFC 1997](https://datatracker.ietf.org/doc/rfc1997/)
     */
    COMMUNITY_NO_EXPORT                  = (int) 0xffffff01,
    /**
     *
     * @see [RFC 1997](https://datatracker.ietf.org/doc/rfc1997/)
     */
    COMMUNITY_NO_ADVERTISE               = (int) 0xffffff02,
    /**
     *
     * @see [RFC 1997](https://datatracker.ietf.org/doc/rfc1997/)
     */
    COMMUNITY_NO_EXPORT_SUBCONFED        = (int) 0xffffff03,
    /**
     *
     * @see [RFC 3765](https://datatracker.ietf.org/doc/rfc3765/)
     */
    COMMUNITY_NO_PEER                    = (int) 0xffffff04
};

/**
 * @brief BGP Community structure.
 *
 * @see [BGP Communities Attribute](https://tools.ietf.org/html/rfc1997)
 */
typedef uint32_t community_t;

/**
 * @brief Extended community structure.
 *
 * @see [RFC4360](https://datatracker.ietf.org/doc/rfc4360/)
 */
typedef union {
    struct {
        uint8_t hitype;  ///< High octet of the type field.
        uint8_t lotype;  ///< Low octet of the type field, may be part of the value.
        uint16_t hival;  ///< Most significant portion of value field.
        uint32_t loval;  ///< Least significant portion of value field.
    };
    struct {
        uint8_t : 8;          ///< Padding field.
        uint8_t two_subtype;  ///< Two-octets subtype field.
        uint16_t two_global;  ///< Two-octets global field.
        uint32_t two_local;   ///< Two-octets local field.
    };
    struct {
        uint8_t : 8;           ///< Padding field.
        uint8_t v4_subtype;    ///< IPv4 Address subtype field.
        uint16_t v4_higlobal;  ///< Most significant portion of IPv4 Address global field.
        uint16_t v4_loglobal;  ///< Least significant portion of IPv4 Address global field.
        uint16_t v4_local;     ///< IPv4 Address local field.
    };
    struct {
        uint8_t : 8;
        uint8_t opq_subtype;
        uint16_t opq_hival;
        uint32_t opq_loval;
    };
    uint64_t typeval;  ///< To manage the extended community as a whole.
} ex_community_t;

static_assert(sizeof(ex_community_t) == 8, "Unsupported platform");

/// @brief Notable bits inside \a ex_community_t \a hitype field.
enum {
    IANA_AUTHORITY_BIT       = 1 << 7,
    TRANSITIVE_COMMUNITY_BIT = 1 << 6
};

/// @brief retrieve the whole global v4 address extended community field.
inline uint32_t getv4addrglobal(ex_community_t ecomm)
{
    return ((uint32_t) ecomm.v4_higlobal << 16) | ecomm.v4_loglobal;
}

/// @brief Retrieve the opaque value inside an extended community.
inline uint64_t getopaquevalue(ex_community_t ecomm)
{
    return ((uint64_t) ecomm.opq_hival << 16) | ecomm.opq_loval;
}

/**
 * @brief IPv6 specific extended community structure.
 *
 * @see [RFC 5701](https://datatracker.ietf.org/doc/rfc5701/)
 */
typedef struct {
    uint8_t hitype;      ///< High order byte of the type field.
    uint8_t lotype;      ///< Low order byte of the type field.
    uint8_t global[16];  ///< Packed IPv6 global administrator address.
    uint16_t local;      ///< Local administrator field.
} ex_community_v6_t;

static_assert(sizeof(ex_community_v6_t) == 20, "Unsupported platform");

inline void getexcomm6globaladdr(struct in6_addr *dst, const ex_community_v6_t *excomm6)
{
    memcpy(dst, excomm6->global, sizeof(*dst));
}

/**
 * @brief Large community structure.
 *
 * @see [RFC 8092](https://datatracker.ietf.org/doc/rfc8092/)
 */
typedef struct {
    uint32_t global;
    uint32_t hilocal;
    uint32_t lolocal;
} large_community_t;

static_assert(sizeof(large_community_t) == 12, "Unsupported platform");

typedef struct {
    uint8_t flags;
    uint8_t code;
    union {
        struct {
            uint8_t len;
            unsigned char data[1];
        };
        struct {
            uint8_t exlen[2];
            unsigned char exdata[1];
        };
    };
} bgpattr_t;

inline size_t getattrlenextended(const bgpattr_t *attr)
{
    assert(attr->flags & ATTR_EXTENDED_LENGTH);

    uint16_t len;
    memcpy(&len, attr->exlen, sizeof(len));
    return frombig16(len);
}

inline void setattrlenextended(bgpattr_t *attr, size_t len)
{
    assert(len <= UINT16_MAX);
    assert(attr->flags & ATTR_EXTENDED_LENGTH);

    uint16_t n = tobig16(len);
    memcpy(attr->exlen, &n, sizeof(n));
}

inline int getorigin(const bgpattr_t *attr)
{
    assert(attr->code == ORIGIN_CODE);

    return attr->data[!!(attr->flags & ATTR_EXTENDED_LENGTH)];
}

inline bgpattr_t *setorigin(bgpattr_t *dst, int origin)
{
    assert(dst->code == ORIGIN_CODE);

    dst->data[!!(dst->flags & ATTR_EXTENDED_LENGTH)] = origin;
    return dst;
}

enum {
    DEFAULT_ORIGIN_FLAGS = ATTR_TRANSITIVE,
    EXTENDED_ORIGIN_FLAGS = DEFAULT_ORIGIN_FLAGS | ATTR_EXTENDED_LENGTH,

    DEFAULT_NEXT_HOP_FLAGS = ATTR_TRANSITIVE,
    EXTENDED_NEXT_HOP_FLAGS = DEFAULT_NEXT_HOP_FLAGS | ATTR_EXTENDED_LENGTH,

    DEFAULT_AS_PATH_FLAGS = ATTR_TRANSITIVE,
    EXTENDED_AS_PATH_FLAGS = DEFAULT_AS_PATH_FLAGS | ATTR_EXTENDED_LENGTH,

    DEFAULT_AS4_PATH_FLAGS = ATTR_TRANSITIVE | ATTR_OPTIONAL,
    EXTENDED_AS4_PATH_FLAGS = DEFAULT_AS4_PATH_FLAGS | ATTR_EXTENDED_LENGTH,

    DEFAULT_MP_REACH_NLRI_FLAGS = ATTR_OPTIONAL,
    EXTENDED_MP_REACH_NLRI_FLAGS = DEFAULT_MP_REACH_NLRI_FLAGS | ATTR_EXTENDED_LENGTH,
    MP_REACH_BASE_LEN = sizeof(uint16_t) + 3 * sizeof(uint8_t),

    DEFAULT_MP_UNREACH_NLRI_FLAGS = ATTR_OPTIONAL,
    EXTENDED_MP_UNREACH_NLRI_FLAGS = DEFAULT_MP_UNREACH_NLRI_FLAGS | ATTR_EXTENDED_LENGTH,
    MP_UNREACH_BASE_LEN = sizeof(uint16_t) + sizeof(uint8_t),

    DEFAULT_COMMUNITY_FLAGS = ATTR_TRANSITIVE | ATTR_OPTIONAL,
    EXTENDED_COMMUNITY_FLAGS = DEFAULT_COMMUNITY_FLAGS | ATTR_EXTENDED_LENGTH
};

bgpattr_t *setmpafisafi(bgpattr_t *dst, afi_t afi, safi_t safi);

bgpattr_t *putmpnexthop(bgpattr_t *dst, int family, const void *addr);

bgpattr_t *putmpnrli(bgpattr_t *dst, const netaddr_t *addr);

inline void *getaspath(const bgpattr_t *attr, size_t *pn)
{
    assert(attr->code == AS_PATH_CODE || attr->code == AS4_PATH_CODE);

    unsigned char *ptr = (unsigned char *) &attr->len;

    size_t len = *ptr++;
    if (attr->flags & ATTR_EXTENDED_LENGTH) {
        len <<= 8;
        len |= *ptr++;
    }

    if (likely(pn))
        *pn = len;

    return ptr;
}

/**
 * @brief Put 16-bits wide AS segment into AS path attribute.
 *
 * This function is identical to putasseg32(), but takes 16-bits wide ASes.
 *
 * @see putasseg32()
 */
bgpattr_t *putasseg16(bgpattr_t *dst, int type, const uint16_t *seg, size_t count);

/**
 * @brief Put 32-bits wide AS segment into AS path attribute.
 *
 * Append an AS segment to an AS Path attribute, ASes are 32-bits wide.
 *
 * @param [out] attr  Buffer containing an AS Path attribute, the segment
 *                    is appended at attribute's end. This argument must *not*
 *                    be \a NULL. Behavior is undefined if this argument
 *                    references anything other than a legitimate AS Path
 *                    attribute.
 * @param [in]  type  Segment type, either \a AS_SEGMENT_SEQ or
 *                    \a AS_SEGMENT_SET, specifying anything else is
 *                    undefined behavior.
 * @param [in]  seg   AS buffer for segment, this value may only be \a NULL if
 *                    \a count is also 0.
 * @param [in]  count Number of ASes stored into \a seg.
 *
 * @return The \a buf argument, \a NULL on error. Possible errors are:
 *         * The ASes count would overflow the segment header count field
 *           (i.e. \a count is larger than \a AS_SEGMENT_COUNT_MAX).
 *         * Appending the segment to the specified AS Path would overflow
 *           the attribute length (i.e. the overall AS Path length in bytes
 *           would be larger than \a ATTR_LENGTH_MAX or
 *           \a ATTR_EXTENDED_LENGTH_MAX, depending on the
 *           \a ATTR_EXTENDED_LENGTH flag status of the attribute).
 *
 * @note This function does not take any additional parameter to specify the
 *       size of \a buf, because in no way this function shall append more than:
 *       @code{.c}
 *           AS_SEGMENT_HEADER_SIZE + count * sizeof(*seg);
 *       @endcode
 *       bytes to the provided attribute, whose current size can be computed
 *       using:
 *       @code{.c}
 *           bgpattrhdrsize(buf) + bgpattrlen(buf);
 *       @endcode
 *
 *       The caller has the ability to ensure that this function shall not
 *       overflow the buffer.
 *       In any way, this function cannot generate oversized attributes, hence
 *       a buffer of size \a ATTR_EXTENDED_SIZE_MAX is always large enough for
 *       any extended attribute, and \a ATTR_SIZE_MAX is always large enough to
 *       hold non-extended attributes.
 */
bgpattr_t *putasseg32(bgpattr_t *attr, int type, const uint32_t *seg, size_t count);

// TODO size_t stoaspath(bgpattr_t *attr, size_t n, int code, int flags, size_t as_size, const char *s, char **eptr);

inline uint32_t getoriginatorid(const bgpattr_t *attr)
{
    assert(attr->code == ORIGINATOR_ID_CODE);

    uint32_t id;
    memcpy(&id, &attr->data[!!(attr->flags & ATTR_EXTENDED_LENGTH)], sizeof(id));
    return frombig32(id);
}

inline bgpattr_t *setoriginatorid(bgpattr_t *attr, uint32_t id)
{
    assert(attr->code == ORIGINATOR_ID_CODE);

    id = tobig32(id);
    memcpy(&attr->data[!!(attr->flags & ATTR_EXTENDED_LENGTH)], &id, sizeof(id));
    return attr;
}

inline struct in_addr getnexthop(const bgpattr_t *attr)
{
    assert(attr->code == NEXT_HOP_CODE);

    struct in_addr in;
    memcpy(&in, &attr->data[!!(attr->flags & ATTR_EXTENDED_LENGTH)], sizeof(in));
    return in;
}

inline bgpattr_t *setnexthop(bgpattr_t *attr, struct in_addr in)
{
    assert(attr->code == NEXT_HOP_CODE);

    memcpy(&attr->data[!!(attr->flags & ATTR_EXTENDED_LENGTH)], &in, sizeof(in));
    return attr;
}

inline uint32_t getmultiexitdisc(const bgpattr_t *attr)
{
    assert(attr->code == MULTI_EXIT_DISC_CODE);

    uint32_t disc;
    memcpy(&disc, &attr->data[!!(attr->flags & ATTR_EXTENDED_LENGTH)], sizeof(disc));
    return frombig32(disc);
}

inline bgpattr_t *setmultiexitdisc(bgpattr_t *attr, uint32_t disc)
{
    assert(attr->code == MULTI_EXIT_DISC_CODE);

    disc = tobig32(disc);
    memcpy(&attr->data[!!(attr->flags & ATTR_EXTENDED_LENGTH)], &disc, sizeof(disc));
    return attr;
}

inline uint32_t getlocalpref(const bgpattr_t *attr)
{
    assert(attr->code == LOCAL_PREF_CODE);

    uint32_t pref;
    memcpy(&pref, &attr->data[!!(attr->flags & ATTR_EXTENDED_LENGTH)], sizeof(pref));
    return frombig32(pref);
}

inline bgpattr_t *setlocalpref(bgpattr_t *attr, uint32_t pref)
{
    assert(attr->code == LOCAL_PREF_CODE);

    pref = tobig32(pref);
    memcpy(&attr->data[!!(attr->flags & ATTR_EXTENDED_LENGTH)], &pref, sizeof(pref));
    return attr;
}

inline uint32_t getaggregatoras(const bgpattr_t *attr)
{
    assert(attr->code == AGGREGATOR_CODE || attr->code == AS4_AGGREGATOR_CODE);

    const unsigned char *ptr = &attr->len;
    size_t len = *ptr++;
    if (attr->flags & ATTR_EXTENDED_LENGTH) {
        len <<= 8;
        len |= *ptr++;
    }

    if (len == AGGREGATOR_AS32_LENGTH) {
        uint32_t as32;
        memcpy(&as32, ptr, sizeof(as32));
        return frombig32(as32);
    } else {
        uint16_t as16;
        memcpy(&as16, ptr, sizeof(as16));
        return frombig16(as16);
    }
}

inline struct in_addr getaggregatoraddress(const bgpattr_t *attr)
{
    assert(attr->code == AGGREGATOR_CODE || attr->code == AS4_AGGREGATOR_CODE);

    const unsigned char *ptr = &attr->len;
    size_t len = *ptr++;
    if (attr->flags & ATTR_EXTENDED_LENGTH) {
        len <<= 8;
        len |= *ptr++;
    }

    struct in_addr in;

    ptr += len;
    ptr -= sizeof(in);

    memcpy(&in, ptr, sizeof(in));
    return in;
}

inline bgpattr_t *setaggregator(bgpattr_t *attr, uint32_t as, size_t as_size, struct in_addr in)
{
    assert(attr->code == AGGREGATOR_CODE || attr->code == AS4_AGGREGATOR_CODE);

    if (unlikely(as_size != sizeof(uint16_t) && as_size != sizeof(uint32_t)))
        return NULL;

    unsigned char *ptr = &attr->data[!!(attr->flags & ATTR_EXTENDED_LENGTH)];
    if (as_size == sizeof(uint32_t)) {
        uint32_t as32 = tobig32(as);
        memcpy(ptr, &as32, sizeof(as32));
        ptr += sizeof(as32);
    } else {
        uint16_t as16 = tobig16(as);
        memcpy(ptr, &as16, sizeof(as16));
        ptr += sizeof(as16);
    }

    memcpy(ptr, &in, sizeof(in));
    return attr;
}

inline afi_t getmpafi(const bgpattr_t *attr)
{
    assert(attr->code == MP_REACH_NLRI_CODE || attr->code == MP_UNREACH_NLRI_CODE);

    const unsigned char *ptr = &attr->data[!!(attr->flags & ATTR_EXTENDED_LENGTH)];

    uint16_t t;
    memcpy(&t, ptr, sizeof(t));
    return frombig16(t);
}

inline safi_t getmpsafi(const bgpattr_t *attr)
{
    assert(attr->code == MP_REACH_NLRI_CODE || attr->code == MP_UNREACH_NLRI_CODE);

    return attr->data[!!(attr->flags & ATTR_EXTENDED_LENGTH) + sizeof(uint16_t)];
}

void *getmpnlri(const bgpattr_t *attr, size_t *pn);

void *getmpnexthop(const bgpattr_t *attr, size_t *pn);

bgpattr_t *putcommunities(bgpattr_t *attr, const community_t *comms, size_t count);

bgpattr_t *putexcommunities(bgpattr_t *attr, const ex_community_t *comms, size_t count);

bgpattr_t *putlargecommunities(bgpattr_t *attr, const large_community_t *comms, size_t count);

inline void *getcommunities(const bgpattr_t *attr, size_t size, size_t *pn)
{
    assert(attr->code == COMMUNITY_CODE || attr->code == EXTENDED_COMMUNITY_CODE || attr->code == LARGE_COMMUNITY_CODE);

    unsigned char *ptr = (unsigned char *) &attr->len;
    size_t len = *ptr++;
    if (attr->flags & ATTR_EXTENDED_LENGTH) {
        len <<= 8;
        len |= *ptr++;
    }

    if (likely(pn))
        *pn = len / size;

    return ptr;
}

enum {
    COMMSTR_EX,
    COMMSTR_PLAIN
};

/// @brief Community to string.
char *communitytos(community_t c, int mode);

/// @brief String to community.
community_t stocommunity(const char *s, char **eptr);

/// @brief String to IPv6 specific extended community attribute.
ex_community_t stoexcommunity6(const char *s, char **eptr);

/// @brief Large community to string.
char *largecommunitytos(large_community_t c);

/// @brief String to large community attribute.
large_community_t stolargecommunity(const char *s, char **eptr);

#endif
