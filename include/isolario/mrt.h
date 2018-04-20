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
#include <isolario/bgp.h>

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
    MRT_ENOERR = 0,      ///< No error (success) guaranteed to be zero.
    BGP_ERRNO,           ///< System error (see errno).
    MRT_EINVOP,          ///< Invalid operation (e.g. write while reading packet).
    MRT_ENOMEM,          ///< Out of memory.
    MRT_EBADHDR,         ///< Bad BGP packet header.
    MRT_BGPERROR,        ///< Error in BGP wrapped
    MRT_LENGTHOVERFLOW,  ///< Overflow of MRT length + wrapped data length
    BGP_EBADTYPE,        ///< Unrecognized BGP packet type.
    BGP_EBADPARAMLEN,    ///< Open message has invalid parameters field length.
    BGP_EBADWDRWNLEN,    ///< Update message has invalid Withdrawn field length.
    BGP_EBADATTRSLEN,    ///< Update message has invalid Path Attributes field length.
    BGP_EBADNLRILEN      ///< Update message has invalid NLRI field length.
};

/**
* @brief table_dumpv2 sub types
* 
* [iana table dumpv2 Subtype Codes](https://www.iana.org/assignments/mrt/mrt.xhtml#table-dump-v2-subtype-codes)
* further detail at https://tools.ietf.org/html/rfc6396#section-4.3
* 
*/
enum class table_dumpv2_sub_type_t : std::uint_least16_t {
    PEER_INDEX_TABLE = 1,             /// RFC6396
    RIB_IPV4_UNICAST = 2,             /// RFC6396
    RIB_IPV4_MULTICAST = 3,           /// RFC6396
    RIB_IPV6_UNICAST = 4,             /// RFC6396
    RIB_IPV6_MULTICAST = 5,           /// RFC6396
    RIB_GENERIC = 6,                  /// RFC6396
    GEO_PEER_TABLE = 7,               /// RFC6397
    RIB_IPV4_UNICAST_ADDPATH = 8,     /// RFC8050
    RIB_IPV4_MULTICAST_ADDPATH = 9,   /// RFC8050
    RIB_IPV6_UNICAST_ADDPATH = 10,    /// RFC8050
    RIB_IPV6_MULTICAST_ADDPATH = 11,  /// RFC8050
    RIB_GENERIC_ADDPATH = 12          /// RFC8050
};

inline const char *bgpstrerror(int err)
{
    switch (err) {
        case MRT_ENOERR:
            return "Success";
        case MRT_ERRNO:
            return "System error";
        case MRT_EINVOP:
            return "Invalid operation";
        case MRT_ENOMEM:
            return "Out of memory";
        case MRT_EBADHDR:
            return "Bad BGP header";
        case MRT_EBADTYPE:
            return "Bad MRT packet type";
        case MRT_BGPERROR:
            return "Error in wrapped BGP";
        case MRT_LENGTHOVERFLOW:
            return "Error in wrapped BGP";
            /*
    case BGP_EBADPARAMLEN:
        return "Oversized or inconsistent BGP open parameters length";
    case BGP_EBADWDRWNLEN:
        return "Oversized or inconsistent BGP update Withdrawn length";
    case BGP_EBADATTRSLEN:
        return "Oversized or inconsistent BGP update Path Attributes length";
    case BGP_EBADNLRILEN:
        return "Oversized or inconsistent BGP update NLRI length";
        */
        default:
            return "Unknown error";
    }
}

/**
 * @brief Read MRT from Unix file descriptor.
 */
int setmrtreadfd(int fd);

/**
 * @brief Initialize a MRT packet for read from pre-existing data.
 */
int setmrtread(const void *data, size_t n);

/**
 * @brief Initialize a MRT packet for writing a new packet white type \a type and subtype \a subtype from scratch.
 */
int setmrtwrite(int type, int subtype);

/**
 * @brief Get MRT packet type from header.
 */
int getmrttype(void);

/**
 * @brief Get MRT packet subtype from header.
 */
int getmrtsubtype(void);

/**
 * @brief Get MRT packet length from header.
 */
size_t getmrtlen(void);

int mrterror(void);

void *mrtfinish(size_t *pn);

int mrtclose(void);

typedef struct {
    uint32_t timestamp;
    uint16_t type;
    uint16_t subtype;
    uint32_t length;
    uint32_t microseconds;
} mrt_header_t;

/**
*   @brief BGP4_mp types
* 
* further detail at https://tools.ietf.org/html/rfc6396#section-4.2
* fand for extensions https://tools.ietf.org/html/rfc8050#page-2
*/
/// [iana bgp4mp Subtype Codes](https://www.iana.org/assignments/mrt/mrt.xhtml#BGP4MP-codes)
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

#endif
