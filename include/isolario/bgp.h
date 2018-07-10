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
 * @file isolario/bgp.h
 *
 * @brief Isolario BGP packet reading and writing routines.
 *
 * @note This file is guaranteed to include POSIX \a arpa/inet.h, standard
 *       \a stdint.h and isolario-specific \a netaddr.h and \a io.h.
 *       It may include additional standard and isolario-specific
 *       headers to provide its functionality.
 */

#ifndef ISOLARIO_BGP_H_
#define ISOLARIO_BGP_H_

#include <arpa/inet.h>
#include <isolario/bgpparams.h>
#include <isolario/bgpattribs.h>
#include <isolario/funcattribs.h>
#include <isolario/netaddr.h>
#include <isolario/io.h>  // also includes stddef.h
#include <stdint.h>

enum {
    BGP_FSM_IDLE        = 1,
    BGP_FSM_CONNECT     = 2,
    BGP_FSM_ACTIVE      = 3,
    BGP_FSM_OPENSENT    = 4,
    BGP_FSM_OPENCONFIRM = 5,
    BGP_FSM_ESTABLISHED = 6
};

enum {
    AS_TRANS = 23456
};

enum {
    BGP_VERSION = 4,

    BGP_HOLD_SECS = 180,

    // BGP flags

    BGPF_DEFAULT      = 0,       /// @brief Default flags for \a setbgpread(), copy the buffer.
    BGPF_NOCOPY       = 1 << 0,  /// @brief A flag for \a setbgpread(), does not copy the read-buffer.
    BGPF_ADDPATH      = 1 << 1,
    BGPF_ASN32BIT     = 1 << 2,
    BGPF_GUESSMRT     = 0,
    BGPF_STDMRT       = 1 << 3,
    BGPF_FULLMPREACH  = 1 << 4,
    BGPF_STRIPUNREACH = 1 << 5
};

/**
 * @defgroup BGP Border Gateway Procol packet creation
 *
 * @brief Basic functions to create and decode BGP packets.
 *
 * @{
 */

enum {
    BGP_BADTYPE = -1,

    BGP_OPEN = 1,
    BGP_UPDATE = 2,
    BGP_NOTIFICATION = 3,
    BGP_KEEPALIVE = 4,
    BGP_ROUTE_REFRESH = 5,
    BGP_CLOSE = 255
};

enum {
    BGP_ENOERR = 0,    ///< No error (success) guaranteed to be zero.
    BGP_EIO,           ///< Input/Output error during packet read.
    BGP_EINVOP,        ///< Invalid operation (e.g. write while reading packet).
    BGP_ENOMEM,        ///< Out of memory.
    BGP_EBADHDR,       ///< Bad BGP packet header.
    BGP_EBADTYPE,      ///< Unrecognized BGP packet type.
    BGP_EBADPARAMLEN,  ///< Open message has invalid parameters field length.
    BGP_EBADWDRWN,     ///< Update message has invalid Withdrawn field.
    BGP_EBADATTR,      ///< Update message has invalid Path Attributes field.
    BGP_EBADNLRI       ///< Update message has invalid NLRI field.
};

inline purefunc nonullreturn const char *bgpstrerror(int err)
{
    switch (err) {
    case BGP_ENOERR:
        return "Success";
    case BGP_EIO:
        return "I/O error";
    case BGP_EINVOP:
        return "Invalid operation";
    case BGP_ENOMEM:
        return "Out of memory";
    case BGP_EBADHDR:
        return "Bad BGP header";
    case BGP_EBADTYPE:
        return "Bad BGP packet type";
    case BGP_EBADPARAMLEN:
        return "Oversized or inconsistent BGP open parameters length";
    case BGP_EBADWDRWN:
        return "Oversized or inconsistent BGP update Withdrawn length";
    case BGP_EBADATTR:
        return "Malformed attribute list";
    case BGP_EBADNLRI:
        return "Oversized or inconsistent BGP update NLRI field";
    default:
        return "Unknown error";
    }
}

/**
 * @brief Initialize a BGP packet for read from pre-existing data.
 */
int setbgpread(const void *data, size_t n, int flags);

/**
 * @brief Read BGP from Unix file descriptor.
 */
int setbgpreadfd(int fd, int flags);

nonnull(1) int setbgpreadfrom(io_rw_t *io, int flags);

/**
 * @brief Initialize a BGP packet for writing a new packet of type \a type from scratch.
 */
int setbgpwrite(int type, int flags);

nonnull(1) int rebuildbgpfrommrt(const void *nlri, const void *data, size_t n, int flags);

