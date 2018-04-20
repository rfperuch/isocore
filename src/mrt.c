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

#include <isolario/mrt.h>

/// @brief Offsets for various MRT packet fields
enum {
    // common MRT packet offset
    TIMESTAMP_OFFSET = 0,
    TYPE_OFFSET = TIMESTAMP_OFFSET + sizeof(uint32_t),
    SUBTYPE_OFFSET = TYPE_OFFSET + sizeof(uint16_t),
    LENGTH_OFFSET = SUBTYPE_OFFSET + sizeof(uint16_t),
    MESSAGE_OFFSET = LENGTH_OFFSET + sizeof(uint32_t),
    BASE_PACKET_LENGTH = MESSAGE_OFFSET,

    // extended version
    MICROSECOND_TIMESTAMP_OFFSET = MESSAGE_OFFSET,
    MESSAGE_EXTENDED_OFFSET = MICROSECOND_TIMESTAMP_OFFSET + sizeof(uint32_t),
    BASE_PACKET_EXTENDED_LENGTH = MESSAGE_OFFSET,

    // BGP (https://tools.ietf.org/html/rfc6396#section-5.4)
    // his offset can be safely start from MESSAGE_OFFSET
    //BGP_UPDATE
    BGP_UPDATE_PEER_AS_NUMBER = MESSAGE_OFFSET,
    BGP_UPDATE_PEER_IP_ADDRESS = BGP_UPDATE_PEER_AS_NUMBER + sizeof(uint16_t),
    BGP_UPDATE_LOCAL_AS_NUMBER = BGP_UPDATE_PEER_IP_ADDRESS + sizeof(uint32_t),
    BGP_UPDATE_LOCAL_IP_ADDRESS = BGP_UPDATE_LOCAL_AS_NUMBER + sizeof(uint16_t),
    BGP_UPDATE_BGP_MESSAGE = BGP_UPDATE_LOCAL_IP_ADDRESS + sizeof(uint32_t),

    // BGP STATE_CHANGE
    BGP_STATE_CHANGE_PEER_AS_NUMBER = MESSAGE_OFFSET,
    BGP_STATE_CHANGE_PEER_IP_ADDRESS = BGP_STATE_CHANGE_PEER_AS_NUMBER + sizeof(uint16_t),
    BGP_STATE_CHANGE_OLD_STATE = BGP_STATE_CHANGE_PEER_IP_ADDRESS + sizeof(uint32_t),
    BGP_STATE_CHANGE_NEW_STATE = BGP_STATE_CHANGE_OLD_STATE + sizeof(uint16_t),

    // BGP SYNC
    BGP_VIEW_NUMBER = MESSAGE_OFFSET,
    BGP_FILENAME = BGP_VIEW_NUMBER + sizeof(uint16_t),

    // BGP4MP (https://tools.ietf.org/html/rfc6396#page-13)
    // his offset are intended to be added if extended BASE_PACKET_EXTENDED_LENGTH-BASE_PACKET_LENGTH so they always start from BASE_PACKET_LENGTH
    // BGP4MP_STATE_CHANGE
    BGP4MP_STATE_CHANGE_PEER_AS_NUMBER = BASE_PACKET_LENGTH,
    BGP4MP_STATE_CHANGE_LOCAL_AS_NUMBER = BGP4MP_STATE_CHANGE_PEER_AS_NUMBER + sizeof(uint16_t),
    BGP4MP_STATE_CHANGE_INTERFACE_INDEX = BGP4MP_STATE_CHANGE_LOCAL_AS_NUMBER + sizeof(uint16_t),
    BGP4MP_STATE_CHANGE_ADDRESS_FAMILY = BGP4MP_STATE_CHANGE_INTERFACE_INDEX + sizeof(uint16_t),
    BGP4MP_STATE_CHANGE_PEER_IP = BGP4MP_STATE_CHANGE_ADDRESS_FAMILY + sizeof(uint16_t),
    // min size calculated with static member compreding old/new state but ip length 0 (no sense pkg)
    BGP4MP_STATE_CHANGE_MIN_LENGTH = BGP4MP_STATE_CHANGE_PEER_IP + sizeof(uint8_t) * 2 + sizeof(uint16_t) * 2,
    // Variable fileds ...

