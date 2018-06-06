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
    F_WITHDRN    = 1 << 3,    ///< Reading/writing Withdrawn field
    F_ALLWITHDRN = 1 << 4,
    F_PATTR      = 1 << 5,  ///< Reading/writing Path Attributes field
    F_NLRI       = 1 << 6,  ///< Reading/writing NLRI field
    F_ALLNLRI    = 1 << 7,
    F_ASPATH     = 1 << 8,
    F_REALASPATH = 1 << 9,
    F_NHOP       = 1 << 10
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
    ROUTE_REFRESH_LENGTH = BASE_PACKET_LENGTH + sizeof(afi_safi_t),

    // special offset value, indicating that value was not found
    // (used while reading special attributes in update message, to indicate
    // that the attribute was already searched, but was not found)
    OFFSET_NOT_FOUND = UINT16_MAX
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
    const int mask = F_PM | F_WITHDRN | F_PATTR | F_NLRI | F_ASPATH | F_NHOP;

    // small optimization for common case
    if (likely((msg->flags & mask) == 0))
        return msg->err;

    // only one flag can be set
    if (msg->flags & F_PM)
        return endbgpcaps_r(msg);
    if (msg->flags & F_WITHDRN)
        return endwithdrawn_r(msg);
    if (msg->flags & F_PATTR)
        return endbgpattribs_r(msg);
    if (msg->flags & F_NLRI)
        return endnlri_r(msg);
    if (msg->flags & F_ASPATH)
        return endaspath_r(msg);

    assert(msg->flags & F_NHOP);
    return endnhop_r(msg);
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
    memset(msg->offtab, 0, sizeof(msg->offtab));
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
    memset(msg->offtab, 0, sizeof(msg->offtab));
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

    memcpy(msg->buf, bgp_marker, sizeof(bgp_marker));
    memset(msg->buf + sizeof(bgp_marker), 0, min_len - sizeof(bgp_marker));
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

        // memset(msg, 0, sizeof(*msg) - BGPBUFSIZ); XXX: optimize
        msg->flags = 0;
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

#define SAVE_UPDATE_ITER(pkt)                  \
    int prev_flags_             = pkt->flags;  \
    unsigned char *prev_ustart_ = pkt->ustart; \
    unsigned char *prev_uptr_   = pkt->uptr;   \
    unsigned char *prev_uend_   = pkt->uend;

#define RESTORE_UPDATE_ITER(pkt) \
    pkt->flags  = prev_flags_;   \
    pkt->ustart = prev_ustart_;  \
    pkt->uptr   = prev_uptr_;    \
    pkt->uend   = prev_uend_;

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

int startallwithdrawn(void)
{
    return startallwithdrawn_r(&curmsg);
}

static int dostartwithdrawn(bgp_msg_t *msg, int flags)
{
    endpending(msg);

    size_t n;
    unsigned char *ptr = getwithdrawn_r(msg, &n);
    if (msg->flags & F_WR) {

        if (bgppreserve(msg, ptr + n) != BGP_ENOERR)
            return msg->err;

        msg->pktlen -= n;
    } else {
        msg->pfxbuf.family = AF_INET;
    }

    msg->uptr   = ptr;
    msg->ustart = ptr;
    msg->uend   = ptr + n;
    msg->flags |= flags;
    return BGP_ENOERR;
}

int startwithdrawn_r(bgp_msg_t *msg)
{
    if (checkanytype(msg, BGP_UPDATE, F_RDWR))
        return msg->err;

    return dostartwithdrawn(msg, F_WITHDRN);
}

