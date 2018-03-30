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
#include <isolario/bgpattribs.h>
#include <isolario/bgpparams.h>
#include <isolario/branch.h>
#include <isolario/endian.h>
#include <isolario/util.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/// @brief General constants for the BGP packet reader/writer
enum {
    BGPBUFSIZ    = 4096,  ///< Working buffer for packet writing, should be large
    BGPGROWSTEP  = 256,
    BGPTMPBUFSIZ = 128    ///< Small additional buffer to use for out-of-order fields
};

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

/// @brief Packet reader/writer global status structure.
typedef struct {
    uint16_t flags;      ///< General status flags.
    uint16_t pktlen;     ///< Actual packet length.
    uint16_t bufsiz;     ///< Packet buffer capacity
    int16_t err;         ///< Last error code.
    unsigned char *buf;  ///< Packet buffer base.
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

    unsigned char fastbuf[BGPBUFSIZ];  ///< Fast buffer to avoid malloc()s.
} packet_state_t;

/// @brief Packet reader/writer instance
static _Thread_local packet_state_t curpkt;

// Minimum packet size table (see setbgpwrite()) ===============================

static const uint8_t bgp_minlengths[] = {
    [BGP_OPEN]          = MIN_OPEN_LENGTH,
    [BGP_UPDATE]        = BASE_PACKET_LENGTH,
    [BGP_NOTIFICATION]  = MIN_NOTIFICATION_LENGTH,
    [BGP_KEEPALIVE]     = BASE_PACKET_LENGTH,
    [BGP_ROUTE_REFRESH] = ROUTE_REFRESH_LENGTH,
    [BGP_CLOSE]         = BASE_PACKET_LENGTH
};

/// @brief Close any pending field from packet.
static int endpending(void)
{
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
}

// General functions ===========================================================

// extern version of inline function
extern const char *bgpstrerror(int err);

/// @brief Check for a \a flags operation on a BGP packet of type \a type.
static int checktype(int type, int flags)
{
    if (unlikely(getbgptype() != type))
        curpkt.err = BGP_EINVOP;
    if (unlikely((curpkt.flags & flags) != flags))
        curpkt.err = BGP_EINVOP;

    return curpkt.err;
}

/// @brief Check for any \a flags operation on a BGP packet of type \a type.
static int checkanytype(int type, int flags)
{
    if (unlikely(getbgptype() != type))
        curpkt.err = BGP_EINVOP;
    if (unlikely((curpkt.flags & flags) == 0))
        curpkt.err = BGP_EINVOP;

    return curpkt.err;
}

int setbgpread(const void *data, size_t n)
{
    assert(n <= UINT16_MAX);
    if (curpkt.flags & F_RDWR)
        bgpclose();

    curpkt.buf = curpkt.fastbuf;
    if (unlikely(n > sizeof(curpkt.fastbuf)))
        curpkt.buf = malloc(n);

    if (unlikely(!curpkt.buf))
        return BGP_ENOMEM;

    curpkt.flags = F_RD;
    curpkt.err = BGP_ENOERR;
    curpkt.pktlen = n;
    curpkt.bufsiz = n;

    memcpy(curpkt.buf, data, n);
    return BGP_ENOERR;
}

int setbgpwrite(int type)
{
    if (curpkt.flags & F_RDWR)
        bgpclose();

    if (unlikely(type < 0 || (unsigned) type >= nelems(bgp_minlengths)))
        return BGP_EBADTYPE;

    size_t min_len = bgp_minlengths[type];
    if (unlikely(min_len == 0))
        return BGP_EBADTYPE;

    curpkt.flags = F_WR;
    curpkt.pktlen = min_len;
    curpkt.bufsiz = sizeof(curpkt.fastbuf);
    curpkt.err = BGP_ENOERR;
    curpkt.buf = curpkt.fastbuf;

    memset(curpkt.buf, 0, min_len);
    memcpy(curpkt.buf, bgp_marker, sizeof(bgp_marker));
    curpkt.buf[TYPE_OFFSET] = type;
    return BGP_ENOERR;
}