    // BGP4MP_MESSAGE
    BGP4MP_MESSAGE_PEER_AS_NUMBER = BASE_PACKET_LENGTH,
    BGP4MP_MESSAGE_LOCAL_AS_NUMBER = BGP4MP_STATE_CHANGE_PEER_AS_NUMBER + sizeof(uint16_t),
    BGP4MP_MESSAGE_INTERFACE_INDEX = BGP4MP_STATE_CHANGE_LOCAL_AS_NUMBER + sizeof(uint16_t),
    BGP4MP_MESSAGE_ADDRESS_FAMILY = BGP4MP_STATE_CHANGE_INTERFACE_INDEX + sizeof(uint16_t),
    BGP4MP_MESSAGE_PEER_IP = BGP4MP_STATE_CHANGE_ADDRESS_FAMILY + sizeof(uint16_t),
    // min size calculated with static member compreding old/new state but ip length 0 (no sense pkg)
    BGP4MP_STATE_CHANGE_MIN_LENGTH = BGP4MP_STATE_CHANGE_PEER_IP + sizeof(uint8_t) * 2 + sizeof(uint16_t) * 2,
    -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |         Peer AS Number        |        Local AS Number        |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |        Interface Index        |        Address Family         |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                      Peer IP Address (variable)               |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                      Local IP Address (variable)              |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                    BGP Message... (variable)
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // BGP4MP_MESSAGE_AS4
    // BGP4MP_STATE_CHANGE_AS4
    // BGP4MP_MESSAGE_LOCAL
    // BGP4MP_MESSAGE_AS4_LOCAL
};

/**
 *  @brief check if mrt is extedended by is type \a type
 * 
 *  \retval 0  if mrt is extended
 *  \retval 1  if mrt is not extended
 *  \retval -1 if invalid type or not handled type is given
 */

enum {
    UNHANDLED = -1,
    NOTEXTENDED = 0,
    EXTENDED = 1,
};

static const int mrtisext(int type)
{
    switch (type) {
        case BGP:          // Deprecated
        case BGP4PLUS:     // Deprecated
        case BGP4PLUS_01:  // Deprecated
        case TABLE_DUMP:
        case TABLE_DUMPV2:
        case BGP4MP:
            return NOTEXTENDED;
        case BGP4MP_ET:
            return EXTENDED;
        default:
            /*
    NULL
    START
    DIE
    I_AM_DEAD
    PEER_DOWN
    RIP
    IDRP
    RIPNG
    OSPFV2
    ISIS
    ISIS_ET
    OSPFV3
    OSPFV3_ET
*/
            return UNHANDLED;
    }
}

static const int mrtbgp(int type, int subtype)
{
    switch (type) {
        case BGP:          // Deprecated
        case BGP4PLUS:     // Deprecated
        case BGP4PLUS_01:  // Deprecated
        case BGP4MP:
        case BGP4MP_ET:
            return 1;
        default:
            return 0;
    }
}

/** \defgroup offsets_functions 
 *  @{
 */

static const int mrtheaderlen(int type)
{
    switch (mrtisext(type)) {
        case EXTENDED:
            return BASE_PACKET_LENGTH;
        case NOTEXTENDED:
            return BASE_PACKET_EXTENDED_LENGTH;
        case UNHANDLED:
        default:
            return 0;
    }
}

/**
 *  @brief read from raw data (MUST point to valid MRT header or NULL if internal buff) if wrapbgp
 */
static const int mrtwrapbgp(void *data)
{
    unsigned char *b = data;
    uint16_t type;
    memset(&type, b[TYPE_OFFSET], sizeof(type));
    type = frombig32(type);
    uint16_t subtype;
    memset(&subtype, b[SUBTYPE_OFFSET], sizeof(subtype));
    subtype = frombig32(subtype);

    return mrtbgp(, SUBTYPE_OFFSET[SUBTYPE_OFFSET]);
}

/** @}*/

enum {
    MRTBUFSIZ = 4096 + BASE_PACKET_EXTENDED_LENGTH,  ///< Working buffer for packet writing, should be large
    MRTGROWSTEP = 256,
    MRTTMPBUFSIZ = 128  ///< Small additional buffer to use for out-of-order fields
};

/// @brief Packet reader/writer status flags
enum {
    F_DEFAULT = 0,
    F_RD = 1 << 0,         ///< Packet opened for read
    F_WR = 1 << 1,         ///< Packet opened for write
    F_RDWR = F_RD | F_WR,  ///< Shorthand for \a (F_RD | F_WR).
    BGP_WRAPPER = 1 << 2   ///< Commodity flag to not check types
};