int startallwithdrawn_r(bgp_msg_t *msg)
{
    if (checktype(msg, BGP_UPDATE, F_RD))
        return msg->err;

    return dostartwithdrawn(msg, F_WITHDRN | F_ALLWITHDRN);
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

    while (msg->uptr == msg->uend) {  // this loop handles empty MP_UNREACH

        if ((msg->flags & F_ALLWITHDRN) == 0)
            return NULL;  // we're done

        msg->flags &= ~F_ALLWITHDRN;

        bgpattr_t *attr = getbgpmpunreach_r(msg);
        if (!attr)
            return NULL;

        afi_t  afi  = getmpafi(attr);
        safi_t safi = getmpsafi(attr);
        if (unlikely(safi != SAFI_UNICAST && safi != SAFI_MULTICAST)) {
            msg->err = BGP_EBADWDRWN;  // FIXME
            return NULL;
        }
        switch (afi) {
        case AFI_IPV4:
            msg->pfxbuf.family = AF_INET;
            break;
        case AFI_IPV6:
            msg->pfxbuf.family = AF_INET6;
            break;
        default:
            msg->err = BGP_EBADWDRWN; // FIXME;
            return NULL;
        }

        size_t len;
        msg->ustart = getmpnlri(attr, &len);
        msg->uptr   = msg->ustart;
        msg->uend   = msg->ustart + len;
    }

    memset(&msg->pfxbuf.bytes, 0, sizeof(msg->pfxbuf.bytes));

    size_t bitlen = *msg->uptr++;
    size_t n = naddrsize(bitlen);
    if (msg->uptr + n > msg->uend) {
        msg->err = BGP_EBADWDRWN;
        return NULL;
    }
    memcpy(msg->pfxbuf.bytes, msg->uptr, n);

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

    msg->flags &= ~(F_WITHDRN | F_ALLWITHDRN);
    return BGP_ENOERR;
}

#define INDEX_BIAS 1
#define MAKE_CODE_INDEX(x) ((x) + INDEX_BIAS)
#define EXTRACT_CODE_INDEX(x) ((x) - INDEX_BIAS)

// NOTE: index count must be less than nelems(bgp_msg_t.offtab)!
static const int8_t attr_code_index[255] = {
    [AS_PATH_CODE]            = MAKE_CODE_INDEX(0),
    [AGGREGATOR_CODE]         = MAKE_CODE_INDEX(1),
    [NEXT_HOP_CODE]           = MAKE_CODE_INDEX(2),
    [COMMUNITY_CODE]          = MAKE_CODE_INDEX(3),
    [MP_REACH_NLRI_CODE]      = MAKE_CODE_INDEX(4),
    [MP_UNREACH_NLRI_CODE]    = MAKE_CODE_INDEX(5),
    [EXTENDED_COMMUNITY_CODE] = MAKE_CODE_INDEX(6),
    [AS4_PATH_CODE]           = MAKE_CODE_INDEX(7),
    [AS4_AGGREGATOR_CODE]     = MAKE_CODE_INDEX(8),
    [LARGE_COMMUNITY_CODE]    = MAKE_CODE_INDEX(9)
};

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
    msg->uend   = ptr + n;
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
    if (checktype(msg, BGP_UPDATE, F_RD | F_PATTR))
        return NULL;

    if (msg->uptr == msg->uend)
        return NULL;

    if (unlikely(msg->uptr + ATTR_HEADER_SIZE > msg->uend)) {
        msg->err = BGP_EBADATTR;
        return NULL;
    }

    bgpattr_t *attr = (bgpattr_t *) msg->uptr;

    size_t hdrsize = ATTR_HEADER_SIZE;
    size_t len = attr->len;

    if (attr->flags & ATTR_EXTENDED_LENGTH) {
        if (unlikely(msg->uptr + ATTR_EXTENDED_HEADER_SIZE > msg->uend)) {
            msg->err = BGP_EBADATTR;
            return NULL;
        }
        hdrsize = ATTR_EXTENDED_HEADER_SIZE;
        len <<= 8;
        len |= attr->exlen[1];  // len was exlen[0]
    }

    msg->uptr += hdrsize;
    if (unlikely(msg->uptr + len > msg->uend)) {
        msg->err = BGP_EBADATTR;
        return NULL;
    }
    msg->uptr += len;

    // if this is a notable attribute, then insert its offset into offtab
    int idx = EXTRACT_CODE_INDEX(attr_code_index[attr->code]);
    if (idx >= 0)
        msg->offtab[idx] = ((unsigned char *) attr - msg->buf);

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

int startallnlri(void)
{
    return startallnlri_r(&curmsg);
}

static int dostartnlri(bgp_msg_t *msg, int flags)
{
    endpending(msg);

    size_t n;
    unsigned char *ptr = getnlri_r(msg, &n);
    if (msg->flags & F_WR)
        msg->pktlen -= n;  // forget any previous NLRI field
    else
        msg->pfxbuf.family = AF_INET; // NLRI field definitely has AF_INET prefixes

    msg->uptr = ptr;
    msg->ustart = ptr; // this is not strictly necessary because nlri does not have a summary length
    msg->uend = ptr + n;
    msg->flags |= flags;
    return msg->err;
}

