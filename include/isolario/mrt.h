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
    MRT_BGP = 5,           // Deprecated
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

enum {
    MRT_BGP_NULL = 0,
    MRT_BGP_UPDATE = 1,
    MRT_BGP_PREF_UPDATE = 2,
    MRT_BGP_STATE_CHANGE = 3,
    MRT_BGP_SYNC = 4,
    MRT_BGP_OPEN = 5,
    MRT_BGP_NOTIFY = 6,
    MRT_BGP_KEEPALIVE = 7
};

/**
 *   @brief BGP4_mp types
 *
 * further detail at https://tools.ietf.org/html/rfc6396#section-4.2
 * and for extensions https://tools.ietf.org/html/rfc8050#page-2
 *
 * @see [iana bgp4mp Subtype Codes](https://www.iana.org/assignments/mrt/mrt.xhtml#BGP4MP-codes)
 *
 */
enum {
    BGP4MP_STATE_CHANGE = 0,               ///< RFC 6396
    BGP4MP_MESSAGE = 1,                    ///< RFC 6396
    BGP4MP_ENTRY = 2,                      ///< Deprecated
    BGP4MP_SNAPSHOT = 3,                   ///< Deprecated
    BGP4MP_MESSAGE_AS4 = 4,                ///< RFC 6396
    BGP4MP_STATE_CHANGE_AS4 = 5,           ///< RFC 6396
    BGP4MP_MESSAGE_LOCAL = 6,              ///< RFC 6396
    BGP4MP_MESSAGE_AS4_LOCAL = 7,          ///< RFC 6396
    BGP4MP_MESSAGE_ADDPATH = 8,            ///< RFC 8050
    BGP4MP_MESSAGE_AS4_ADDPATH = 9,        ///< RFC 8050
    BGP4MP_MESSAGE_LOCAL_ADDPATH = 10,     ///< RFC 8050
    BGP4MP_MESSAGE_AS4_LOCAL_ADDPATH = 11  ///< RFC 8050
};

/**
*   @brief BGP4_mp state change types
* 
*/
enum {
    BGP4MP_IDLE = 1,
    BGP4MP_CONNECT = 2,
    BGP4MP_ACTIVE = 3,
    BGP4MP_OPENSENT = 4,
    BGP4MP_OPENCONFIRM = 5,
    BGP4MP_ESTABLISHED = 6
};

typedef struct {
    afi_t afi;
    size_t as_size;
    uint32_t as;
    union {
        struct in_addr  in;
        struct in6_addr in6;
    };
} peer_entry_t;

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
    MRT_ENOERR = 0,     ///< No error (success) guaranteed to be zero.
    MRT_EIO,            ///< Input/Output error during packet read.
    MRT_EINVOP,         ///< Invalid operation (e.g. write while reading packet).
    MRT_ENOMEM,         ///< Out of memory.
    MRT_EBADHDR,        ///< Bad MRT packet header.
    MRT_EBADTYPE,       ///< Bad MRT packet type.
    MRT_EBADPEERIDX     ///< Error encountered parsing associated Peer Index.
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
    case MRT_EBADPEERIDX:
        return "Bad Peer Index message";
    default:
        return "Unknown error";
    }
}

int mrterror(void);

int setmrtread(const void *data, size_t n);

int setmrtreadfd(int fd);

int setmrtreadfrom(io_rw_t *io);

int mrtclose(void);

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
    MRTBUFSIZ = 4096
};

/// @brief Packet reader/writer global status structure.
typedef struct mrt_msg_s {
    uint16_t flags;      ///< General status flags.
    int16_t err;         ///< Last error code.
    uint32_t bufsiz;     ///< Packet buffer capacity

    mrt_header_t hdr;

    peer_entry_t pe;      ///< Current peer entry
    unsigned char *peptr; ///< Raw peer entry pointer in current packet

    union {
        struct mrt_msg_t *peer_index;
        bgp_msg_t *bgp;
    };

    unsigned char *buf;  ///< Packet buffer base.
    unsigned char fastbuf[MRTBUFSIZ];  ///< Fast buffer to avoid malloc()s.
} mrt_msg_t;

int mrterror_r(mrt_msg_t *msg);

int mrtclose_r(mrt_msg_t *msg);

int setmrtread_r(mrt_msg_t *msg, const void *data, size_t n);

int setmrtreadfd_r(mrt_msg_t *msg, int fd);

int setmrtreadfrom_r(mrt_msg_t *msg, io_rw_t *io);

//header

mrt_header_t *getmrtheader_r(mrt_msg_t *msg);

int setmrtheaderv_r(mrt_msg_t *msg, const mrt_header_t *hdr, va_list va);

int setmrtheader_r(mrt_msg_t *msg, const mrt_header_t *hdr, ...);

// Peer Index

int setpeerents(const void *buf, size_t n);

int setpeerents_r(mrt_msg_t *msg, const void *buf, size_t n);

void *getpeerents(size_t *pn);

void *getpeerents_r(mrt_msg_t *msg, size_t *pn);

int startpeerents(void);

int startpeerents_r(mrt_msg_t *msg);

peer_entry_t *nextpeerent(void);

peer_entry_t *nextpeerent_r(mrt_msg_t *msg);

int putpeerent(const peer_entry_t *pe);

int putpeerent_r(mrt_msg_t *msg, const peer_entry_t *pe);

int endpeerents(void);

int endpeerents_r(mrt_msg_t *msg);

#endif