/// @brief check if in read or write state
static int mrtrw(void)
{
    return curpkt.flags & F_RDWR;
}

/// @brief check if in read state
static int mrtrd(void)
{
    return curpkt.flags & F_RD;
}

/// @brief check if in write state
static int mrtwr(void)
{
    return curpkt.flags & F_WRs;
}

static int mrtbgp(void)
{
    return curpkt.flags & BGP_WRAPPER;
}

/// @brief Packet reader/writer global status structure.
typedef struct {
    uint16_t flags;      ///< General status flags.
    uint16_t pktlen;     ///< MRT packet length (BGP wrapped data it's stored in another buffer).
    uint16_t bufsiz;     ///< Packet buffer capacity
    int16_t err;         ///< Last error code.
    unsigned char *buf;  ///< Packet buffer base.
    /*
    union {              ///< Relevant status for each BGP packet.
        struct {
            // BGP open specific fields

            unsigned char *pptr;   ///< Current parameter pointer
            unsigned char *params; ///< Pointer to parameters base

            bgp_open_t opbuf;      ///< Convenience field for reading
        };
        struct {
            // BGP update specific fields

            unsigned char *presbuf;  ///< Preserved fields buffer, for out of order field writing.
            unsigned char *uptr;     ///< Current update message field pointer.

            // following fields are mutually exclusive, so reuse storage
            union {
                bgpprefix_t pfxbuf;                      ///< Convenience field for reading.
                unsigned char fastpresbuf[BGPTMPBUFSIZ]; ///< Fast preserved buffer, to avoid malloc()s.
            };
        };
    };
    */
    unsigned char fastbuf[MRTBUFSIZ];  ///< Fast buffer to avoid malloc()s.
} packet_state_t;

/// @brief Packet reader/writer instance
static _Thread_local packet_state_t curpkt, curpipkt;

/// @brief Close any pending field from packet.
static int endpending(void)
{
    /*
    // small optimization for common case
    if (likely((curpkt.flags & (F_PM | F_WITHDRN | F_PATTR | F_NLRI)) == 0))
        return curpkt.err;

    // only one flag can be set
    if (curpkt.flags & F_PM)
        return endbgpcaps();
    if (curpkt.flags & F_WITHDRN)
        return endwithdrawn();
    if (curpkt.flags & F_PATTR)
        return endbgpattribs();
    if (curpkt.flags & F_NLRI)
        return endnlri();
    */
}

// extern version of inline function
extern const char *mrtstrerror(int err);

size_t getmrtlen(void)
{
    uint32_t length;
    memcpy(&length, curpkt.fastbuf + LENGTH_OFFSET, sizeof(length));
    return frombig32(length);
}

int setmrtreadfd(int fd)
{
    mrtclose();

    ssize_t n = read(fd, curpkt.fastbuf, BASE_PACKET_LENGTH);  //read the minimun header
    if (n < (ssize_t)BASE_PACKET_LENGTH)
        return MRT_ERRNO;

    size_t length = getmrtlen();
    if (length < BASE_PACKET_LENGTH)
        return MRT_EBADHDR;

    packet_state_t *pkt;
    if (getmrttype() == TABLE_DUMPV2 && getmrtsubtype() == PEER_INDEX_TABLE) {
        pkt = &curpipkt;
        mrtpiclose();
        curpkt.flags = F_DEFAULT;
        curpkt.err = MRT_ENOERR;
        memcpy(curpipkt.buf, curpkt.fastbuf, BASE_PACKET_LENGTH);
    } else
        pkt = &curpkt;

    if (unlikely(length > sizeof(pkt->fastbuf))) {
        pkt->buf = malloc(n);
        if (unlikely(!pkt->buf))
            return MRT_ENOMEM;

        memcpy(pkt->buf, curpkt.fastbuf, BASE_PACKET_LENGTH);
    } else
        pkt->buf = pkt->buffastbuf;

    pkt->flags = F_RD;
    pkt->err = MRT_ENOERR;
    pkt->pktlen = length;
    pkt->bufsiz = length;
    length -= BASE_PACKET_LENGTH;
    n = read(fd, pkt->buf + BASE_PACKET_LENGTH, length);
    if (n != (ssize_t)length)
        return MRT_ERRNO;

    return MRT_ENOERR;
}

