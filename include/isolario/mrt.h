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
 * @file isolario/mrt.h
 *
 * @brief Isolario MRT packet reading and writing routines.
 *
 * @note WORK IN PROGRESS!!
 *
 */

#ifndef ISOLARIO_MRT_H_
#define ISOLARIO_MRT_H_

#include <isolario/bgp.h>  // also includes stdint.h
#include <stdarg.h>
#include <time.h>

enum {
    MRT_NULL = 0,          // Deprecated
    MRT_START = 1,         // Deprecated
    MRT_DIE = 2,           // Deprecated
    MRT_I_AM_DEAD = 3,     // Deprecated
    MRT_PEER_DOWN = 4,     // Deprecated
    MRT_BGP = 5,           // Deprecated also known as ZEBRA_BGP
    MRT_RIP = 6,           // Deprecated
    MRT_IDRP = 7,          // Deprecated
    MRT_RIPNG = 8,         // Deprecated
    MRT_BGP4PLUS = 9,      // Deprecated
    MRT_BGP4PLUS_01 = 10,  // Deprecated
    MRT_OSPFV2 = 11,
    MRT_TABLE_DUMP = 12,
    MRT_TABLE_DUMPV2 = 13,
    MRT_BGP4MP = 16,
    MRT_BGP4MP_ET = 17,
    MRT_ISIS = 32,
    MRT_ISIS_ET = 33,
    MRT_OSPFV3 = 48,
    MRT_OSPFV3_ET = 49
};

/**
 * @brief BGP/ZEBRA BGP subtypes
 *
 * Deprecated by BGP4MP
 */
enum {
    MRT_BGP_NULL         = 0,
    MRT_BGP_UPDATE       = 1,
    MRT_BGP_PREF_UPDATE  = 2,
    MRT_BGP_STATE_CHANGE = 3,
    MRT_BGP_SYNC         = 4,
    MRT_BGP_OPEN         = 5,
    MRT_BGP_NOTIFY       = 6,
    MRT_BGP_KEEPALIVE    = 7
};

/**
 * @brief BGP4MP types
 *
 * further detail at https://tools.ietf.org/html/rfc6396#section-4.2
 * and for extensions https://tools.ietf.org/html/rfc8050#page-2
 *
 * @see [iana bgp4mp Subtype Codes](https://www.iana.org/assignments/mrt/mrt.xhtml#BGP4MP-codes)
 *
 */
enum {
    BGP4MP_STATE_CHANGE              = 0,   ///< RFC 6396
    BGP4MP_MESSAGE                   = 1,   ///< RFC 6396
    BGP4MP_ENTRY                     = 2,   ///< Deprecated
    BGP4MP_SNAPSHOT                  = 3,   ///< Deprecated
    BGP4MP_MESSAGE_AS4               = 4,   ///< RFC 6396
    BGP4MP_STATE_CHANGE_AS4          = 5,   ///< RFC 6396
    BGP4MP_MESSAGE_LOCAL             = 6,   ///< RFC 6396
    BGP4MP_MESSAGE_AS4_LOCAL         = 7,   ///< RFC 6396
    BGP4MP_MESSAGE_ADDPATH           = 8,   ///< RFC 8050
    BGP4MP_MESSAGE_AS4_ADDPATH       = 9,   ///< RFC 8050
    BGP4MP_MESSAGE_LOCAL_ADDPATH     = 10,  ///< RFC 8050
    BGP4MP_MESSAGE_AS4_LOCAL_ADDPATH = 11   ///< RFC 8050
};

typedef struct {
    size_t as_size;
    uint32_t as;
    struct in_addr id;
    netaddr_t addr;
} peer_entry_t;

typedef struct {
    uint32_t seqno;
    afi_t afi;
    safi_t safi;
    netaddr_t nlri;
} rib_header_t;