int startnlri_r(bgp_msg_t *msg)
{
    if (checkanytype(msg, BGP_UPDATE, F_RDWR))
        return msg->err;

    return dostartnlri(msg, F_NLRI);
}

int startallnlri_r(bgp_msg_t *msg)
{
    if (checktype(msg, BGP_UPDATE, F_RD))
        return msg->err;

    return dostartnlri(msg, F_NLRI | F_ALLNLRI);
}

netaddr_t *nextnlri(void)
{
    return nextnlri_r(&curmsg);
}

netaddr_t *nextnlri_r(bgp_msg_t *msg)
{
    if (checktype(msg, BGP_UPDATE, F_RD | F_NLRI))
        return NULL;

    while (msg->uptr == msg->uend) {  // this handles empty MP_REACH attributes
        if ((msg->flags & F_ALLNLRI) == 0)
            return NULL;

        msg->flags &= ~F_ALLNLRI;

        bgpattr_t *attr = getbgpmpreach_r(msg);
        if (!attr)
            return NULL;

        afi_t afi   = getmpafi(attr);
        safi_t safi = getmpsafi(attr);
        if (unlikely(safi != SAFI_UNICAST && safi != SAFI_MULTICAST)) {
            msg->err = BGP_EBADNLRI;  // FIXME
            return NULL;
        }
        switch (afi) {
        case AFI_IPV4:
            msg->pfxbuf.family = AF_INET;
            break;
        case AFI_IPV6:
            msg->pfxbuf.family = AF_INET6;
            break;
        default:
            msg->err = BGP_EBADNLRI; // FIXME;
            return NULL;
        }

        size_t len;
        msg->ustart = getmpnlri(attr, &len);
        msg->uptr   = msg->ustart;
        msg->uend   = msg->uptr + len;
    }

    memset(msg->pfxbuf.bytes, 0, sizeof(msg->pfxbuf.bytes));

    msg->pfxbuf.bitlen = *msg->uptr++;
    size_t n = naddrsize(msg->pfxbuf.bitlen);
    if (unlikely(msg->uptr + n > msg->uend)) {
        msg->err = BGP_EBADNLRI;
        return NULL;
    }

    memcpy(msg->pfxbuf.bytes, msg->uptr, n);
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

    msg->flags &= ~(F_NLRI | F_ALLNLRI);
    return msg->err;
}

static int dostartaspath(bgp_msg_t *msg, bgpattr_t *attr, size_t as_size)
{
    endpending(msg);

    msg->seglen      = 0;
    msg->asp.as_size = as_size;
    // don't do any AS count verification on regular AS_PATH iteration
    msg->ascount     = -1;
    msg->asp.segno   = -1;
    if (attr) {
        size_t len;

        msg->asptr = getaspath(attr, &len);
        msg->asend = msg->asptr + len;
    } else {
        // no such path, empty iterator
        msg->asptr = msg->asend = NULL;
    }

    msg->flags |= F_ASPATH;
    return BGP_ENOERR;
}

int startaspath(size_t as_size)
{
    return startaspath_r(&curmsg, as_size);
}

int startaspath_r(bgp_msg_t *msg, size_t as_size)
{
    if (checktype(msg, BGP_UPDATE, F_RD))
        return msg->err;
    if (unlikely(as_size != sizeof(uint16_t) && as_size != sizeof(uint32_t))) {
        msg->err = BGP_EINVOP;
        return BGP_EINVOP;
    }

    return dostartaspath(msg, getbgpaspath_r(msg), as_size);
}

int startas4path(void)
{
    return startas4path_r(&curmsg);
}

int startas4path_r(bgp_msg_t *msg)
{
    if (checktype(msg, BGP_UPDATE, F_RD))
        return msg->err;

    return dostartaspath(msg, getbgpas4path_r(msg), sizeof(uint32_t));
}

int startrealaspath(size_t as_size)
{
    return startrealaspath_r(&curmsg, as_size);
}

