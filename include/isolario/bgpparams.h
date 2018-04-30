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
 * @file isolario/bgpparams.h
 *
 * @brief Constants and functions for BGP open packet parameter field.
 *
 * This file exposes a fast and performance-oriented interface to BGP open
 * packet parameters, defined in relevant RFC.
 *
 * @note This file is guaranteed to include \a stddef.h and \a stdint.h, as well
 *       as \a isolario/netaddr.h, it may include other standard C or
 *       isolario-specific headers in the interest of providing inline versions
 *       of its functions.
 */

#ifndef ISOLARIO_BGPPARAMS_H_
#define ISOLARIO_BGPPARAMS_H_

#include <assert.h>
#include <isolario/branch.h>
#include <isolario/endian.h>
#include <isolario/netaddr.h>
#include <stddef.h>
#include <string.h>

enum {
    PARAM_CODE_OFFSET = 0,
    PARAM_LENGTH_OFFSET = sizeof(uint8_t),

    PARAM_HEADER_SIZE = PARAM_LENGTH_OFFSET + sizeof(uint8_t),
    PARAM_LENGTH_MAX = 0xff,

    /// @brief Maximum parameter size in bytes (header included).
    PARAM_SIZE_MAX = PARAM_LENGTH_MAX + PARAM_HEADER_SIZE,

    /// @brief Maximum overall parameter chunk size in BGP open.
    PARAMS_SIZE_MAX = 0xff
};

/// @brief Constants relevant to the parameters field inside a BGP open packet.
enum {
    CAPABILITY_CODE = 2  ///< Parameter code indicating a capability list.
};

/// @brief https://www.iana.org/assignments/capability-codes/capability-codes.xhtml
enum {
    BAD_CAPABILITY_CODE = -1,                   ///< Not a valid value, error indicator.

    MULTIPROTOCOL_CODE = 1,                     ///< RFC 2858
    ROUTE_REFRESH_CODE = 2,                     ///< RFC 2918
    OUTBOUND_ROUTE_FILTERING_CODE = 3,          ///< RFC 5291
    MULTIPLE_ROUTES_TO_A_DESTINATION_CODE = 4,  ///< RFC 3107
    EXTENDED_NEXT_HOP_ENCODING_CODE = 5,        ///< RFC 5549
    EXTENDED_MESSAGE_CODE = 6,                  ///< https://tools.ietf.org/html/draft-ietf-idr-bgp-extended-messages-21
    BGPSEC_CAPABILITY_CODE = 7,                 ///< https://tools.ietf.org/html/draft-ietf-sidr-bgpsec-protocol-23
    GRACEFUL_RESTART_CODE = 64,                 ///< RFC 4724
    ASN32BIT_CODE = 65,                         ///< RFC 6793
    DYNAMIC_CAPABILITY_CODE = 67,
    MULTISESSION_BGP_CODE = 68,                 ///< https://tools.ietf.org/html/draft-ietf-idr-bgp-multisession-07
    ADD_PATH_CODE = 69,                         ///< RFC 7911
    ENHANCED_ROUTE_REFRESH_CODE = 70,           ///< RFC 7313
    LONG_LIVED_GRACEFUL_RESTART_CODE = 71,      ///< https://tools.ietf.org/html/draft-uttaro-idr-bgp-persistence-03
    FQDN_CODE = 73,                             ///< https://tools.ietf.org/html/draft-walton-bgp-hostname-capability-02
    MULTISESSION_CISCO_CODE = 131               ///< Cisco version of multisession_bgp
};

enum {
    CAPABILITY_CODE_OFFSET   = 0,
    CAPABILITY_LENGTH_OFFSET = CAPABILITY_CODE_OFFSET + sizeof(uint8_t),
    CAPABILITY_HEADER_SIZE   = CAPABILITY_LENGTH_OFFSET + sizeof(uint8_t),

    CAPABILITY_LENGTH_MAX = 0xff - CAPABILITY_HEADER_SIZE,
    CAPABILITY_SIZE_MAX   = CAPABILITY_LENGTH_MAX + CAPABILITY_HEADER_SIZE,

    ASN32BIT_LENGTH = sizeof(uint32_t),
    ASN32BIT_SIZE   = ASN32BIT_LENGTH + CAPABILITY_HEADER_SIZE,

    MULTIPROTOCOL_AFI_OFFSET      = 0,
    MULTIPROTOCOL_RESERVED_OFFSET = MULTIPROTOCOL_AFI_OFFSET + sizeof(uint16_t),
    MULTIPROTOCOL_SAFI_OFFSET     = MULTIPROTOCOL_RESERVED_OFFSET + sizeof(uint8_t),
    MULTIPROTOCOL_LENGTH          = MULTIPROTOCOL_SAFI_OFFSET + sizeof(uint8_t),
    MULTIPROTOCOL_SIZE            = MULTIPROTOCOL_LENGTH + CAPABILITY_HEADER_SIZE,

    GRACEFUL_RESTART_FLAGTIME_OFFSET = 0,
    GRACEFUL_RESTART_TUPLES_OFFSET   = GRACEFUL_RESTART_FLAGTIME_OFFSET + sizeof(uint16_t),
    GRACEFUL_RESTART_BASE_LENGTH     = GRACEFUL_RESTART_TUPLES_OFFSET,
    GRACEFUL_RESTART_BASE_SIZE       = GRACEFUL_RESTART_BASE_LENGTH + CAPABILITY_HEADER_SIZE
};