int getbgptype(void)
{
    if (unlikely((curpkt.flags & F_RDWR) == 0))
        return BGP_BADTYPE;

    return curpkt.buf[TYPE_OFFSET];
}

int bgperror(void)
{
    return curpkt.err;
}

void *bgpfinish(size_t *pn)
{
    if ((curpkt.flags & F_WR) == 0)  // XXX: make it a function
        curpkt.err = BGP_EINVOP;
    if (unlikely(curpkt.err))
        return NULL;

    endpending();

    size_t n = curpkt.pktlen;

    uint16_t len = tobig16(n);
    memcpy(&curpkt.buf[LENGTH_OFFSET], &len, sizeof(len));
    if (likely(pn))
        *pn = n;

    return curpkt.buf;
}

int bgpclose(void)
{
    int err = BGP_ENOERR;
    if (curpkt.flags & F_RDWR) {
        err = bgperror();
        if (unlikely(curpkt.buf != curpkt.fastbuf))
            free(curpkt.buf);

        memset(&curpkt, 0, sizeof(curpkt));  // XXX: optimize
    }
    return err;
}

// Open message read/write functions ===========================================

bgp_open_t *getbgpopen(void)
{
    if (checktype(BGP_OPEN, F_RD))
        return NULL;

    bgp_open_t *op = &curpkt.opbuf;

    op->version = curpkt.buf[VERSION_OFFSET];
    memcpy(&op->hold_time, &curpkt.buf[HOLD_TIME_OFFSET], sizeof(op->hold_time));
    op->hold_time = frombig16(op->hold_time);
    memcpy(&op->my_as, &curpkt.buf[MY_AS_OFFSET], sizeof(op->my_as));
    op->my_as = frombig16(op->my_as);
    memcpy(&op->iden.s_addr, &curpkt.buf[IDEN_OFFSET], sizeof(op->iden.s_addr));

    return op;
}

int setbgpopen(const bgp_open_t *op)
{
    if (checktype(BGP_OPEN, F_WR))
        return curpkt.err;

    curpkt.buf[VERSION_OFFSET]   = op->version;
    
    uint16_t hold_time = tobig16(op->hold_time);
    memcpy(&curpkt.buf[HOLD_TIME_OFFSET], &hold_time, sizeof(hold_time));

    uint16_t my_as = tobig16(op->my_as);
    memcpy(&curpkt.buf[MY_AS_OFFSET], &my_as, sizeof(my_as));
    memcpy(&curpkt.buf[IDEN_OFFSET], &op->iden.s_addr, sizeof(op->iden.s_addr));
    return BGP_ENOERR;
}

void *getbgpparams(size_t *pn)
{
    if (checkanytype(BGP_OPEN, F_RDWR))
        return NULL;

    if (likely(pn))
        *pn = curpkt.buf[PARAMS_LENGTH_OFFSET];

    return &curpkt.buf[PARAMS_OFFSET];
}

int setbgpparams(const void *data, size_t n)
{
    if (checktype(BGP_OPEN, F_WR))
        return curpkt.err;

    // TODO no assert but actual check
    assert(n <= PARAMS_SIZE_MAX);

    curpkt.buf[PARAM_LENGTH_OFFSET] = n;
    memcpy(&curpkt.buf[PARAMS_OFFSET], data, n);

    curpkt.pktlen = PARAMS_OFFSET + n;
    return BGP_ENOERR;
}

int startbgpcaps(void)
{
    if (checkanytype(BGP_OPEN, F_RDWR))
        return curpkt.err;

    endpending();

    curpkt.flags |= F_PM;
    curpkt.params = getbgpparams(NULL);
    curpkt.pptr   = curpkt.params;
    return BGP_ENOERR;
}