typedef struct {
    uint16_t peer_idx;     // peer index number, always 0 for TABLE_DUMP
    uint16_t attr_length;  // attrs field length in bytes
    uint32_t seqno;        // entry sequence number, same as rib_header_t seqno
                           // when TABLE_DUMPV2, sequence entry information
                           // for TABLE_DUMP (TABLE_DUMP sequence numbers are
                           // 16-bits wide, wrapping is very likely)
    time_t originated;
    netaddr_t nlri;        // same as rib_header_t nlri when TABLE_DUMPV2
    uint32_t pathid;       // only meaningful for ADDPATH TABLE_DUMPV2 subtypes, 0 otherwise
    peer_entry_t *peer;    // PEER_INDEX information when TABLE_DUMPV2, available
                           // peer information when TABLE_DUMP
    bgpattr_t *attrs;
} rib_entry_t;

/**
 *
 * further detail at https://tools.ietf.org/html/rfc6396#section-4.3
 *
 * @see [iana table dumpv2 Subtype Codes](https://www.iana.org/assignments/mrt/mrt.xhtml#table-dump-v2-subtype-codes)
 */
enum {
    MRT_TABLE_DUMPV2_PEER_INDEX_TABLE = 1,               /// RFC6396
    MRT_TABLE_DUMPV2_RIB_IPV4_UNICAST = 2,               /// RFC6396
    MRT_TABLE_DUMPV2_RIB_IPV4_MULTICAST = 3,             /// RFC6396
    MRT_TABLE_DUMPV2_RIB_IPV6_UNICAST = 4,               /// RFC6396
    MRT_TABLE_DUMPV2_RIB_IPV6_MULTICAST = 5,             /// RFC6396
    MRT_TABLE_DUMPV2_RIB_GENERIC = 6,                    /// RFC6396
    MRT_TABLE_DUMPV2_GEO_PEER_TABLE = 7,                 /// RFC6397
    MRT_TABLE_DUMPV2_RIB_IPV4_UNICAST_ADDPATH = 8,       /// RFC8050
    MRT_TABLE_DUMPV2_RIB_IPV4_MULTICAST_ADDPATH = 9,     /// RFC8050
    MRT_TABLE_DUMPV2_RIB_IPV6_UNICAST_ADDPATH = 10,      /// RFC8050
    MRT_TABLE_DUMPV2_RIB_IPV6_MULTICAST_ADDPATH = 11,    /// RFC8050
    MRT_TABLE_DUMPV2_RIB_GENERIC_ADDPATH = 12            /// RFC8050
};

enum {
    //recoverable errors
    MRT_NOTPEERIDX = -1,
    //unrecoverable
    MRT_ENOERR = 0,      ///< No error (success) guaranteed to be zero.
    MRT_EIO,             ///< Input/Output error during packet read.
    MRT_EINVOP,          ///< Invalid operation (e.g. write while reading packet).
    MRT_ENOMEM,          ///< Out of memory.
    MRT_EBADHDR,         ///< Bad MRT packet header.
    MRT_EBADTYPE,        ///< Bad MRT packet type.
    MRT_EBADBGP4MPHDR,   ///< Bad BGP4MP header.
    MRT_EBADZEBRAHDR,    ///< Bad Zebra BGP header.
    MRT_EBADPEERIDXHDR,  ///< Bad Peer index header.
    MRT_EBADPEERIDX,     ///< Error encountered parsing associated Peer Index.
    MRT_ENEEDSPEERIDX,   ///< Packet needs a peer index, but none was provided.
    MRT_ERIBNOTSUP,      ///< Unsupported RIB entry encountered, according to RFC6396:
                         ///  "An implementation that does not recognize particular AFI and SAFI
                         ///  values SHOULD discard the remainder of the MRT record."
    MRT_EBADRIBENT,      ///< Corrupted or truncated RIB entry
    MRT_EAFINOTSUP,
    MRT_ETYPENOTSUP      ///< MRT type is not supported
};

