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
 *       \a stddef.h and \a stdint.h and isolario-specific \a bgpprefix.h.
 *       It may include additional standard and isolario-specific
 *       headers to provide its functionality.
 */

#ifndef ISOLARIO_BGP_H_
#define ISOLARIO_BGP_H_

#include <arpa/inet.h>
#include <isolario/bgpprefix.h>
#include <stddef.h>
#include <stdint.h>

enum {
    BGP_FSM_IDLE         = 1,
    BGP_FSM_CONNECT      = 2,
    BGP_FSM_ACTIVE       = 3,
    BGP_FSM_OPENSENT     = 4,
    BGP_FSM_OPENCONFIRM  = 5,
    BGP_FSM_ESTABILISHED = 6
};

enum {
    AS_TRANS = 23456
};

enum {
    BGP_VERSION   = 4,

    BGP_HOLD_SECS = 180
};

/**
* @defgroup BGP Border Gateway Procol packet creation
*
* @brief Basic functions to create and decode BGP packets.
*
* @{
*/

enum {
    BGP_BADTYPE       = -1,

    BGP_OPEN          = 1,
    BGP_UPDATE        = 2,
    BGP_NOTIFICATION  = 3,
    BGP_KEEPALIVE     = 4,
    BGP_ROUTE_REFRESH = 5,
    BGP_CLOSE         = 255
};

enum {
    BGP_ENOERR = 0,    ///< No error (success) guaranteed to be zero.
    BGP_ERRNO,         ///< System error (see errno).
    BGP_EINVOP,        ///< Invalid operation (e.g. write while reading packet).
    BGP_ENOMEM,        ///< Out of memory.
    BGP_EBADHDR,       ///< Bad BGP packet header.
    BGP_EBADTYPE,      ///< Unrecognized BGP packet type.
    BGP_EBADPARAMLEN,  ///< Open message has invalid parameters field length.
    BGP_EBADWDRWNLEN,  ///< Update message has invalid Withdrawn field length.
    BGP_EBADATTRSLEN,  ///< Update message has invalid Path Attributes field length.
    BGP_EBADNLRILEN    ///< Update message has invalid NLRI field length.
};

inline const char *bgpstrerror(int err)
{
    switch (err) {
    case BGP_ENOERR:
        return "Success";
    case BGP_ERRNO:
        return "System error";
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
    case BGP_EBADWDRWNLEN:
        return "Oversized or inconsistent BGP update Withdrawn length";
    case BGP_EBADATTRSLEN:
        return "Oversized or inconsistent BGP update Path Attributes length";
    case BGP_EBADNLRILEN:
        return "Oversized or inconsistent BGP update NLRI length";
    default:
        return "Unknown error";
    }
}

/**
 * @brief Read BGP from Unix file descriptor.
 */
int setbgpreadfd(int fd);

/**
 * @brief Initialize a BGP packet for read from pre-existing data.
 */
int setbgpread(const void *data, size_t n);

/**
 * @brief Initialize a BGP packet for writing a new packet of type \a type from scratch.
 */
int setbgpwrite(int type);

/**
 * @brief Get BGP packet type from header.
 */
int getbgptype(void);

int bgperror(void);

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

int setbgpopen(const bgp_open_t *open);

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
void *nextbgpcap(size_t *pn);

/** @brief Write a capability into a BGP open packet, length is in bytes, header included. */
int putbgpcap(const void *data, size_t n);

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

bgpprefix_t *nextwithdrawn(void);

int putwithdrawn(const bgpprefix_t *p);

int endwithdrawn(void);

int setbgpattribs(const void *data, size_t n);

void *getbgpattribs(size_t *pn);

int startbgpattribs(void);

void *nextbgpattrib(size_t *pn);

int putbgpattrib(const void *attr, size_t n);

int endbgpattribs(void);

int setnlri(const void *data, size_t n);

void *getnlri(size_t *pn);

int startnlri(void);

int putnlri(const bgpprefix_t *p);

bgpprefix_t *nextnlri(void);

int endnlri(void);

/** @} */

#endif