void *nextbgpcap(size_t *pn)
{
    if (checktype(BGP_OPEN, F_RD | F_PM))
        return NULL;

    size_t n;
    unsigned char *base  = getbgpparams(&n);
    unsigned char *limit = base + n;
    unsigned char *end   = curpkt.params + PARAM_HEADER_SIZE + curpkt.params[1];
    unsigned char *ptr   = curpkt.pptr;
    if (ptr == end)
        curpkt.params = end;  // move to next parameter

    // skip uninteresting parameters
    while (ptr == curpkt.params) {
        if (ptr >= limit) {
            // end of parameters in this packet
            if (unlikely(ptr > limit))
                curpkt.err = BGP_EBADPARAMLEN;

            return NULL;
        }
        if (ptr[0] == CAPABILITY_CODE) {
            // found
            ptr += PARAM_HEADER_SIZE;
            break;
        }

        // next parameter
        ptr = end;
        curpkt.params = end;
        end = ptr + PARAM_HEADER_SIZE + ptr[1];
    }

    if (unlikely(ptr + ptr[1] + CAPABILITY_HEADER_SIZE > end)) {
        // bad packet
        curpkt.err = BGP_EBADPARAMLEN;
        return NULL;
    }

    if (pn)
        *pn = ptr[CAPABILITY_LENGTH_OFFSET] + CAPABILITY_HEADER_SIZE;

    // advance to next parameter
    curpkt.pptr = ptr + CAPABILITY_HEADER_SIZE + ptr[1];
    return ptr;
}

int putbgpcap(const void *data, size_t n)
{
    if (checktype(BGP_OPEN, F_WR | F_PM))
        return curpkt.err;

    unsigned char *ptr = curpkt.pptr;
    if (ptr == curpkt.params) {
        // first capability in parameter
        ptr[PARAM_CODE_OFFSET]   = CAPABILITY_CODE;
        ptr[PARAM_LENGTH_OFFSET] = 0; // update later
        ptr += PARAM_HEADER_SIZE;

        curpkt.pktlen += PARAM_HEADER_SIZE;
    }

    static_assert(BGPBUFSIZ >= MIN_OPEN_LENGTH + 0xff, "code assumes putbgpcap() can't overflow BGPBUFSIZ");

    // TODO actual check rather than assert()
    assert(n <= CAPABILITY_SIZE_MAX);
    memcpy(ptr, data, n);
    ptr += n;

    curpkt.pktlen += n;
    curpkt.pptr = ptr;
    return BGP_ENOERR;
}

int endbgpcaps(void)
{
    if (checktype(BGP_OPEN, F_PM))
        return curpkt.err;

    if (curpkt.flags & F_WR) {
        // write length for the last parameter
        unsigned char *ptr = curpkt.pptr;
        curpkt.params[1] = (ptr - curpkt.params) - PARAM_HEADER_SIZE;

        // write length for the entire parameter list
        const unsigned char *base = getbgpparams(NULL);
        size_t n = ptr - base;

        // TODO actual check rather than assert()
        assert(n <= PARAM_LENGTH_MAX);
        curpkt.buf[PARAMS_LENGTH_OFFSET] = n;
    }

    curpkt.flags &= ~F_PM;
    return BGP_ENOERR;
}

// Update message read/write functions =========================================

static int bgpensure(size_t len)
{
    len += curpkt.pktlen;
    if (unlikely(len > curpkt.bufsiz)) {
        // FIXME if len > 0xffff then oversized BGP packet (4K for regular BGP)!
        len += BGPGROWSTEP;
        if (unlikely(len > 0xffff))
            len = 0xffff;

        unsigned char *buf = curpkt.buf;
        if (buf == curpkt.fastbuf)
            buf = NULL;

        unsigned char *larger = realloc(buf, len);
        if (unlikely(!larger)) {
            curpkt.err = BGP_ENOMEM;
            return false;
        }

        if (!buf)
            memcpy(larger, curpkt.buf, curpkt.pktlen);

        curpkt.buf    = larger;
        curpkt.bufsiz = len;
    }

    return true;
}