int startrealaspath_r(bgp_msg_t *msg, size_t as_size)
{
    if (checktype(msg, BGP_UPDATE, F_RD))
        return msg->err;
    if (unlikely(as_size != sizeof(uint16_t) && as_size != sizeof(uint32_t))) {
        msg->err = BGP_EINVOP;
        return BGP_EINVOP;
    }

    endpending(msg);

    msg->flags      |= F_ASPATH;
    msg->seglen      = 0;
    msg->ascount     = -1;
    msg->asp.as_size = as_size;
    msg->asp.segno   = -1;

    bgpattr_t *asp = getbgpaspath_r(msg);
    if (!asp) {
        msg->asptr = msg->asend = NULL;
        msg->as4ptr = msg->as4end = NULL;
        return BGP_ENOERR;
    }

    size_t len;
    msg->asptr = getaspath(asp, &len);
    msg->asend = msg->asptr + len;
    if (as_size != sizeof(uint32_t)) {
        bgpattr_t *aggr = getbgpaggregator_r(msg);
        if (!aggr || getaggregatoras(aggr) != AS_TRANS)
            return BGP_ENOERR;

        aggr = getbgpas4aggregator_r(msg);
        if (!aggr)
            return BGP_ENOERR;

        bgpattr_t *as4p = getbgpas4path_r(msg);
        if (!as4p)
            return BGP_ENOERR;

        msg->as4ptr = getaspath(as4p, &len);
        msg->as4end = msg->as4ptr + len;

        unsigned char *ptr = msg->asptr;
        unsigned char *end = msg->asend;

        int ascount = 0, as4count = 0;
        while (ptr < end) {
            int type  = *ptr++;
            int count = *ptr++;

            ptr += count * sizeof(uint16_t);
            if (type == AS_SEGMENT_SET)
                count = 1;

            ascount += count;
        }

        if (unlikely(ptr > end)) {
            msg->err = BGP_EBADATTR;
            return msg->err;
        }

        ptr = msg->as4ptr;
        end = msg->as4end;
        while (ptr < end) {
            int type  = *ptr++;
            int count = *ptr++;

            ptr += count * sizeof(uint32_t);
            if (type == AS_SEGMENT_SET)
                count = 1;

            as4count += count;
        }

        if (unlikely(ptr > end)) {
            msg->err = BGP_EBADATTR;
            return msg->err;
        }

        if (ascount < as4count)
            return BGP_ENOERR;   // must ignore AS4_PATH

        msg->as4ptr  = getaspath(as4p, &len);
        msg->as4end  = msg->as4ptr + len;
        msg->ascount = ascount;
        msg->flags  |= F_REALASPATH;
    }

    return BGP_ENOERR;
}

as_pathent_t *nextaspath(void)
{
    return nextaspath_r(&curmsg);
}

as_pathent_t *nextaspath_r(bgp_msg_t *msg)
{
    if (checktype(msg, BGP_UPDATE, F_ASPATH))
        return NULL;
    if (msg->ascount == 0)
        return NULL;  // end of real AS path iteration

    while (msg->seglen == 0) {
        if (msg->asptr == msg->asend) {
            // end of iteration
            if (unlikely(msg->ascount > 0))
                msg->err = BGP_EBADATTR;  // unexpected AS4_PATH end

            return NULL;
        }

        if (unlikely(msg->asptr + 2 > msg->asend)) {
            msg->err = BGP_EBADATTR; // FIXME
            return NULL;
        }

        msg->asp.type = *msg->asptr++;
        msg->seglen   = *msg->asptr++;
        msg->segi     = 0;
        msg->asp.segno++;
    }

    // AS_PATH or AS4_PATH
    if (msg->asp.as_size == sizeof(uint16_t)) {
        uint16_t as16;

        memcpy(&as16, msg->asptr, sizeof(as16));
        msg->asp.as = frombig16(as16);
    } else {
        assert(msg->asp.as_size == sizeof(uint32_t));

        memcpy(&msg->asp.as, msg->asptr, sizeof(msg->asp.as));
        msg->asp.as = frombig32(msg->asp.as);
    }

    msg->asptr += msg->asp.as_size;
    msg->seglen--;
    msg->segi++;
    if ((msg->flags & F_REALASPATH) == 0 || msg->asp.as != AS_TRANS || msg->asp.type == AS_SEGMENT_SET) {
        // only decrement AS count if element is first in a SET or if inside a SEQ
        msg->ascount -= (msg->asp.type != AS_SEGMENT_SET || msg->segi == 1);
        return &msg->asp;
    }

    // we've hit an AS_TRANS, we must commute to AS4_PATH
    msg->asptr       = msg->as4ptr;
    msg->asend       = msg->as4end;
    msg->asp.as_size = sizeof(uint32_t);
    msg->seglen      = 0;
    msg->flags      &= ~F_REALASPATH;
    return nextaspath_r(msg);
}

