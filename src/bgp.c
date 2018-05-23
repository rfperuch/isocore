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

#include <assert.h>
#include <isolario/bgp.h>
#include <isolario/branch.h>
#include <isolario/endian.h>
#include <isolario/util.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum { BGPGROWSTEP  = 256 };

/// @brief BGP packet marker, prepended to any packet
static const unsigned char bgp_marker[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

/// @brief Packet reader/writer status flags
enum {
    F_RD = 1 << 0,        ///< Packet opened for read
    F_WR = 1 << 1,        ///< Packet opened for write
    F_RDWR = F_RD | F_WR, ///< Shorthand for \a (F_RD | F_WR).

    // Open packet flags
    F_PM = 1 << 2,  ///< Reading/writing BGP open parameters

    // Update packet flags
    F_WITHDRN = 1 << 3,  ///< Reading/writing Withdrown field
    F_PATTR   = 1 << 4,  ///< Reading/writing Path Attributes field
    F_NLRI    = 1 << 5   ///< Reading/writing NLRI field

};

/// @brief Offsets for various BGP packet fields
enum {
    // common BGP packet offset
    LENGTH_OFFSET = sizeof(bgp_marker),
    TYPE_OFFSET   = LENGTH_OFFSET + sizeof(uint16_t),

    BASE_PACKET_LENGTH = TYPE_OFFSET + sizeof(uint8_t),

    // BGP open packet
    VERSION_OFFSET       = TYPE_OFFSET + sizeof(uint8_t),
    MY_AS_OFFSET         = VERSION_OFFSET + sizeof(uint8_t),
    HOLD_TIME_OFFSET     = MY_AS_OFFSET + sizeof(uint16_t),
    IDEN_OFFSET          = HOLD_TIME_OFFSET + sizeof(uint16_t),
    PARAMS_LENGTH_OFFSET = IDEN_OFFSET + sizeof(uint32_t),
    PARAMS_OFFSET        = PARAMS_LENGTH_OFFSET + sizeof(uint8_t),

    MIN_OPEN_LENGTH = PARAMS_OFFSET,

    // BGP notification packet
    ERROR_CODE_OFFSET    = TYPE_OFFSET + sizeof(uint8_t),
    ERROR_SUBCODE_OFFSET = ERROR_CODE_OFFSET + sizeof(uint8_t),

    MIN_NOTIFICATION_LENGTH = ERROR_SUBCODE_OFFSET + sizeof(uint8_t),

    // BGP update packet
    MIN_UPDATE_LENGTH = BASE_PACKET_LENGTH + 2 * sizeof(uint16_t),

    // BGP route refresh packet
    ROUTE_REFRESH_LENGTH = BASE_PACKET_LENGTH + sizeof(afi_safi_t)
};

/// @brief Global thread BGP message instance.
static _Thread_local bgp_msg_t curmsg;

// Minimum packet size table (see setbgpwrite()) ===============================

static const uint8_t bgp_minlengths[] = {
    [BGP_OPEN]          = MIN_OPEN_LENGTH,
    [BGP_UPDATE]        = MIN_UPDATE_LENGTH,
    [BGP_NOTIFICATION]  = MIN_NOTIFICATION_LENGTH,
    [BGP_KEEPALIVE]     = BASE_PACKET_LENGTH,
    [BGP_ROUTE_REFRESH] = ROUTE_REFRESH_LENGTH,
    [BGP_CLOSE]         = BASE_PACKET_LENGTH
};

/// @brief Close any pending field from packet.
static int endpending(bgp_msg_t *msg)
{
    // small optimization for common case
    if (likely((msg->flags & (F_PM | F_WITHDRN | F_PATTR | F_NLRI)) == 0))
        return msg->err;

    // only one flag can be set
    if (msg->flags & F_PM)
        return endbgpcaps_r(msg);
    if (msg->flags & F_WITHDRN)
        return endwithdrawn_r(msg);
    if (msg->flags & F_PATTR)
        return endbgpattribs_r(msg);

    assert(msg->flags & F_NLRI);
    return endnlri_r(msg);
}

// General functions ===========================================================

// extern version of inline function
extern const char *bgpstrerror(int err);

/// @brief Check for a \a flags operation on a BGP packet of type \a type.
static int checktype(bgp_msg_t *msg, int type, int flags)
{
    if (unlikely(getbgptype_r(msg) != type))
        msg->err = BGP_EINVOP;
    if (unlikely((msg->flags & flags) != flags))
        msg->err = BGP_EINVOP;

    return msg->err;
}

/// @brief Check for any \a flags operation on a BGP packet of type \a type.
static int checkanytype(bgp_msg_t *msg, int type, int flags)
{
    if (unlikely(getbgptype_r(msg) != type))
        msg->err = BGP_EINVOP;
    if (unlikely((msg->flags & flags) == 0))
        msg->err = BGP_EINVOP;

    return msg->err;
}

bgp_msg_t *getbgp(void)
{
    return &curmsg;
}

int setbgpread(const void *data, size_t n)
{
    return setbgpread_r(&curmsg, data, n);
}

int setbgpread_r(bgp_msg_t *msg, const void *data, size_t n)
{
    assert(n <= UINT16_MAX);
    if (msg->flags & F_RDWR)
        bgpclose_r(msg);

    msg->buf = msg->fastbuf;
    if (unlikely(n > sizeof(msg->fastbuf)))
        msg->buf = malloc(n);

    if (unlikely(!msg->buf))
        return BGP_ENOMEM;

    msg->flags = F_RD;
    msg->err = BGP_ENOERR;
    msg->pktlen = n;
    msg->bufsiz = n;

    memcpy(msg->buf, data, n);
    return BGP_ENOERR;
}

int setbgpreadfd(int fd)
{
    return setbgpreadfd_r(&curmsg, fd);
}

int setbgpreadfd_r(bgp_msg_t *msg, int fd)
{
    io_rw_t io = IO_FD_INIT(fd);
    return setbgpreadfrom_r(msg, &io);
}

int setbgpreadfrom(io_rw_t *io)
{
    return setbgpreadfrom_r(&curmsg, io);
}

int setbgpreadfrom_r(bgp_msg_t *msg, io_rw_t *io)
{
    if (msg->flags & F_RDWR)
        bgpclose_r(msg);

    unsigned char hdr[BASE_PACKET_LENGTH];
    if (io->read(io, hdr, sizeof(hdr)) != sizeof(hdr))
        return BGP_EIO;

    uint16_t len;
    memcpy(&len, &hdr[LENGTH_OFFSET], sizeof(len));
    len = frombig16(len);

    if (memcmp(hdr, bgp_marker, sizeof(bgp_marker)) != 0)
        return BGP_EBADHDR;
    if (len < BASE_PACKET_LENGTH)
        return BGP_EBADHDR;

    msg->buf = msg->fastbuf;
    if (unlikely(len > sizeof(msg->fastbuf)))
        msg->buf = malloc(len);
    if (unlikely(!msg->buf))
        return BGP_ENOMEM;

    memcpy(msg->buf, hdr, sizeof(hdr));
    size_t n = len - sizeof(hdr);
    if (io->read(io, &msg->buf[sizeof(hdr)], n) != n)
        return BGP_EIO;

    msg->flags = F_RD;
    msg->err = BGP_ENOERR;
    msg->pktlen = len;
    msg->bufsiz = len;
    return BGP_ENOERR;
}

int setbgpwrite(int type)
{
    return setbgpwrite_r(&curmsg, type);
}

int setbgpwrite_r(bgp_msg_t *msg, int type)
{
    if (msg->flags & F_RDWR)
        bgpclose_r(msg);

    if (unlikely(type < 0 || (unsigned int) type >= nelems(bgp_minlengths)))
        return BGP_EBADTYPE;

    size_t min_len = bgp_minlengths[type];
    if (unlikely(min_len == 0))
        return BGP_EBADTYPE;

    msg->flags = F_WR;
    msg->pktlen = min_len;
    msg->bufsiz = sizeof(msg->fastbuf);
    msg->err = BGP_ENOERR;
    msg->buf = msg->fastbuf;

    memset(msg->buf, 0, min_len);
    memcpy(msg->buf, bgp_marker, sizeof(bgp_marker));
    msg->buf[TYPE_OFFSET] = type;
    return BGP_ENOERR;
}

int getbgptype(void)
{
    return getbgptype_r(&curmsg);
}

int getbgptype_r(bgp_msg_t *msg)
{
    if (unlikely((msg->flags & F_RDWR) == 0))
        return BGP_BADTYPE;

    return msg->buf[TYPE_OFFSET];
}

size_t getbgplength(void)
{
    return getbgplength_r(&curmsg);
}

size_t getbgplength_r(bgp_msg_t *msg)
{
    if (unlikely((msg->flags & F_RD) == 0))
        msg->err = BGP_EINVOP;
    if (unlikely(msg->err))
        return 0;

    uint16_t len;
    memcpy(&len, &msg->buf[LENGTH_OFFSET], sizeof(len));
    return frombig16(len);
}

int bgperror(void)
{
    return bgperror_r(&curmsg);
}

int bgperror_r(bgp_msg_t *msg)
{
    return msg->err;
}

void *bgpfinish(size_t *pn)
{
    return bgpfinish_r(&curmsg, pn);
}

void *bgpfinish_r(bgp_msg_t *msg, size_t *pn)
{
    if ((msg->flags & F_WR) == 0)  // XXX: make it a function
        msg->err = BGP_EINVOP;
    if (unlikely(msg->err))
        return NULL;

    endpending(msg);

    size_t n = msg->pktlen;

    uint16_t len = tobig16(n);
    memcpy(&msg->buf[LENGTH_OFFSET], &len, sizeof(len));
    if (likely(pn))
        *pn = n;

    msg->flags &= ~F_WR;
    msg->flags |= F_RD; //allow reading from this message in the future

    return msg->buf;
}

int bgpclose(void)
{
    return bgpclose_r(&curmsg);
}

int bgpclose_r(bgp_msg_t *msg)
{
    int err = BGP_ENOERR;
    if (msg->flags & F_RDWR) {
        err = bgperror_r(msg);
        if (unlikely(msg->buf != msg->fastbuf))
            free(msg->buf);

        memset(msg, 0, sizeof(*msg));  // XXX: optimize
    }
    return err;
}

// Open message read/write functions ===========================================

bgp_open_t *getbgpopen(void)
{
    return getbgpopen_r(&curmsg);
}

bgp_open_t *getbgpopen_r(bgp_msg_t *msg)
{
    if (checktype(msg, BGP_OPEN, F_RD))
        return NULL;

    bgp_open_t *op = &msg->opbuf;

    op->version = msg->buf[VERSION_OFFSET];
    memcpy(&op->hold_time, &msg->buf[HOLD_TIME_OFFSET], sizeof(op->hold_time));
    op->hold_time = frombig16(op->hold_time);
    memcpy(&op->my_as, &msg->buf[MY_AS_OFFSET], sizeof(op->my_as));
    op->my_as = frombig16(op->my_as);
    memcpy(&op->iden.s_addr, &msg->buf[IDEN_OFFSET], sizeof(op->iden.s_addr));

    return op;
}

int setbgpopen(const bgp_open_t *op)
{
    return setbgpopen_r(&curmsg, op);
}

int setbgpopen_r(bgp_msg_t *msg, const bgp_open_t *op)
{
    if (checktype(msg, BGP_OPEN, F_WR))
        return msg->err;

    msg->buf[VERSION_OFFSET] = op->version;

    uint16_t hold_time = tobig16(op->hold_time);
    memcpy(&msg->buf[HOLD_TIME_OFFSET], &hold_time, sizeof(hold_time));

    uint16_t my_as = tobig16(op->my_as);
    memcpy(&msg->buf[MY_AS_OFFSET], &my_as, sizeof(my_as));
    memcpy(&msg->buf[IDEN_OFFSET], &op->iden.s_addr, sizeof(op->iden.s_addr));
    return BGP_ENOERR;
}

void *getbgpparams(size_t *pn)
{
    return getbgpparams_r(&curmsg, pn);
}

void *getbgpparams_r(bgp_msg_t *msg, size_t *pn)
{
    if (checkanytype(msg, BGP_OPEN, F_RDWR))
        return NULL;

    if (likely(pn))
        *pn = msg->buf[PARAMS_LENGTH_OFFSET];

    return &msg->buf[PARAMS_OFFSET];
}

int setbgpparams(const void *data, size_t n)
{
    return setbgpparams_r(&curmsg, data, n);
}

int setbgpparams_r(bgp_msg_t *msg, const void *data, size_t n)
{
    if (checktype(msg, BGP_OPEN, F_WR))
        return msg->err;

    // TODO no assert but actual check
    assert(n <= PARAMS_SIZE_MAX);

    msg->buf[PARAM_LENGTH_OFFSET] = n;
    memcpy(&msg->buf[PARAMS_OFFSET], data, n);

    msg->pktlen = PARAMS_OFFSET + n;
    return BGP_ENOERR;
}

int startbgpcaps(void)
{
    return startbgpcaps_r(&curmsg);
}

int startbgpcaps_r(bgp_msg_t *msg)
{
    if (checkanytype(msg, BGP_OPEN, F_RDWR))
        return msg->err;

    endpending(msg);

    msg->flags |= F_PM;
    msg->params = getbgpparams_r(msg, NULL);
    msg->pptr   = msg->params;
    return BGP_ENOERR;
}

bgpcap_t *nextbgpcap(void)
{
    return nextbgpcap_r(&curmsg);
}

bgpcap_t *nextbgpcap_r(bgp_msg_t *msg)
{
    if (checktype(msg, BGP_OPEN, F_RD | F_PM))
        return NULL;

    size_t n;
    unsigned char *base  = getbgpparams_r(msg, &n);
    unsigned char *limit = base + n;
    unsigned char *end   = msg->params + PARAM_HEADER_SIZE + msg->params[1];
    unsigned char *ptr   = msg->pptr;
    if (ptr == end)
        msg->params = end;  // move to next parameter

    // skip uninteresting parameters
    while (ptr == msg->params) {
        if (ptr >= limit) {
            // end of parameters in this packet
            if (unlikely(ptr > limit))
                msg->err = BGP_EBADPARAMLEN;

            return NULL;
        }
        if (ptr[0] == CAPABILITY_CODE) {
            // found
            ptr += PARAM_HEADER_SIZE;
            break;
        }

        // next parameter
        ptr = end;
        msg->params = end;
        end = ptr + PARAM_HEADER_SIZE + ptr[1];
    }

    if (unlikely(ptr + ptr[1] + CAPABILITY_HEADER_SIZE > end)) {
        // bad packet
        msg->err = BGP_EBADPARAMLEN;
        return NULL;
    }

    // advance to next parameter
    msg->pptr = ptr + CAPABILITY_HEADER_SIZE + ptr[1];
    return (bgpcap_t *) ptr;
}

int putbgpcap(const bgpcap_t *cap)
{
    return putbgpcap_r(&curmsg, cap);
}

int putbgpcap_r(bgp_msg_t *msg, const bgpcap_t *cap)
{
    if (checktype(msg, BGP_OPEN, F_WR | F_PM))
        return msg->err;

    unsigned char *ptr = msg->pptr;
    if (ptr == msg->params) {
        // first capability in parameter
        ptr[PARAM_CODE_OFFSET]   = CAPABILITY_CODE;
        ptr[PARAM_LENGTH_OFFSET] = 0; // update later
        ptr += PARAM_HEADER_SIZE;

        msg->pktlen += PARAM_HEADER_SIZE;
    }

    static_assert(BGPBUFSIZ >= MIN_OPEN_LENGTH + 0xff, "code assumes putbgpcap() can't overflow BGPBUFSIZ");

    size_t n = CAPABILITY_HEADER_SIZE + cap->len;
    // TODO actual check rather than assert()
    assert(n <= CAPABILITY_SIZE_MAX);
    memcpy(ptr, cap, n);
    ptr += n;

    msg->pktlen += n;
    msg->pptr = ptr;
    return BGP_ENOERR;
}

int endbgpcaps(void)
{
    return endbgpcaps_r(&curmsg);
}

int endbgpcaps_r(bgp_msg_t *msg)
{
    if (checktype(msg, BGP_OPEN, F_PM))
        return msg->err;

    if (msg->flags & F_WR) {
        // write length for the last parameter
        unsigned char *ptr = msg->pptr;
        msg->params[1] = (ptr - msg->params) - PARAM_HEADER_SIZE;

        // write length for the entire parameter list
        const unsigned char *base = getbgpparams_r(msg, NULL);
        size_t n = ptr - base;

        // TODO actual check rather than assert()
        assert(n <= PARAM_LENGTH_MAX);
        msg->buf[PARAMS_LENGTH_OFFSET] = n;
    }

    msg->flags &= ~F_PM;
    return BGP_ENOERR;
}

// Update message read/write functions =========================================

static int bgpensure(bgp_msg_t *msg, size_t len)
{
    len += msg->pktlen;
    if (unlikely(len > msg->bufsiz)) {
        // FIXME if len > 0xffff then oversized BGP packet (4K for regular BGP)!
        len += BGPGROWSTEP;
        if (unlikely(len > UINT16_MAX))
            len = UINT16_MAX;

        unsigned char *buf = msg->buf;
        if (buf == msg->fastbuf)
            buf = NULL;

        unsigned char *larger = realloc(buf, len);
        if (unlikely(!larger)) {
            msg->err = BGP_ENOMEM;
            return false;
        }

        if (!buf)
            memcpy(larger, msg->buf, msg->pktlen);

        msg->buf    = larger;
        msg->bufsiz = len;
    }

    return true;
}

static int bgppreserve(bgp_msg_t *msg, const unsigned char *from)
{
    unsigned char *end = &msg->buf[msg->pktlen];
    size_t size = end - from;

    msg->presbuf = msg->fastpresbuf;
    if (size > sizeof(msg->fastpresbuf)) {
        msg->presbuf = malloc(size);
        if (unlikely(!msg->presbuf)) {
            msg->err = BGP_ENOMEM;
            return msg->err;
        }
    }

    memcpy(msg->presbuf, from, size);
    return BGP_ENOERR;
}

static void bgprestore(bgp_msg_t *msg)
{
    unsigned char *end = &msg->buf[msg->pktlen];
    size_t n = end - msg->uptr;

    memcpy(msg->uptr, msg->presbuf, n);
    if (msg->presbuf != msg->fastpresbuf)
        free(msg->presbuf);
}

int startwithdrawn(void)
{
    return startwithdrawn_r(&curmsg);
}

int startwithdrawn_r(bgp_msg_t *msg)
{
    if (checkanytype(msg, BGP_UPDATE, F_RDWR))
        return msg->err;

    endpending(msg);

    size_t n;
    unsigned char *ptr = getwithdrawn_r(msg, &n);
    if (msg->flags & F_WR) {

        if (bgppreserve(msg, ptr + n) != BGP_ENOERR)
            return msg->err;

        msg->pktlen -= n;
    }

    msg->uptr   = ptr;
    msg->ustart = ptr;
    msg->flags |= F_WITHDRN;
    return BGP_ENOERR;
}

int setwithdrawn(const void *data, size_t n)
{
    return setwithdrawn_r(&curmsg, data, n);
}

int setwithdrawn_r(bgp_msg_t *msg, const void *data, size_t n)
{
    if (checktype(msg, BGP_UPDATE, F_WR))
        return msg->err;

    uint16_t old_size;

    unsigned char *ptr = &msg->buf[BASE_PACKET_LENGTH];
    memcpy(&old_size, ptr, sizeof(old_size));
    old_size = frombig16(old_size);

    if (n > old_size && !bgpensure(msg, n - old_size))
        return msg->err;

    unsigned char *start = ptr + sizeof(old_size) + old_size;
    unsigned char *end   = msg->buf + msg->pktlen;
    size_t bufsiz        = end - start;

    unsigned char buf[bufsiz];
    memcpy(buf, start, bufsiz);

    uint16_t new_size = tobig16(n);  // safe, since bgpensure() hasn't failed
    memcpy(ptr, &new_size, sizeof(new_size));

    ptr += sizeof(new_size);
    memcpy(ptr, data, n);

    ptr += n;
    memcpy(ptr, buf, bufsiz);

    msg->pktlen -= old_size;
    msg->pktlen += n;
    return BGP_ENOERR;
}

void *getwithdrawn(size_t *pn)
{
    return getwithdrawn_r(&curmsg, pn);
}

void *getwithdrawn_r(bgp_msg_t *msg, size_t *pn)
{
    if (checkanytype(msg, BGP_UPDATE, F_RDWR))
        return NULL;

    uint16_t len;
    unsigned char *ptr = &msg->buf[BASE_PACKET_LENGTH];
    if (likely(pn)) {
        memcpy(&len, ptr, sizeof(len));
        *pn = frombig16(len);
    }

    ptr += sizeof(len);
    return ptr;
}

netaddr_t *nextwithdrawn(void)
{
    return nextwithdrawn_r(&curmsg);
}

netaddr_t *nextwithdrawn_r(bgp_msg_t *msg)
{
    if (checktype(msg, BGP_UPDATE, F_RD | F_WITHDRN))
        return NULL;

    uint16_t wlen;
    memcpy(&wlen, msg->ustart - sizeof(wlen), sizeof(wlen));
    wlen = frombig16(wlen);

    unsigned char *end = msg->ustart + wlen;
    if (msg->uptr == end)
        return NULL;

    memset(&msg->pfxbuf, 0, sizeof(msg->pfxbuf));

    size_t bitlen = *msg->uptr++;
    size_t n = naddrsize(bitlen);
    if (msg->uptr + n > end) {
        msg->err = BGP_EBADWDRWN;
        return NULL;
    }
    makenaddr(&msg->pfxbuf, msg->uptr, bitlen);

    msg->uptr += n;
    return &msg->pfxbuf;
}

int putwithdrawn(const netaddr_t *p)
{
    return putwithdrawn_r(&curmsg, p);
}

int putwithdrawn_r(bgp_msg_t *msg, const netaddr_t *p)
{
    if (checktype(msg, BGP_UPDATE, F_WR | F_WITHDRN))
        return msg->err;

    size_t len = naddrsize(p->bitlen);
    if (unlikely(!bgpensure(msg, len + 1)))
        return msg->err;

    *msg->uptr++ = p->bitlen;
    memcpy(msg->uptr, p->bytes, len);
    msg->uptr   += len;
    msg->pktlen += len + 1;
    return BGP_ENOERR;
}

int endwithdrawn(void)
{
    return endwithdrawn_r(&curmsg);
}

int endwithdrawn_r(bgp_msg_t *msg)
{
    if (checktype(msg, BGP_UPDATE, F_WITHDRN))
        return msg->err;

    if (msg->flags & F_WR) {
        bgprestore(msg);
        uint16_t len = tobig16(msg->uptr - msg->ustart);
        memcpy(msg->ustart - sizeof(len), &len, sizeof(len));
    }

    msg->flags &= ~F_WITHDRN;
    return BGP_ENOERR;
}

void *getbgpattribs(size_t *pn)
{
    return getbgpattribs_r(&curmsg, pn);
}

void *getbgpattribs_r(bgp_msg_t *msg, size_t *pn)
{
    if (checkanytype(msg, BGP_UPDATE, F_RDWR))
        return NULL;

    size_t withdrawn_size;
    unsigned char *ptr = getwithdrawn_r(msg, &withdrawn_size);
    ptr += withdrawn_size;

    uint16_t len;
    if (likely(pn)) {
        memcpy(&len, ptr, sizeof(len));
        *pn = frombig16(len);
    }

    ptr += sizeof(len);
    return ptr;
}

int startbgpattribs(void)
{
    return startbgpattribs_r(&curmsg);
}

int startbgpattribs_r(bgp_msg_t *msg)
{
    if (checkanytype(msg, BGP_UPDATE, F_RDWR))
        return msg->err;

    endpending(msg);

    size_t n;
    unsigned char *ptr = getbgpattribs_r(msg, &n);
    if (msg->flags & F_WR) {

        if (bgppreserve(msg, ptr + n) != BGP_ENOERR)
            return msg->err;

        msg->pktlen -= n;
    }

    msg->uptr   = ptr;
    msg->ustart = ptr;
    msg->flags |= F_PATTR;
    return BGP_ENOERR;
}

int putbgpattrib(const bgpattr_t *attr)
{
    return putbgpattrib_r(&curmsg, attr);
}

int putbgpattrib_r(bgp_msg_t *msg, const bgpattr_t *attr)
{
    if (checktype(msg, BGP_UPDATE, F_WR | F_PATTR))
        return msg->err;

    size_t hdrsize = ATTR_HEADER_SIZE;
    size_t len = attr->len;
    if (attr->flags & ATTR_EXTENDED_LENGTH) {
        hdrsize = ATTR_EXTENDED_HEADER_SIZE;
        len <<= 8;
        len |= attr->exlen[1];  // len was exlen[0]
    }

    len += hdrsize;
    memcpy(msg->uptr, attr, len);
    msg->uptr += len;
    msg->pktlen += len;
    return BGP_ENOERR;
}

bgpattr_t *nextbgpattrib(void)
{
    return nextbgpattrib_r(&curmsg);
}

bgpattr_t *nextbgpattrib_r(bgp_msg_t *msg)
{
    if (checktype(msg, BGP_UPDATE, F_WR | F_PATTR))
        return NULL;

    uint16_t attrlen;
    memcpy(&attrlen, msg->ustart - sizeof(attrlen), sizeof(attrlen));
    attrlen = frombig16(attrlen);

    const unsigned char *end = msg->ustart + attrlen;
    if (msg->uptr == end)
        return NULL;
    
    if (unlikely(msg->uptr + ATTR_HEADER_SIZE > end)) {
        msg->err = BGP_EBADATTR;
        return NULL;
    }

    bgpattr_t *attr = (bgpattr_t *) msg->uptr;

    size_t hdrsize = ATTR_HEADER_SIZE;
    size_t len = attr->len;

    if (attr->flags & ATTR_EXTENDED_LENGTH) {
        if (unlikely(msg->uptr + ATTR_EXTENDED_HEADER_SIZE > end)) {
            msg->err = BGP_EBADATTR;
            return NULL;
        }
        hdrsize = ATTR_EXTENDED_HEADER_SIZE;
        len <<= 8;
        len |= attr->exlen[1];  // len was exlen[0]
    }

    msg->uptr += hdrsize;
    if (unlikely(msg->uptr + len > end)) {
        msg->err = BGP_EBADATTR;
        return NULL;
    }
    msg->uptr += len;
    return attr;
}

int endbgpattribs(void)
{
    return endbgpattribs_r(&curmsg);
}

int endbgpattribs_r(bgp_msg_t *msg)
{
    if (checktype(msg, BGP_UPDATE, F_PATTR))
        return msg->err;

    if (msg->flags & F_WR) { 
        bgprestore(msg);
        uint16_t len = tobig16(msg->uptr - msg->ustart);
        memcpy(msg->ustart - sizeof(len), &len, sizeof(len));
    }

    msg->flags &= ~F_PATTR;
    return BGP_ENOERR;
}

void *getnlri(size_t *pn)
{
    return getnlri_r(&curmsg, pn);
}

void *getnlri_r(bgp_msg_t *msg, size_t *pn)
{
    if (checkanytype(msg, BGP_UPDATE, F_RDWR))
        return NULL;

    size_t pattrs_length;
    unsigned char *ptr = getbgpattribs_r(msg, &pattrs_length);
    ptr += pattrs_length;

    if (likely(pn))
        *pn = msg->buf + msg->pktlen - ptr;

    return ptr;
}

int setnlri_r(bgp_msg_t *msg, const void *data, size_t n)
{
    if (checktype(msg, BGP_UPDATE, F_WR))
        return msg->err;

    size_t old_size;
    unsigned char *ptr = getnlri_r(msg, &old_size);
    if (n > old_size && !bgpensure(msg, n - old_size))
        return msg->err;

    uint16_t len = tobig16(n);
    ptr -= sizeof(len);
    memcpy(ptr, &len, sizeof(len));

    ptr += sizeof(len);
    memcpy(ptr, data, n);

    msg->pktlen -= old_size;
    msg->pktlen += n;
    return BGP_ENOERR;
}

int startnlri(void)
{
    return startnlri_r(&curmsg);
}

int startnlri_r(bgp_msg_t *msg)
{
    if (checkanytype(msg, BGP_UPDATE, F_RDWR))
        return msg->err;

    endpending(msg);

    size_t n;
    unsigned char *ptr = getnlri_r(msg, &n);
    if (msg->flags & F_WR)
        msg->pktlen -= n;

    msg->uptr   = ptr;
    //msg->ustart = ptr; this is not necessary because nlri field does not have a summary length field
    msg->flags |= F_NLRI;
    return BGP_ENOERR;
}

netaddr_t *nextnlri(void)
{
    return nextnlri_r(&curmsg);
}

netaddr_t *nextnlri_r(bgp_msg_t *msg)
{
    if (checktype(msg, BGP_UPDATE, F_RD | F_NLRI))
        return NULL;

    if (msg->uptr == msg->buf + msg->pktlen)
        return NULL;

    memset(&msg->pfxbuf, 0, sizeof(msg->pfxbuf));

    size_t bitlen = *msg->uptr++;
    size_t n = naddrsize(bitlen);

    if (unlikely(msg->uptr + n > msg->buf + msg->pktlen)) {
        msg->err = BGP_EBADNLRI;
        return NULL;
    }

    makenaddr(&msg->pfxbuf, msg->uptr, bitlen);

    msg->uptr += n;
    return &msg->pfxbuf;
}

int putnlri(const netaddr_t *p)
{
    return putnlri_r(&curmsg, p);
}

int putnlri_r(bgp_msg_t *msg, const netaddr_t *p)
{
    if (checktype(msg, BGP_UPDATE, F_WR | F_NLRI))
        return msg->err;

    size_t len = naddrsize(p->bitlen);
    if (!bgpensure(msg, len + 1))
        return msg->err;

    *msg->uptr++ = p->bitlen;
    memcpy(msg->uptr, p->bytes, len);
    msg->uptr   += len;
    msg->pktlen += len + 1;
    return BGP_ENOERR;
}

int endnlri(void)
{
    return endnlri_r(&curmsg);
}

int endnlri_r(bgp_msg_t *msg)
{
    if (checktype(msg, BGP_UPDATE, F_NLRI))
        return msg->err;

    msg->flags &= ~F_NLRI;
    return msg->err;
}

// TODO Route refresh message read/write functions =============================

// TODO Notification message read/write functions ==============================