static int bgppreserve(const unsigned char *from)
{
    unsigned char *end = &curpkt.buf[curpkt.pktlen];
    size_t size = end - from;

    curpkt.presbuf = curpkt.fastpresbuf;
    if (size > sizeof(curpkt.fastpresbuf)) {
        curpkt.presbuf = malloc(size);
        if (unlikely(!curpkt.presbuf)) {
            curpkt.err = BGP_ENOMEM;
            return curpkt.err;
        }
    }

    memcpy(curpkt.presbuf, from, size);
    return BGP_ENOERR;
}

static void bgprestore(void)
{
    unsigned char *end = &curpkt.buf[curpkt.pktlen];

    memcpy(curpkt.uptr, curpkt.presbuf, end - curpkt.uptr);
    if (curpkt.presbuf != curpkt.fastpresbuf)
        free(curpkt.presbuf);
}

int startwithdrawn(void)
{
    if (checkanytype(BGP_UPDATE, F_RDWR))
        return curpkt.err;

    endpending();

    size_t n;
    unsigned char *ptr = getwithdrawn(&n);
    if (curpkt.flags & F_WR) {

        if (!bgppreserve(ptr + n))
            return curpkt.err;

        curpkt.pktlen -= n;
    }

    curpkt.uptr   = ptr;
    curpkt.flags |= F_WITHDRN;
    return BGP_ENOERR;
}

int setwithdrawn(const void *data, size_t n)
{
    if (checktype(BGP_UPDATE, F_WR))
        return curpkt.err;

    uint16_t old_size;

    unsigned char *ptr = &curpkt.buf[BASE_PACKET_LENGTH];
    memcpy(&old_size, ptr, sizeof(old_size));
    old_size = frombig16(old_size);

    if (n > old_size && !bgpensure(n - old_size))
        return curpkt.err;

    unsigned char *start = ptr + sizeof(old_size) + old_size;
    unsigned char *end   = curpkt.buf + curpkt.pktlen;
    size_t bufsiz        = end - start;

    unsigned char buf[bufsiz];
    memcpy(buf, start, bufsiz);

    uint16_t new_size = tobig16(n);  // safe, since bgpensure() hasn't failed
    memcpy(ptr, &new_size, sizeof(new_size));

    ptr += sizeof(new_size);
    memcpy(ptr, data, n);

    ptr += n;
    memcpy(ptr, buf, bufsiz);

    curpkt.pktlen -= old_size;
    curpkt.pktlen += n;
    return BGP_ENOERR;
}

void *getwithdrawn(size_t *pn)
{
    if (checkanytype(BGP_UPDATE, F_RDWR))
        return NULL;

    uint16_t len;
    unsigned char *ptr = &curpkt.buf[BASE_PACKET_LENGTH];
    if (likely(pn)) {
        memcpy(&len, ptr, sizeof(len));
        *pn = frombig16(len);
    }

    ptr += sizeof(len);
    return ptr;
}

bgpprefix_t *nextwithdrawn(void)
{
    if (checktype(BGP_UPDATE, F_RD | F_WITHDRN))
        return NULL;

    // FIXME (also bound check the prefix)
    memset(&curpkt.pfxbuf, 0, sizeof(curpkt.pfxbuf));

    const bgpprefix_t *src = (const bgpprefix_t *) curpkt.uptr;
    size_t len = bgpprefixlen(src);
    memcpy(&curpkt.pfxbuf, src, len);
    // END

    curpkt.uptr += len;
    return &curpkt.pfxbuf;
}

int putwithdrawn(const bgpprefix_t *p)
{
    if (checktype(BGP_UPDATE, F_WR | F_WITHDRN))
        return curpkt.err;

    size_t len = bgpprefixlen(p);
    if (unlikely(!bgpensure(len)))
        return curpkt.err;

    memcpy(curpkt.uptr, p, len);
    curpkt.uptr   += len;
    curpkt.pktlen += len;
    return BGP_ENOERR;
}

int endwithdrawn(void)
{
    if (checktype(BGP_UPDATE, F_RDWR | F_WITHDRN))
        return curpkt.err;

    bgprestore();

    curpkt.flags &= ~F_WITHDRN;
    return BGP_ENOERR;
}

