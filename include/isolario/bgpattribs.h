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
    ATTR_CODE_OFFSET            = 0,
    ATTR_FLAGS_OFFSET           = ATTR_CODE_OFFSET + sizeof(uint8_t),
    ATTR_LENGTH_OFFSET          = ATTR_FLAGS_OFFSET + sizeof(uint8_t),
    ATTR_EXTENDED_LENGTH_OFFSET = ATTR_LENGTH_OFFSET + sizeof(uint8_t),

    ATTR_HEADER_SIZE          = ATTR_LENGTH_OFFSET + sizeof(uint8_t),
    ATTR_EXTENDED_HEADER_SIZE = ATTR_EXTENDED_LENGTH_OFFSET +
                                sizeof(uint8_t),

    ATTR_LENGTH_MAX          = 0xff,
    ATTR_EXTENDED_LENGTH_MAX = 0xffff,
    ATTR_SIZE_MAX            = ATTR_HEADER_SIZE + ATTR_LENGTH_MAX,
    ATTR_EXTENDED_SIZE_MAX   = ATTR_EXTENDED_HEADER_SIZE +
                               ATTR_EXTENDED_LENGTH_MAX,

    /// Origin attribute offset, relative to BGP attribute header size.
    ORIGIN_LENGTH        = sizeof(uint8_t),
    ORIGIN_SIZE          = ATTR_HEADER_SIZE + ORIGIN_LENGTH,
    EXTENDED_ORIGIN_SIZE = ATTR_EXTENDED_HEADER_SIZE + ORIGIN_LENGTH,

    ORIGINATOR_ID_LENGTH        = sizeof(uint32_t),
    ORIGINATOR_ID_SIZE          = ATTR_HEADER_SIZE + ORIGINATOR_ID_LENGTH,
    EXTENDED_ORIGINATOR_ID_SIZE = ATTR_EXTENDED_HEADER_SIZE +
                                  ORIGINATOR_ID_LENGTH,

    ATOMIC_AGGREGATOR_SIZE          = ATTR_HEADER_SIZE,
    EXTENDED_ATOMIC_AGGREGATOR_SIZE = ATTR_EXTENDED_HEADER_SIZE,

    NEXT_HOP_LENGTH        = sizeof(struct in_addr),
    NEXT_HOP_SIZE          = ATTR_HEADER_SIZE + NEXT_HOP_LENGTH,
    NEXT_HOP_EXTENDED_SIZE = ATTR_EXTENDED_HEADER_SIZE + NEXT_HOP_LENGTH,

    MULTI_EXIT_DISC_LENGTH        = sizeof(uint32_t),
    MULTI_EXIT_DISC_SIZE          = ATTR_HEADER_SIZE + MULTI_EXIT_DISC_LENGTH,
    EXTENDED_MULTI_EXIT_DISC_SIZE = ATTR_EXTENDED_HEADER_SIZE +
                                    MULTI_EXIT_DISC_LENGTH,

    LOCAL_PREF_LENGTH        = sizeof(uint32_t),
    LOCAL_PREF_SIZE          = ATTR_HEADER_SIZE + LOCAL_PREF_LENGTH,
    EXTENDED_LOCAL_PREF_SIZE = ATTR_EXTENDED_HEADER_SIZE + LOCAL_PREF_LENGTH,

    AGGREGATOR_AS32_LENGTH        = sizeof(uint32_t) + sizeof(struct in_addr),
    AGGREGATOR_AS16_LENGTH        = sizeof(uint16_t) + sizeof(struct in_addr),
    AGGREGATOR_AS32_SIZE          = ATTR_HEADER_SIZE + AGGREGATOR_AS32_LENGTH,
    AGGREGATOR_AS16_SIZE          = ATTR_HEADER_SIZE + AGGREGATOR_AS16_LENGTH,
    EXTENDED_AGGREGATOR_AS32_SIZE = ATTR_EXTENDED_HEADER_SIZE +
                                    AGGREGATOR_AS32_LENGTH,
    EXTENDED_AGGREGATOR_AS16_SIZE = ATTR_EXTENDED_HEADER_SIZE +
                                    AGGREGATOR_AS16_LENGTH,

    AS4_AGGREGATOR_LENGTH        = sizeof(uint32_t) + sizeof(struct in_addr),
    AS4_AGGREGATOR_SIZE          = ATTR_HEADER_SIZE + AS4_AGGREGATOR_LENGTH,
    EXTENDED_AS4_AGGREGATOR_SIZE = ATTR_EXTENDED_HEADER_SIZE +
                                   AS4_AGGREGATOR_LENGTH
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