/**
 * @brief Get BGP packet type from header.
 */
wur int getbgptype(void);

/**
 * @brief Get BGP packet length from header.
 */
wur size_t getbgplength(void);

/**
 * @brief Get BGP length and raw data.
 */
wur void *getbgpdata(size_t *pn);

int isbgpasn32bit(void);

int isbgpaddpath(void);

wur int bgperror(void);

void *bgpfinish(size_t *pn);

int bgpclose(void);

/** @} */

/**
 * @defgroup BGP_open Border Gateway Procol open packet writing and reading
 *
 * @{
 */

typedef struct {
    uint8_t version;
    uint16_t hold_time;
    uint16_t my_as;
    struct in_addr iden;
} bgp_open_t;

bgp_open_t *getbgpopen(void);

nonnull(1) int setbgpopen(const bgp_open_t *open);

void *getbgpparams(size_t *pn);

int setbgpparams(const void *params, size_t n);

/**
 * @brief Position BGP capability iterator to the first entry.
 *
 * This function must be called before reading or writing capabilities from
 * or to the current packet.
 *
 * @return \a BGP_ENOERR on success, an error code otherwise.
 */
int startbgpcaps(void);

/** @brief Read next BGP capability. */
bgpcap_t *nextbgpcap(void);

/** @brief Write a capability into a BGP open packet, length is in bytes, header included. */
nonnull(1) int putbgpcap(const bgpcap_t *cap);

int endbgpcaps(void);

/** @} */

/**
 * @defgroup BGP_update Border Gateway Procol update message reading and writing
 *
 * @{
 */

int setwithdrawn(const void *data, size_t n);

void *getwithdrawn(size_t *pn);

int startwithdrawn(void);

int startallwithdrawn(void);
/**
 * @warning This function returns either a \a netaddr_t or a \a netaddrap_t,
 *          depending on the package being ADDPATH enabled.
 *          The returned pointer may always be used as \a netaddr_t.
 */
void *nextwithdrawn(void);
/**
 * @warning \a p may be either a \a netaddr_t or a \a netaddrap_t, depending on
 *          the package being ADDPATH enabled.
 */
nonnull(1) int putwithdrawn(const void *p);

int endwithdrawn(void);

int setbgpattribs(const void *data, size_t n);

void *getbgpattribs(size_t *pn);

int startbgpattribs(void);

bgpattr_t *nextbgpattrib(void);

nonnull(1) int putbgpattrib(const bgpattr_t *attr);

int endbgpattribs(void);

int setnlri(const void *data, size_t n);

void *getnlri(size_t *pn);

int startnlri(void);

int startallnlri(void);
/**
 * @warning \a p may be either a \a netaddr_t or a \a netaddrap_t, depending on
 *          the package being ADDPATH enabled.
 */
nonnull(1) int putnlri(const void *p);
/**
 * @warning This function returns either a \a netaddr_t or a \a netaddrap_t,
 *          depending on the package being ADDPATH enabled.
 *          The returned pointer may always be used as \a netaddr_t.
 */
void *nextnlri(void);

int endnlri(void);

typedef struct {
    size_t as_size;
    int type, segno;
    uint32_t as;
} as_pathent_t;

int startaspath(void);

int startas4path(void);

int startrealaspath(void);

as_pathent_t *nextaspath(void);

int endaspath(void);

int startnhop(void);

netaddr_t *nextnhop(void);

int endnhop(void);

/** @} */

/**
 * @defgroup BGP_reentrant Reentrant version of the BGP API
 *
 * @brief A variation of the BGP API allowing elaboration of multiple
 *        messages in the same thread.
 *
 * @{
 */

enum {
    /**
     * @brief A good initial buffer size to store a BGP message.
     *
     * A buffer with this size should be enough to store most (if not all)
     * BGP messages without any reallocation, but code should be ready to
     * reallocate buffer to a larger size if necessary.
     *
     * @see [BGP extended messages draft](https://tools.ietf.org/html/draft-ietf-idr-bgp-extended-messages-24)
     */
    BGPBUFSIZ = 4096
};

/**
 * @brief BGP message structure.
 *
 * A structure encapsulating all the relevant status used to read or write a
 * BGP message.
 *
 * @warning This structure must be considered opaque, no field in this structure
 *          is to be accessed directly, use the appropriate functions instead!
 */