int endaspath(void)
{
    return endaspath_r(&curmsg);
}

int endaspath_r(bgp_msg_t *msg)
{
    if (checktype(msg, BGP_UPDATE, F_ASPATH))
        return msg->err;

    msg->flags &= ~(F_ASPATH | F_REALASPATH);
    return BGP_ENOERR;
}

int startnhop(void)
{
    return startnhop_r(&curmsg);
}

int startnhop_r(bgp_msg_t *msg)
{
    if (checktype(msg, BGP_UPDATE, F_RD))
        return msg->err;

    endpending(msg);

    msg->nhptr = msg->nhend     = NULL;
    msg->mpnhptr = msg->mpnhend = NULL;

    bgpattr_t *attr = getbgpnexthop_r(msg);
    if (attr) {
        // setup iterator to return the IPv4 NEXT_HOP
        msg->nhbuf = getnexthop(attr);
        msg->nhptr = (unsigned char *) &msg->nhbuf;
        msg->nhend = msg->nhptr + sizeof(msg->nhbuf);

        msg->pfxbuf.family = AF_INET;
        msg->pfxbuf.bitlen = 32;
    }

    attr = getbgpmpreach_r(msg);
    if (attr) {
        // setup multiprotocol extension NEXT_HOP field
        size_t len;

        msg->mpnhptr = getmpnexthop(attr, &len);
        msg->mpnhend = msg->mpnhptr + len;

        afi_t afi   = getmpafi(attr);
        safi_t safi = getmpsafi(attr);
        if (unlikely(safi != SAFI_UNICAST && safi != SAFI_MULTICAST)) {
            msg->err = BGP_EBADATTR;
            return msg->err;
        }
        switch (afi) {
        case AFI_IPV4:
            msg->mpfamily = AF_INET;
            msg->mpbitlen = 32;
            break;
        case AFI_IPV6:
            msg->mpfamily = AF_INET6;
            msg->mpbitlen = 128;
            break;
        default:
            msg->err = BGP_EBADATTR;
            return msg->err;
        }
    }
    msg->flags |= F_NHOP;
    return BGP_ENOERR;
}

netaddr_t *nextnhop(void)
{
    return nextnhop_r(&curmsg);
}

netaddr_t *nextnhop_r(bgp_msg_t *msg)
{
    if (checktype(msg, BGP_UPDATE, F_NHOP))
        return NULL;

    if (msg->nhptr == msg->nhend) {
        if (!msg->mpnhptr)
            return NULL;  // done iterating

        // swap with multiprotocol
        msg->nhptr = msg->mpnhptr;
        msg->nhend = msg->mpnhend;

        msg->pfxbuf.family = msg->mpfamily;
        msg->pfxbuf.bitlen = msg->mpbitlen;

        msg->mpnhptr = msg->mpnhend = NULL;
    }

    size_t n = (msg->pfxbuf.bitlen >> 3);
    memcpy(msg->pfxbuf.bytes, msg->nhptr, n);
    msg->nhptr += n;
    return &msg->pfxbuf;
}

int endnhop(void)
{
    return endnhop_r(&curmsg);
}

int endnhop_r(bgp_msg_t *msg)
{
    if (checktype(msg, BGP_UPDATE, F_NHOP))
        return msg->err;

    msg->flags &= ~F_NHOP;
    return BGP_ENOERR;
}