/// @brief Get attribute code from an attribute buffer.
inline int bgpattrcode(const void *attr)
{
    const unsigned char *ptr = attr;
    return ptr[ATTR_CODE_OFFSET];
}

/// @brief Get attribute flags from buffer.
inline int bgpattrflags(const void *attr)
{
    const unsigned char *ptr = attr;
    return ptr[ATTR_FLAGS_OFFSET];
}

/**
 * @brief Is BGP attribute extended.
 *
 * Tests if a BGP update attribute has the \a ATTR_EXTENDED_LENGTH
 * flag set.
 *
 * Shorthand for:
 * @code{.c}
 *     (bgpattrflags(buf) & ATTR_EXTENDED_LENGTH) != 0;
 * @endcode
 *
 * @param [in] buf Buffer referencing a BGP update attribute, must *not* be
 *                 \a NULL; behavior is undefined if buffer references any
 *                 information other than a legitimate BGP attribute.
 *
 * @return \a true if \a ATTR_EXTENDED_LENGTH is set in attribute header,
 *         \a false otherwise.
 */
inline int isbgpattrext(const void *attr)
{
    return (bgpattrflags(attr) & ATTR_EXTENDED_LENGTH) != 0;
}

/// @brief Get attribute header size, accounting for the \a ATTR_EXTENDED_LENGTH flag.
inline size_t bgpattrhdrsize(const void *attr)
{
    return isbgpattrext(attr) ? ATTR_EXTENDED_HEADER_SIZE
                              : ATTR_HEADER_SIZE;
}

/// @brief Get the attribute length, accounting for the \a ATTR_EXTENDED_LENGTH flag.
inline size_t bgpattrlen(const void *attr)
{
    const unsigned char *ptr = attr;
    int flags = bgpattrflags(attr);

    size_t len = ptr[ATTR_LENGTH_OFFSET];
    if (flags & ATTR_EXTENDED_LENGTH) {
        len <<= 8;
        len |= ptr[ATTR_EXTENDED_LENGTH_OFFSET];
    }
    return len;
}

inline int getbgporigin(const void *attr)
{
    assert(bgpattrcode(attr) == ORIGIN_CODE);
    assert(bgpattrlen(attr) == ORIGIN_LENGTH);

    const unsigned char *ptr = attr;
    return ptr[bgpattrhdrsize(attr)];
}

inline void *makebgporigin(void *attr, int flags, int origin)
{
    assert(origin != ORIGIN_BAD);

    unsigned char *ptr = attr;

    *ptr++ = ORIGIN_CODE;
    *ptr++ = flags;
    if (flags & ATTR_EXTENDED_LENGTH)
        *ptr++ = 0;  // if you say so...

    *ptr++ = ORIGIN_LENGTH;
    *ptr   = origin;
    return attr;
}

/**
 * @brief Convert a string to a BGP origin attribute.
 *
 * @return The \a buf argument, \a NULL on invalid string.
 */
void *stobgporigin(void *attr, int flags, const char *s);

inline void *makeaspath(void *attr, int flags)
{
    unsigned char *ptr = attr;

    // AS path must be transitive
    flags |= ATTR_TRANSITIVE;
    flags &= ATTR_TRANSITIVE | ATTR_EXTENDED_LENGTH;

    *ptr++ = AS_PATH_CODE;
    *ptr++ = flags;
    if (flags & ATTR_EXTENDED_LENGTH)
        *ptr++ = 0;

    *ptr = 0;
    return attr;
}

inline void *makeas4path(void *attr, int flags)
{
    unsigned char *ptr = attr;

    flags |= ATTR_TRANSITIVE;
    flags &= ATTR_TRANSITIVE | ATTR_EXTENDED_LENGTH;

    *ptr++ = AS4_PATH_CODE;
    *ptr++ = flags;
    if (flags & ATTR_EXTENDED_LENGTH)
        *ptr++ = 0;

    *ptr++ = 0;
    return attr;
}

/**
 * @brief Put 16-bits wide AS segment into AS path attribute.
 *
 * This function is identical to putasseg32(), but takes 16-bits wide ASes.
 *
 * @see putasseg32()
 */