typedef struct {
    uint16_t flags;      ///< @private General status flags.
    uint16_t pktlen;     ///< @private Actual packet length.
    uint16_t bufsiz;     ///< @private Packet buffer capacity
    int16_t err;         ///< @private Last error code.
    unsigned char *buf;  ///< @private Packet buffer base.
    /// @private Relevant status for each BGP packet.
    union {
        /// @private BGP open specific fields
        struct {
            unsigned char *pptr;    ///< @private Current parameter pointer
            unsigned char *params;  ///< @private Pointer to parameters base

            bgp_open_t opbuf;  ///< @private Convenience field for reading
        };
        /// @private BGP update specific fields
        struct {
            unsigned char *ustart;   ///< @private Current update message field starting pointer.
            unsigned char *uptr;     ///< @private Current update message field pointer.
            unsigned char *uend;     ///< @private Current update message field ending pointer.

            /// @private following fields are mutually exclusive
            union {
                /// @private read-specific fields.
                struct {
                    netaddrap_t pfxbuf;     ///< @private Convenience field for reading prefixes.
                    union {
                        struct {
                            unsigned char *asptr, *asend;
                            unsigned char *as4ptr, *as4end;
                            uint8_t seglen;
                            uint8_t segi;
                            int16_t ascount;
                            as_pathent_t asp;
                        };
                        struct {
                            unsigned char *nhptr, *nhend;
                            unsigned char *mpnhptr, *mpnhend;
                            short mpfamily, mpbitlen;
                            struct in_addr nhbuf;
                        };
                    };

                    uint16_t offtab[16];  ///< @private Notable attributes offset table.
                };

                /// @private write-specific fields.
                struct {
                    /// @private Preserved buffer pointer, either \a fastpresbuf or malloc()ed
                    unsigned char *presbuf;
                    /// @private Fast preserve buffer, to avoid malloc()s.
                    unsigned char fastpresbuf[96];
                };
            };
        };
    };

    unsigned char fastbuf[BGPBUFSIZ];  ///< @private Fast buffer to avoid malloc()s.
} bgp_msg_t;

/**
 * @brief Retrieve a pointer to the thread local BGP message structure.
 */
nonullreturn wur bgp_msg_t *getbgp(void);

int isbgpasn32bit_r(bgp_msg_t *msg);

int isbgpaddpath_r(bgp_msg_t *msg);

nonnull(1) int setbgpread_r(bgp_msg_t *msg, const void *data, size_t n, int flags);

nonnull(1) int setbgpreadfd_r(bgp_msg_t *msg, int fd, int flags);

nonnull(1, 2) int setbgpreadfrom_r(bgp_msg_t *msg, io_rw_t *io, int flags);

nonnull(1) int setbgpwrite_r(bgp_msg_t *msg, int type, int flags);

nonnull(1) wur int getbgptype_r(bgp_msg_t *msg);

nonnull(1, 2) int rebuildbgpfrommrt_r(bgp_msg_t *msg, const void *nlri, const void *data, size_t n, int flags);

nonnull(1) wur size_t getbgplength_r(bgp_msg_t *msg);

nonnull(1) wur void *getbgpdata_r(bgp_msg_t *msg, size_t *pn);

nonnull(1) wur int bgperror_r(bgp_msg_t *msg);

nonnull(1) void *bgpfinish_r(bgp_msg_t *msg, size_t *pn);

nonnull(1) int bgpclose_r(bgp_msg_t *msg);

nonnull(1) bgp_open_t *getbgpopen_r(bgp_msg_t *msg);

nonnull(1) int setbgpopen_r(bgp_msg_t *msg, const bgp_open_t *open);

nonnull(1) void *getbgpparams_r(bgp_msg_t *msg, size_t *pn);

nonnull(1) int setbgpparams_r(bgp_msg_t *msg, const void *params, size_t n);

nonnull(1) int startbgpcaps_r(bgp_msg_t *msg);

nonnull(1) bgpcap_t *nextbgpcap_r(bgp_msg_t *msg);

nonnull(1, 2) int putbgpcap_r(bgp_msg_t *msg, const bgpcap_t *cap);

nonnull(1) int endbgpcaps_r(bgp_msg_t *msg);

nonnull(1) int setwithdrawn_r(bgp_msg_t *msg, const void *data, size_t n);

nonnull(1) void *getwithdrawn_r(bgp_msg_t *msg, size_t *pn);

nonnull(1) int startallwithdrawn_r(bgp_msg_t *msg);

nonnull(1) int startwithdrawn_r(bgp_msg_t *msg);
/**
 * @warning This function returns either a \a netaddr_t or a \a netaddrap_t,
 *          depending on the package being ADDPATH enabled.
 *          The returned pointer may always be used as \a netaddr_t.
 */