inline const char *mrtstrerror(int err)
{
    switch (err) {
    case MRT_NOTPEERIDX:
        return "Not Peer Index message";
    case MRT_ENOERR:
        return "Success";
    case MRT_EIO:
        return "I/O error";
    case MRT_EINVOP:
        return "Invalid operation";
    case MRT_ENOMEM:
        return "Out of memory";
    case MRT_EBADHDR:
        return "Bad MRT header";
    case MRT_EBADTYPE:
        return "Bad MRT packet type";
    case MRT_EBADBGP4MPHDR:
        return "Bad BGP4MP header";
    case MRT_EBADZEBRAHDR:
        return "Bad Zebra BGP header";
    case MRT_EBADPEERIDXHDR:
        return "Bad Peer Index header";
    case MRT_EBADPEERIDX:
        return "Bad Peer Index message";
    case MRT_ENEEDSPEERIDX:
        return "No peer index provided";
    case MRT_ERIBNOTSUP:
        return "Unsupported RIB entry";
    case MRT_EAFINOTSUP:
        return "Unsupported AFI";
    case MRT_EBADRIBENT:
        return "Corrupted or truncated RIB entry";
    case MRT_ETYPENOTSUP:
        return "Unsupported MRT packet type";
    default:
        return "Unknown error";
    }
}

int mrterror(void);

int setmrtread(const void *data, size_t n);

int setmrtreadfd(int fd);

int setmrtreadfrom(io_rw_t *io);

int mrtclose(void);

int mrtclosepi(void);

//header section

typedef struct {
    struct timespec stamp;
    int type, subtype;
    size_t len;
} mrt_header_t;

mrt_header_t *getmrtheader(void);

int setmrtheaderv(const mrt_header_t *hdr, va_list va);

int setmrtheader(const mrt_header_t *hdr, ...);

enum {
    MRTBUFSIZ       = 4096,
    MRTPRESRVBUFSIZ = 512
};

typedef struct {
    uint32_t peer_as, local_as;
    netaddr_t peer_addr, local_addr;
    uint16_t  iface;
    uint16_t old_state, new_state;  ///< Only meaningful for BGP4MP_STATE_CHANGE*
} bgp4mp_header_t;

typedef struct {
    uint16_t peer_as;
    netaddr_t peer_addr;
    union {
        struct {
            uint16_t old_state, new_state;  ///< Only meaningful for MRT_BGP_STATE_CHANGE
        };
        struct {
            uint16_t local_as;
            netaddr_t local_addr;
        };
    };
} zebra_header_t;

/// @brief Packet reader/writer global status structure.
typedef struct mrt_msg_s {
    uint16_t flags;      ///< General status flags.
    int16_t err;         ///< Last error code.
    uint32_t bufsiz;     ///< Packet buffer capacity

    struct mrt_msg_s *peer_index;

    mrt_header_t hdr;
    union {
        struct {
            peer_entry_t pe;      ///< Current peer entry
            unsigned char *peptr; ///< Raw peer entry pointer in current packet
        };
        struct {
            rib_header_t ribhdr;
            rib_entry_t ribent;
            peer_entry_t ribpe;
            unsigned char *reptr; ///< Raw RIB entry pointer in current packet
        };

        bgp4mp_header_t bgp4mphdr;

        zebra_header_t zebrahdr;
    };

    unsigned char *buf;  ///< Packet buffer base.

    uint32_t *pitab;
    uint16_t picount;

    unsigned char fastbuf[MRTBUFSIZ];  ///< Fast buffer to avoid malloc()s.
    union {
        uint32_t fastpitab[MRTPRESRVBUFSIZ / sizeof(uint32_t)];
        unsigned char prsvbuf[MRTPRESRVBUFSIZ];
    };
} mrt_msg_t;

mrt_msg_t *getmrt(void);

mrt_msg_t *getmrtpi(void);

int ismrtext(void);

int ismrtext_r(mrt_msg_t *msg);

int isbgpwrapper(void);

int isbgpwrapper_r(mrt_msg_t *msg);

int ismrtasn32bit(void);

int ismrtasn32bit_r(mrt_msg_t *msg);

int ismrtaddpath(void);

int ismrtaddpath_r(mrt_msg_t *msg);