/// @brief Restart flags see: https://tools.ietf.org/html/rfc4724#section-3.
enum {
    RESTART_FLAG = 1 << 3 ///< Most significant bit, Restart State (R).
};

/// @brief Address Family flags, see: https://tools.ietf.org/html/rfc4724#section-3.
enum {
    FORWARDING_STATE = 1 << 7 ///< Most significant bit, Forwarding State (F).
};

typedef struct {
    afi_t afi;
    safi_t safi;
    uint8_t flags;
} afi_safi_t;

static_assert(sizeof(afi_safi_t) ==  4, "Unsupported platform");

/// @brief Extract capability code from raw capability buffer.
inline int bgpcapcode(const void *buf)
{
    const unsigned char *ptr = buf;
    return ptr[CAPABILITY_CODE_OFFSET];
}

/// @brief Extract capability length from raw capability buffer.
inline size_t bgpcaplen(const void *buf)
{
    const unsigned char *ptr = buf;
    return ptr[CAPABILITY_LENGTH_OFFSET];
}

inline uint32_t getasn32bit(const void *buf)
{
    assert(bgpcapcode(buf) == ASN32BIT_CODE);
    assert(bgpcaplen(buf) == ASN32BIT_LENGTH);

    uint32_t asn32bit;

    const unsigned char *ptr = buf;
    memcpy(&asn32bit, &ptr[CAPABILITY_HEADER_SIZE], sizeof(asn32bit));
    return frombig32(asn32bit);
}

inline void *makeasn32bit(void *buf, uint32_t as)
{
    as = tobig32(as);

    unsigned char *ptr = buf;
    *ptr++ = ASN32BIT_CODE;
    *ptr++ = ASN32BIT_LENGTH;
    memcpy(ptr, &as, sizeof(as));

    return buf;
}

inline void *makemultiprotocol(void *buf, afi_t afi, safi_t safi)
{
    uint16_t t = tobig16(afi);

    uint8_t *ptr = buf;
    *ptr++ = MULTIPROTOCOL_CODE;
    *ptr++ = MULTIPROTOCOL_LENGTH;
    memcpy(ptr, &t, sizeof(t));
    ptr += sizeof(t);

    *ptr++ = 0;  // reserved
    *ptr   = safi;
    return buf;
}

inline afi_safi_t getmultiprotocol(const void *buf)
{
    assert(bgpcapcode(buf) == MULTIPROTOCOL_CODE);
    assert(bgpcaplen(buf) == MULTIPROTOCOL_LENGTH);

    afi_safi_t r;
    uint16_t t;

    const unsigned char *ptr = buf;
    ptr += CAPABILITY_HEADER_SIZE;
    memcpy(&t, &ptr[MULTIPROTOCOL_AFI_OFFSET], sizeof(t));

    r.afi = frombig16(t);
    r.safi = ptr[MULTIPROTOCOL_SAFI_OFFSET];
    r.flags = 0;
    return r;
}

inline void *makegracefulrestart(void *buf, int flags, int secs)
{
    // RFC mandates that any other restart flag is reserved and zeroed
    flags &= RESTART_FLAG;

    unsigned char *ptr = buf;
    *ptr++ = GRACEFUL_RESTART_CODE;
    *ptr++ = GRACEFUL_RESTART_BASE_LENGTH;

    uint16_t flagtime = ((flags & 0x000f) << 12) | (secs & 0x0fff);
    flagtime = tobig16(flagtime);

    memcpy(ptr, &flagtime, sizeof(flagtime));
    return buf;
}

inline void *putgracefulrestarttuple(void *buf, afi_t afi, safi_t safi, int flags)
{
    assert(bgpcapcode(buf) == GRACEFUL_RESTART_CODE);

    size_t len = bgpcaplen(buf);

    // RFC mandates that any other flag is reserved and zeroed out
    flags &= FORWARDING_STATE;
    afi_safi_t t = {
        .afi = tobig16(afi),
        .safi = safi,
        .flags = flags
    };

    assert(len + sizeof(t) <= CAPABILITY_LENGTH_MAX);

    unsigned char *ptr = buf;
    ptr++;                             // skip capability code
    *ptr++ = len + sizeof(t);          // update length
    memcpy(&ptr[len], &t, sizeof(t));  // append tuple
    return buf;
}

inline int getgracefulrestarttime(const void *buf)
{
    assert(bgpcapcode(buf) == GRACEFUL_RESTART_CODE);

    uint16_t flagtime;

    const unsigned char *ptr = buf;
    memcpy(&flagtime, &ptr[CAPABILITY_HEADER_SIZE], sizeof(flagtime));
    flagtime = frombig16(flagtime);
    return flagtime & 0x0fff;
}

inline int getgracefulrestartflags(const void *buf)
{
    assert(bgpcapcode(buf) == GRACEFUL_RESTART_CODE);

    uint16_t flagtime;

    const unsigned char *ptr = buf;
    memcpy(&flagtime, &ptr[CAPABILITY_HEADER_SIZE], sizeof(flagtime));
    flagtime = frombig16(flagtime);

    // XXX: signal non-zeroed flags as an error or mask them?
    return flagtime >> 12;
}

/// @brief Get tuples from a graceful restart capability into a buffer
size_t getgracefulrestarttuples(afi_safi_t *dst, size_t n, const void *buf);

#endif