nonnull(1) void *nextwithdrawn_r(bgp_msg_t *msg);
/**
 * @warning \a p may be either a \a netaddr_t or a \a netaddrap_t, depending on
 *          the package being ADDPATH enabled.
 */
nonnull(1, 2) int putwithdrawn_r(bgp_msg_t *msg, const void *p);

nonnull(1) int endwithdrawn_r(bgp_msg_t *msg);

nonnull(1) int setbgpattribs_r(bgp_msg_t *msg, const void *data, size_t n);

nonnull(1) void *getbgpattribs_r(bgp_msg_t *msg, size_t *pn);

nonnull(1) int startbgpattribs_r(bgp_msg_t *msg);

nonnull(1) bgpattr_t *nextbgpattrib_r(bgp_msg_t *msg);

nonnull(1, 2) int putbgpattrib_r(bgp_msg_t *msg, const bgpattr_t *attr);

nonnull(1) int endbgpattribs_r(bgp_msg_t *msg);

nonnull(1) int setnlri_r(bgp_msg_t *msg, const void *data, size_t n);

nonnull(1) void *getnlri_r(bgp_msg_t *msg, size_t *pn);

nonnull(1) int startallnlri_r(bgp_msg_t *msg);

nonnull(1) int startnlri_r(bgp_msg_t *msg);

/**
 * @warning \a p may be either a \a netaddr_t or a \a netaddrap_t, depending on
 *          the package being ADDPATH enabled.
 */
nonnull(1, 2) int putnlri_r(bgp_msg_t *msg, const void *p);

/**
 * @warning This function returns either a \a netaddr_t or a \a netaddrap_t,
 *          depending on the package being ADDPATH enabled.
 *          The returned pointer may always be used as \a netaddr_t.
 */
nonnull(1) void *nextnlri_r(bgp_msg_t *msg);

nonnull(1) int endnlri_r(bgp_msg_t *msg);

nonnull(1) int startaspath_r(bgp_msg_t *msg);

nonnull(1) int startas4path_r(bgp_msg_t *msg);

nonnull(1) int startrealaspath_r(bgp_msg_t *msg);

nonnull(1) as_pathent_t *nextaspath_r(bgp_msg_t *msg);

nonnull(1) int endaspath_r(bgp_msg_t *msg);

nonnull(1) int startnhop_r(bgp_msg_t *msg);

nonnull(1) netaddr_t *nextnhop_r(bgp_msg_t *msg);

nonnull(1) int endnhop_r(bgp_msg_t *msg);

// utility functions for update packages, direct access to notable attributes

wur bgpattr_t *getbgporigin(void);
nonnull(1) wur bgpattr_t *getbgporigin_r(bgp_msg_t *msg);

wur bgpattr_t *getbgpnexthop(void);
nonnull(1) wur bgpattr_t *getbgpnexthop_r(bgp_msg_t *msg);

wur bgpattr_t *getbgpaggregator(void);
nonnull(1) wur bgpattr_t *getbgpaggregator_r(bgp_msg_t *msg);

wur bgpattr_t *getbgpatomicaggregate(void);
nonnull(1) wur bgpattr_t *getbgpatomicaggregate_r(bgp_msg_t *msg);

wur bgpattr_t *getbgpas4aggregator(void);
nonnull(1) wur bgpattr_t *getbgpas4aggregator_r(bgp_msg_t *msg);

wur bgpattr_t *getrealbgpaggregator(void);
nonnull(1) wur bgpattr_t *getrealbgpaggregator_r(bgp_msg_t *msg);

wur bgpattr_t *getbgpaspath(void);
nonnull(1) wur bgpattr_t *getbgpaspath_r(bgp_msg_t *msg);

wur bgpattr_t *getbgpas4path(void);
nonnull(1) wur bgpattr_t *getbgpas4path_r(bgp_msg_t *msg);

wur bgpattr_t *getbgpmpreach(void);
nonnull(1) wur bgpattr_t *getbgpmpreach_r(bgp_msg_t *msg);

wur bgpattr_t *getbgpmpunreach(void);
nonnull(1) wur bgpattr_t *getbgpmpunreach_r(bgp_msg_t *msg);

wur bgpattr_t *getbgpcommunities(void);
nonnull(1) wur bgpattr_t *getbgpcommunities_r(bgp_msg_t *msg);

wur bgpattr_t *getbgpexcommunities(void);
nonnull(1) wur bgpattr_t *getbgpexcommunities_r(bgp_msg_t *msg);

wur bgpattr_t *getbgplargecommunities(void);
nonnull(1) wur bgpattr_t *getbgplargecommunities_r(bgp_msg_t *msg);

/** @} */

#endif