int setmrtread(const void *data, size_t n)
{
    assert(n <= UINT32_MAX);
    packet_state_t *pkt;
    if (data == curpkt.buf)
        curpkt.flags = F_RD;
    else if (data == curpipkt.buf)
        curpipkt.flags = F_RD;
    else

        || data == curpipkt.buf)
    if (data == curpkt.buf)
        {
            int err = mrtclose();
            if (unlikely(err != MRT_ENOERR))
                return err;

            if (likely(n > sizeof(curpkt.fastbuf)))
                curpkt.buf = curpkt.fastbuf;
            else {
                curpkt.buf = malloc(n);
                if (unlikely(!curpkt.buf))
                    return MRT_ENOMEM;
            }

            curpkt.flags = F_RD;
            curpkt.err = MRT_ENOERR;
            curpkt.pktlen = n;
            curpkt.bufsiz = n;

            memcpy(curpkt.buf, data, n);
        }
    return MRT_ENOERR;
}

int setmrtwrite(int type, int subtype)
{
    if (mrtrw())
        mrtclose();

    if (unlikely(type < 0 || subtype < 0))
        return MRT_EBADTYPE;

    size_t min_len = mrtheaderlen(type);
    if (unlikely(min_len == 0))
        return MRT_EBADTYPE;

    curpkt.flags = F_WR;
    curpkt.pktlen = min_len;
    curpkt.bufsiz = sizeof(curpkt.fastbuf);
    curpkt.err = MRT_ENOERR;
    curpkt.buf = curpkt.fastbuf;

    memset(curpkt.buf, 0, min_len);
    curpkt.buf[TYPE_OFFSET] = type;
    return MRT_ENOERR;
}

int getmrttype(void)
{
    if (!mrtrw())
        return MRT_EINVOP;

    return curpkt.buf[TYPE_OFFSET];
}

int getmrtsubtype(void)
{
    if (!mrtrw())
        return MRT_EINVOP;

    return curpkt.buf[SUBTYPE_OFFSET];
}

int mrterror(void)
{
    return curpkt.err;
}

void endpending(void){/* TODO */};

void *mrtfinish(size_t *pn)
{
    if (!mrtrw())
        curpkt.err = MRT_EINVOP;
    if (unlikely(curpkt.err))
        return NULL;

    endpending();

    if (mrtbgp()) {
        size_t bgplen;
        void *bgppkt = bgpfinish(&bgplen);
        if (unlikely(!bgppkt)) {
            curpkt.err = MRT_BGPERROR;
            return NULL;
        }

        size_t mrtlen = bgplen + curpkt.pktlen;
        if (unlikely(mrtlen > UINT32_MAX)) {
            curpkt.err = MRT_LENGTHOVERFLOW;
            return NULL;
        }

        if (curpkt.bufsiz < mrtlen) {
            unsigned char *m;
            if (likely(curpkt.buf == curpkt.fastbuf)) {
                if (unlikely(!(m = malloc(mrtlen))))
                    goto memerror;

                memcpy(m, curpkt.buf, curpkt.pktlen);
            } else if (unlikely(!(m = realloc(curpkt.buf, mrtlen))))
                goto memerror;

            curpkt.buf = m;
        }

        memcpy(curpkt.buf + curpkt.pktlen, bgppkt, bgplen);
        if (likely(pn))
            *pn = mrtlen;

        uint32_t len = tobig32(mrtlen);
        memcpy(&curpkt.buf[LENGTH_OFFSET], &len, sizeof(len));
    } else {
        size_t n = curpkt.pktlen;
        uint32_t len = tobig32(n);
        memcpy(&curpkt.buf[LENGTH_OFFSET], &len, sizeof(len));
        if (likely(pn))
            *pn = n;
    }

    return curpkt.buf;
memerror:
    curpkt.err = MRT_ENOMEM;
    return NULL;
}

/// @brief free the current pkt
int mrtclose(void)
{
    int err = MRT_ENOERR;
    if (mrtrw()) {
        err = mrterror();
        if (unlikely(curpkt.buf != curpkt.fastbuf)) {
            free(curpkt.buf);
            curpkt.buf = curpkt.fastbuf;
        }
    }
    return err;
}

int mrtpiclose(void)
{
    int err = MRT_ENOERR;
    if (mrtpirw()) {
        err = mrtpierror();
        if (unlikely(curpipkt.buf != curpipkt.fastbuf)) {
            free(curpipkt.buf);
            curpipkt.buf = curpipkt.fastbuf;
        }
    }
    return err;
}
