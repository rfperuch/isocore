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
 * @file isolario/netaddr.h
 *
 * @brief Network address family types, utilities and constants.
 *
 * @note This file is guaranteed to include \a arpa/inet.h and \a stdint.h,
 *       it may include additional headers in the interest of providing inline
 *       versions of its functions.
 */

#ifndef ISOLARIO_NETADDR_H_
#define ISOLARIO_NETADDR_H_

#include <assert.h>
#include <arpa/inet.h>
#include <isolario/branch.h>
#include <isolario/funcattribs.h>
#include <string.h>
#include <stdint.h>

/// @brief Address Family Identifier values.
typedef uint16_t afi_t;

/// @brief Notable values for \a afi_t.
enum {
    AFI_BAD = -1,

    AFI_IPV4 = 1,
    AFI_IPV6 = 2,
    AFI_IPX = 11,
    AFI_APPLETALK = 12
};

/** 
 * 
 * @brief netaddres printing modes
 *
 * @see naddrtos()
 */
enum {
    NADDR_CIDR, ///< prints with /bitlen
    NADDR_PLAIN, ///< prints without /bitlen
};

/**
 * @brief Subsequent Address Family Identifier values
 *
 * https://www.iana.org/assignments/safi-namespace/safi-namespace.xhtml
 */
typedef uint8_t safi_t;

/// @brief Notable values for \a safi_t.
enum {
    SAFI_BAD = -1,
    SAFI_UNICAST = 1,  ///< RFC4760 http://www.iana.org/go/rfc4760
    SAFI_MULTICAST = 2 ///< RFC4760 http://www.iana.org/go/rfc4760
};

typedef struct {
    short family;    ///< Unix address family: \a AF_UNSPEC, \a AF_INET or \a AF_INET6.
    short bitlen;  ///< Address length in bits.
    union {
        struct in_addr sin;
        struct in6_addr sin6;
        uint32_t u32[sizeof(struct in6_addr) / sizeof(uint32_t)];
        unsigned char bytes[sizeof(struct in6_addr)];
    };
} netaddr_t;

typedef struct {
    netaddr_t pfx;
    uint32_t pathid;
} netaddrap_t;

/// @brief Make a network address from an IPv4 address.
nonnull(1) void makenaddr(netaddr_t *ip, const void *sin, int bitlen);

/// @brief Make a network address from an IPv6 address.
nonnull(1) void makenaddr6(netaddr_t *ip, const void *sin6, int bitlen);

/// @brief String to network address.
nonnull(1, 2) int stonaddr(netaddr_t *ip, const char *s);

/// @brief Return the network size in bytes, given the size in bits.
inline constfunc int naddrsize(int bitlen)
{
    assert(bitlen >= 0);
    return (bitlen >> 3) + ((bitlen & 7) != 0);
}

/**
 * @brief Deduce address family from string representation.
 *
 * This function uses a simple and quick heuristic to detect the
 * family of an IP address in its string representation.
 * The fact that an address *might* be of a certain family does not
 * imply that it is correct, that may only be verified by
 * explicitly trying to convert it using stonaddr().
 * It is however true that if the address *is* actually valid, then
 * its family is certainly the one returned by this function.
 *
 * @return \a AF_INET if address is a candidate to be an IPv4 address,
 *         \a AF_INET6 if address is a candidate to be an IPv6 address,
 *         \a AF_UNSPEC if string is definitely a bad address.
 */
inline purefunc sa_family_t saddrfamily(const char *s)
{
    if (unlikely(!s))
        return AF_UNSPEC;

    for (int i = 0; i < 4; i++) {
        char c = *s++;
        if (c == '.')
            return AF_INET;
        if (c == ':')
            return AF_INET6;
        if (unlikely(c == '\0'))
            return AF_UNSPEC;
    }
    return likely(*s == ':') ? AF_INET6 : AF_UNSPEC;
}

inline purefunc nonnull(1, 2) int prefixeqwithmask(const void *a, const void *b, unsigned int mask)
{
    unsigned int n = mask >> 3;

    if (memcmp(a, b, n) == 0) {
        unsigned int m = ~0u << (8 - (mask & 3));

        const unsigned char *aptr = a;
        const unsigned char *bptr = b;

        return (mask & 0x7) == 0 || (aptr[n] & m) == (bptr[n] & m);
    }

    return 0;
}

inline purefunc nonnull(1, 2) int prefixeq(const netaddr_t *a, const netaddr_t *b)
{
    return a->family == b->family
        && a->bitlen == b->bitlen
        && prefixeqwithmask(a->bytes, b->bytes, a->bitlen);
}

inline purefunc nonnull(1, 2) int naddreq(const netaddr_t *a, const netaddr_t *b)
{
    if (a->family != b->family)
        return 0;

    // TODO benchmark a branch-less option
    switch (a->family) {
    case AF_INET:
        return memcmp(&a->sin, &b->sin, sizeof(a->sin)) == 0;
    case AF_INET6:
        return memcmp(&a->sin6, &b->sin6, sizeof(a->sin6)) == 0;
    default:
        return 1;  // both AF_UNSPEC...
    }
}

/**
 * @brief Network address to string.
 *
 * @return The string representation of the provided network address.
 *
 * @note The returned pointer refers to a possibly statically allocated
 *       zone managed by the library and must not be free()d.
 */
nonullreturn char *naddrtos(const netaddr_t *ip, int mode);

#endif