void *putasseg16(void *attr, int type, const uint16_t *seg, size_t count);

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
void *putasseg32(void *attr, int type, const uint32_t *seg, size_t count);

/// @brief String to 16 bits AS Path attribute, buffer is \a n bytes wide.
size_t stoaspath16(void *attr, size_t n, int flags, const char *s, char **eptr);

/// @brief String to 32 bits AS Path.
size_t stoaspath32(void *attr, size_t n, int flags, const char *s, char **eptr);

inline uint32_t getoriginatorid(const void *attr)
{
    assert(bgpattrcode(attr) == ORIGINATOR_ID_CODE);
    assert(bgpattrlen(attr) == ORIGINATOR_ID_LENGTH);

    const unsigned char *ptr = attr;
    ptr += bgpattrhdrsize(attr);

    uint32_t id;
    memcpy(&id, ptr, sizeof(id));
    return frombig32(id);
}

inline void *makeoriginatorid(void *attr, int flags, uint32_t id)
{
    unsigned char *ptr = attr;

    *ptr++ = ORIGINATOR_ID_CODE;
    *ptr++ = flags;
    if (flags & ATTR_EXTENDED_LENGTH)
        *ptr++ = 0;  // as you wish...

    *ptr++ = ORIGINATOR_ID_LENGTH;

    id = tobig32(id);
    memcpy(ptr, &id, sizeof(id));
    return attr;
}

inline void *makeatomicaggregate(void *attr, int flags)
{
    unsigned char *ptr = attr;

    *ptr++ = ATOMIC_AGGREGATE_CODE;
    *ptr++ = flags;
    if (flags & ATTR_EXTENDED_LENGTH)
        *ptr++ = 0;  // you're the boss

    *ptr = 0;
    return attr;
}

inline struct in_addr getnexthop(const void *attr)
{
    assert(bgpattrcode(attr) == NEXT_HOP_CODE);
    assert(bgpattrlen(attr) == NEXT_HOP_LENGTH);

    const unsigned char *ptr = attr;
    ptr += bgpattrhdrsize(attr);

    struct in_addr in;
    memcpy(&in, ptr, sizeof(in));
    return in;
}

inline void *makenexthop(void *attr, int flags, struct in_addr in)
{
    unsigned char *ptr = attr;

    *ptr++ = NEXT_HOP_CODE;
    *ptr++ = flags;
    if (flags & ATTR_EXTENDED_LENGTH)
        *ptr++ = 0;  // really...?

    *ptr++ = NEXT_HOP_LENGTH;
    memcpy(ptr, &in, sizeof(in));
    return attr;
}

inline uint32_t getmultiexitdisc(const void *attr)
{
    assert(bgpattrcode(attr) == MULTI_EXIT_DISC_CODE);
    assert(bgpattrlen(attr) == MULTI_EXIT_DISC_LENGTH);

    const unsigned char *ptr = attr;
    ptr += bgpattrhdrsize(attr);

    uint32_t disc;
    memcpy(&disc, ptr, sizeof(disc));
    return frombig32(disc);
}

inline void *makemultiexitdisc(void *attr, int flags, uint32_t disc)
{
    unsigned char *ptr = attr;

    *ptr++ = MULTI_EXIT_DISC_CODE;
    *ptr++ = flags;
    if (flags & ATTR_EXTENDED_LENGTH)
        *ptr++ = 0;  // ok...

    *ptr++ = MULTI_EXIT_DISC_LENGTH;

    disc = tobig32(disc);
    memcpy(ptr, &disc, sizeof(disc));
    return attr;
}

inline uint32_t getlocalpref(const void *attr)
{
    assert(bgpattrcode(attr) == LOCAL_PREF_CODE);
    assert(bgpattrlen(attr) == LOCAL_PREF_LENGTH);

    const unsigned char *ptr = attr;
    ptr += bgpattrhdrsize(attr);

    uint32_t pref;
    memcpy(&pref, attr, sizeof(pref));
    return frombig32(pref);
}

inline void *makelocalpref(void *attr, int flags, uint32_t pref)
{
    unsigned char *ptr = attr;

    *ptr++ = LOCAL_PREF_CODE;
    *ptr++ = flags;
    if (flags & ATTR_EXTENDED_LENGTH)
        *ptr++ = 0;  // whatever...

    *ptr++ = LOCAL_PREF_LENGTH;

    pref = tobig32(pref);
    memcpy(ptr, &pref, sizeof(pref));
    return attr;
}