void *getbgpattribs(size_t *pn)
{
    if (checkanytype(BGP_UPDATE, F_RDWR))
        return NULL;

    size_t withdrawn_size;
    unsigned char *ptr = getwithdrawn(&withdrawn_size);
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
    if (checkanytype(BGP_UPDATE, F_RDWR))
        return curpkt.err;

    endpending();

    size_t n;
    unsigned char *ptr = getbgpattribs(&n);
    if (curpkt.flags & F_WR) {

        if (!bgppreserve(ptr + n))
            return curpkt.err;

        curpkt.pktlen -= n;
    }

    curpkt.uptr   = ptr;
    curpkt.flags |= F_PATTR;
    return BGP_ENOERR;
}

void *nextbgpattrib(size_t *pn)
{
    return NULL; // TODO
}

int endbgpattribs(void)
{
    if (checkanytype(BGP_UPDATE, F_RDWR | F_PATTR))
        return curpkt.err;

    if (curpkt.flags & F_WR)
        bgprestore();

    curpkt.flags &= ~F_PATTR;
    return BGP_ENOERR;
}

void *getnlri(size_t *pn)
{
    if (checkanytype(BGP_UPDATE, F_RDWR))
        return NULL;

    size_t pattrs_length;
    unsigned char *ptr = getbgpattribs(&pattrs_length);
    ptr += pattrs_length;

    uint16_t len;
    if (likely(pn)) {
        memcpy(&len, ptr, sizeof(len));
        *pn = frombig16(len);
    }

    ptr += sizeof(len);
    return ptr;
}

int setnlri(const void *data, size_t n)
{
    if (checktype(BGP_UPDATE, F_WR))
        return curpkt.err;

    size_t old_size;
    unsigned char *ptr = getnlri(&old_size);
    if (n > old_size && !bgpensure(n - old_size))
        return curpkt.err;

    uint16_t len = tobig16(n);
    ptr -= sizeof(len);
    memcpy(ptr, &len, sizeof(len));

    ptr += sizeof(len);
    memcpy(ptr, data, n);

    curpkt.pktlen -= old_size;
    curpkt.pktlen += n;
    return BGP_ENOERR;
}

int startnlri(void)
{
    if (checkanytype(BGP_UPDATE, F_RDWR))
        return curpkt.err;

    endpending();

    size_t n;
    unsigned char *ptr = getnlri(&n);
    if (curpkt.flags & F_WR)
        curpkt.pktlen -= n;

    curpkt.uptr   = ptr;
    curpkt.flags |= F_NLRI;
    return BGP_ENOERR;
}

bgpprefix_t *nextnlri(void)
{
    if (checktype(BGP_UPDATE, F_RD | F_NLRI))
        return NULL;

    // FIXME (also bound check the prefix)
    memset(&curpkt.pfxbuf, 0, sizeof(curpkt.pfxbuf));

    const bgpprefix_t *src = (const bgpprefix_t *) curpkt.uptr;
    size_t len = bgpprefixlen(src);
    memcpy(&curpkt.pfxbuf, src, len);
    // END

    curpkt.uptr += len;
    return &curpkt.pfxbuf;
}

int putnlri(const bgpprefix_t *p)
{
    if (checktype(BGP_UPDATE, F_WR | F_NLRI))
        return curpkt.err;

    size_t len = bgpprefixlen(p);
    if (!bgpensure(len))
        return curpkt.err;

    memcpy(curpkt.uptr, p, len);
    curpkt.uptr   += len;
    curpkt.pktlen += len;
    return BGP_ENOERR;
}

int endnlri(void)
{
    if (checkanytype(BGP_UPDATE, F_RDWR | F_NLRI))
        return curpkt.err;

    curpkt.flags &= ~F_NLRI;
    return curpkt.err;
}

// TODO Route refresh message read/write functions =============================

// TODO Notification message read/write functions ==============================