static bgpattr_t *seekbgpattr(bgp_msg_t *msg, int code)
{
    if (checktype(msg, BGP_UPDATE, F_RD))
        return NULL;

    int idx = EXTRACT_CODE_INDEX(attr_code_index[code]);

    assert(idx >= 0 && idx < (int) nelems(msg->offtab));

    uint16_t off = msg->offtab[idx];
    if (unlikely(off == 0)) {
        bgpattr_t *attr;

        SAVE_UPDATE_ITER(msg);

        startbgpattribs_r(msg);
        while ((attr = nextbgpattrib_r(msg)) != NULL && attr->code != code);

        if (unlikely(endbgpattribs_r(msg) != BGP_ENOERR))
            return NULL;

        RESTORE_UPDATE_ITER(msg);
        if (msg->offtab[idx] == 0) {
            // we found no such attribute, but we know we iterated all attributes,
            // so remap any 0 to an OFFSET_NOT_FOUND
            for (int i = 0; i < (int) nelems(msg->offtab); i++) {
                if (msg->offtab[i] == 0)
                    msg->offtab[i] = OFFSET_NOT_FOUND;
            }
        }

        off = msg->offtab[idx];
    }
    if (off == OFFSET_NOT_FOUND)
        return NULL;

    return (bgpattr_t *) &msg->buf[off];
}

bgpattr_t *getbgpnexthop(void)
{
    return getbgpnexthop_r(&curmsg);
}

bgpattr_t *getbgpnexthop_r(bgp_msg_t *msg)
{
    return seekbgpattr(msg, NEXT_HOP_CODE);
}

bgpattr_t *getbgpaggregator(void)
{
    return getbgpaggregator_r(&curmsg);
}

bgpattr_t *getbgpaggregator_r(bgp_msg_t *msg)
{
    return seekbgpattr(msg, AGGREGATOR_CODE);
}

bgpattr_t *getbgpas4aggregator(void)
{
    return getbgpaggregator_r(&curmsg);
}

bgpattr_t *getbgpas4aggregator_r(bgp_msg_t *msg)
{
    return seekbgpattr(msg, AS4_AGGREGATOR_CODE);
}

bgpattr_t *getrealbgpaggregator(size_t as_size)
{
    return getrealbgpaggregator_r(&curmsg, as_size);
}

bgpattr_t *getrealbgpaggregator_r(bgp_msg_t *msg, size_t as_size)
{
    if (checktype(msg, BGP_UPDATE, F_RD))
        return NULL;

    bgpattr_t *aggr = getbgpaggregator_r(msg);
    if (unlikely(!aggr))
        return NULL;

    if (getaggregatoras(aggr) == AS_TRANS) {
        aggr = getbgpas4aggregator_r(msg);
        if (unlikely(!aggr))
            msg->err = BGP_EBADATTR;
    }
    return aggr;
}

bgpattr_t *getbgpaspath(void)
{
    return getbgpaspath_r(&curmsg);
}

bgpattr_t *getbgpaspath_r(bgp_msg_t *msg)
{
    return seekbgpattr(msg, AS_PATH_CODE);
}

bgpattr_t *getbgpas4path(void)
{
    return getbgpas4path_r(&curmsg);
}

bgpattr_t *getbgpas4path_r(bgp_msg_t *msg)
{
    return seekbgpattr(msg, AS4_PATH_CODE);
}

bgpattr_t *getbgpmpreach(void)
{
    return getbgpmpreach_r(&curmsg);
}

bgpattr_t *getbgpmpreach_r(bgp_msg_t *msg)
{
    return seekbgpattr(msg, MP_REACH_NLRI_CODE);
}

bgpattr_t *getbgpmpunreach(void)
{
    return getbgpmpunreach_r(&curmsg);
}

bgpattr_t *getbgpmpunreach_r(bgp_msg_t *msg)
{
    return seekbgpattr(msg, MP_UNREACH_NLRI_CODE);
}

bgpattr_t *getbgpcommunities(void)
{
    return getbgpcommunities_r(&curmsg);
}

bgpattr_t *getbgpcommunities_r(bgp_msg_t *msg)
{
    return seekbgpattr(msg, COMMUNITY_CODE);
}

bgpattr_t *getbgplargecommunities(void)
{
    return getbgplargecommunities_r(&curmsg);
}

bgpattr_t *getbgplargecommunities_r(bgp_msg_t *msg)
{
    return seekbgpattr(msg, LARGE_COMMUNITY_CODE);
}

bgpattr_t *getbgpexcommunities(void)
{
    return getbgpexcommunities_r(&curmsg);
}

bgpattr_t *getbgpexcommunities_r(bgp_msg_t *msg)
{
    return seekbgpattr(msg, EXTENDED_COMMUNITY_CODE);
}

// TODO Route refresh message read/write functions =============================

// TODO Notification message read/write functions ==============================