int ismrtrib(void);

int ismrtrib_r(mrt_msg_t *msg);

int setmrtpi_r(mrt_msg_t *msg, mrt_msg_t *pi);

int mrterror_r(mrt_msg_t *msg);

int mrtclose_r(mrt_msg_t *msg);

int setmrtread_r(mrt_msg_t *msg, const void *data, size_t n);

int setmrtreadfd_r(mrt_msg_t *msg, int fd);

int setmrtreadfrom_r(mrt_msg_t *msg, io_rw_t *io);

// header

mrt_header_t *getmrtheader_r(mrt_msg_t *msg);

int setmrtheaderv_r(mrt_msg_t *msg, const mrt_header_t *hdr, va_list va);  // TODO

int setmrtheader_r(mrt_msg_t *msg, const mrt_header_t *hdr, ...);  // TODO

// Peer Index

struct in_addr getpicollector(void);

struct in_addr getpicollector_r(mrt_msg_t *msg);

size_t getpiviewname(char *buf, size_t n);

size_t getpiviewname_r(mrt_msg_t *msg, char *buf, size_t n);

int setpeerents(const void *buf, size_t n);

int setpeerents_r(mrt_msg_t *msg, const void *buf, size_t n);

void *getpeerents(size_t *pcount, size_t *pn);

/**
 * @brief Reentrant variant of getpeerents().
 *
 * @param [in]  msg
 * @param [out] pcount If not NULL, storage where peer entries count should be stored.
 * @param [out] pn     If not NULL, storage where peer entries chunk size (in bytes) should be stored.
 */
void *getpeerents_r(mrt_msg_t *msg, size_t *pcount, size_t *pn);

int startpeerents(size_t *pcount);

int startpeerents_r(mrt_msg_t *msg, size_t *pcount);

peer_entry_t *nextpeerent(void);

peer_entry_t *nextpeerent_r(mrt_msg_t *msg);

int putpeerent(const peer_entry_t *pe);

int putpeerent_r(mrt_msg_t *msg, const peer_entry_t *pe);

int endpeerents(void);

int endpeerents_r(mrt_msg_t *msg);

// RIB subtypes

int setribpi(void);

int setribpi_r(mrt_msg_t *msg, mrt_msg_t *pi);

int setribents(const void *buf, size_t n);

int setribents_r(mrt_msg_t *msg, const void *buf, size_t n);

void *getribents(size_t *pcount, size_t *pn);

/**
 * @brief Reentrant variant of getribents().
 *
 * @param [in]  msg
 * @param [out] pcount If not NULL, storage where RIB entries count should be stored.
 * @param [out] pn     If not NULL, storage where RIB entries chunk size (in bytes) should be stored.
 */
void *getribents_r(mrt_msg_t *msg, size_t *pcount, size_t *pn);

rib_header_t *startribents(size_t *pcount);

rib_header_t *startribents_r(mrt_msg_t *msg, size_t *pcount);

rib_entry_t *nextribent(void);

rib_entry_t *nextribent_r(mrt_msg_t *msg);

int putribent(const rib_entry_t *pe, uint16_t idx, time_t seconds, const bgpattr_t *attrs, size_t attrs_size);

int putribent_r(mrt_msg_t *msg, const rib_entry_t *pe, uint16_t idx, time_t seconds, const bgpattr_t *attrs, size_t attrs_size);

int endribents(void);

int endribents_r(mrt_msg_t *msg);

// BGP4MP

bgp4mp_header_t *getbgp4mpheader(void);

bgp4mp_header_t *getbgp4mpheader_r(mrt_msg_t *msg);

void *unwrapbgp4mp(size_t *pn);

void *unwrapbgp4mp_r(mrt_msg_t *msg, size_t *pn);

// ZEBRA BGP

zebra_header_t *getzebraheader(void);

zebra_header_t *getzebraheader_r(mrt_msg_t *msg);

void *unwrapzebra(size_t *pn);

void *unwrapzebra_r(mrt_msg_t *msg, size_t *pn);

#endif