inline size_t aggregatorassize(const void *attr)
{
    assert(bgpattrcode(attr) == AGGREGATOR_CODE);
    if (bgpattrlen(attr) == AGGREGATOR_AS32_LENGTH)
        return sizeof(uint32_t);
    else
        return sizeof(uint16_t);
}

inline uint32_t getaggregatoras(const void *attr)
{
    const unsigned char *ptr = attr;

    ptr += bgpattrhdrsize(attr);
    if (aggregatorassize(attr) == sizeof(uint32_t)) {
        uint32_t as32;
        memcpy(&as32, ptr, sizeof(as32));
        return frombig32(as32);
    } else {
        uint16_t as16;
        memcpy(&as16, ptr, sizeof(as16));
        return frombig16(as16);
    }
}

inline struct in_addr getaggregatoraddress(const void *attr)
{
    const unsigned char *ptr = attr;
    ptr += bgpattrhdrsize(attr);
    ptr += aggregatorassize(attr);

    struct in_addr in;
    memcpy(&in, ptr, sizeof(in));
    return in;
}

inline void *makeaggregator(void *attr, int flags, uint32_t as, size_t as_size, struct in_addr in)
{
    if (unlikely(as_size != sizeof(uint16_t) && as_size != sizeof(uint32_t)))
        return NULL;

    unsigned char *ptr = attr;

    *ptr++ = AGGREGATOR_CODE;
    *ptr++ = flags;
    if (flags & ATTR_EXTENDED_LENGTH)
        *ptr++ = 0;  // as you command...

    *ptr++ = as_size + sizeof(in);
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

inline void *makeas4aggregator(void *attr, int flags, uint32_t as, struct in_addr in)
{
    unsigned char *ptr = attr;

    *ptr++ = AS4_AGGREGATOR_CODE;
    *ptr++ = flags;
    if (flags & ATTR_EXTENDED_LENGTH)
        *ptr++ = 0;  // as you please...

    *ptr++ = AS4_AGGREGATOR_LENGTH;

    as = tobig32(as);
    memcpy(ptr, &as, sizeof(as));
    ptr += sizeof(as);
    memcpy(ptr, &in, sizeof(in));
    return attr;
}

inline uint32_t getas4aggregatoras(const void *attr)
{
    assert(bgpattrcode(attr) == AS4_AGGREGATOR_CODE);
    assert(bgpattrlen(attr) == AS4_AGGREGATOR_LENGTH);

    const unsigned char *ptr = attr;
    ptr += bgpattrhdrsize(attr);

    uint32_t as4;
    memcpy(&as4, ptr, sizeof(as4));
    return frombig32(as4);
}

inline struct in_addr getas4aggregatoraddress(const void *attr)
{
    assert(bgpattrcode(attr) == AS4_AGGREGATOR_CODE);
    assert(bgpattrlen(attr) == AS4_AGGREGATOR_LENGTH);

    const unsigned char *ptr = attr;
    ptr += bgpattrhdrsize(attr) + sizeof(uint32_t);

    struct in_addr in;
    memcpy(&in, ptr, sizeof(in));
    return in;
}

inline void *makecommunity(void *attr, int flags)
{
    unsigned char *ptr = attr;

    flags |= ATTR_TRANSITIVE | ATTR_OPTIONAL;
    flags &= ATTR_TRANSITIVE | ATTR_OPTIONAL | ATTR_EXTENDED_LENGTH;

    *ptr++ = COMMUNITY_CODE;
    *ptr++ = flags;
    if (flags & ATTR_EXTENDED_LENGTH)
        *ptr++ = 0;

    *ptr++ = 0;
    return attr;
}

/// @brief Community to string.
char *communitytos(community_t c);

/// @brief String to community.
community_t stocommunity(const char *s, char **eptr);

/// @brief String to IPv6 specific extended community attribute.
ex_community_t stoexcommunity6(const char *s, char **eptr);

/// @brief Large community to string.
char *largecommunitytos(large_community_t c);

/// @brief String to large community attribute.
large_community_t stolargecommunity(const char *s, char **eptr);

#endif
