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
#include <isolario/branch.h>
#include <isolario/endian.h>
#include <isolario/mrt.h>
#include <isolario/util.h>
//#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>

enum {
    MRTGROWSTEP = 256
};

/// @brief Offsets for various MRT packet fields
enum {
    // common MRT packet offset
    TIMESTAMP_OFFSET = 0,
    TYPE_OFFSET = TIMESTAMP_OFFSET + sizeof(uint32_t),
    SUBTYPE_OFFSET = TYPE_OFFSET + sizeof(uint16_t),
    LENGTH_OFFSET = SUBTYPE_OFFSET + sizeof(uint16_t),
    MESSAGE_OFFSET = LENGTH_OFFSET + sizeof(uint32_t),
    MRT_HDRSIZ = MESSAGE_OFFSET,

    // extended version
    MICROSECOND_TIMESTAMP_OFFSET = MESSAGE_OFFSET,
    MESSAGE_EXTENDED_OFFSET = MICROSECOND_TIMESTAMP_OFFSET + sizeof(uint32_t),
    EXTENDED_MRT_HDRSIZ = MESSAGE_EXTENDED_OFFSET
};

/// @brief Packet reader/writer status flags
enum {
    MAX_MRT_SUBTYPE = MRT_TABLE_DUMPV2_RIB_GENERIC_ADDPATH,

    F_VALID     = 1 << 0,
    F_AS32      = 1 << 1,
    F_NEEDS_PI  = 1 << 2,
    F_IS_PI     = 1 << 3,
    F_IS_EXT    = 1 << 4,
    F_IS_BGP    = 1 << 5,
    F_HAS_STATE = 1 << 6,
    F_WRAPS_BGP = 1 << 7,

    F_RD   = 1 << 8,        ///< Packet opened for read
    F_WR   = 1 << (8 + 1),  ///< Packet opened for write
    F_RDWR = F_RD | F_WR,   ///< Shorthand for \a (F_RD | F_WR).

    F_PE = 1 << (8 + 2),
    F_RE = 1 << (8 + 3)
};

#define SHIFT(idx) ((idx) - MRT_TABLE_DUMP)

static const uint8_t masktab[][MAX_MRT_SUBTYPE + 1] = {
    [SHIFT(MRT_TABLE_DUMPV2)][MRT_TABLE_DUMPV2_PEER_INDEX_TABLE]   = F_VALID | F_IS_PI,
    [SHIFT(MRT_TABLE_DUMPV2)][MRT_TABLE_DUMPV2_RIB_GENERIC]        = F_VALID | F_NEEDS_PI,
    [SHIFT(MRT_TABLE_DUMPV2)][MRT_TABLE_DUMPV2_RIB_IPV4_UNICAST]   = F_VALID | F_NEEDS_PI,
    [SHIFT(MRT_TABLE_DUMPV2)][MRT_TABLE_DUMPV2_RIB_IPV4_MULTICAST] = F_VALID | F_NEEDS_PI,
    [SHIFT(MRT_TABLE_DUMPV2)][MRT_TABLE_DUMPV2_RIB_IPV6_UNICAST]   = F_VALID | F_NEEDS_PI,
    [SHIFT(MRT_TABLE_DUMPV2)][MRT_TABLE_DUMPV2_RIB_IPV6_MULTICAST] = F_VALID | F_NEEDS_PI,

    [SHIFT(MRT_BGP4MP)][BGP4MP_STATE_CHANGE]              = F_VALID | F_IS_BGP | F_HAS_STATE,
    [SHIFT(MRT_BGP4MP)][BGP4MP_MESSAGE]                   = F_VALID | F_IS_BGP | F_WRAPS_BGP,
    [SHIFT(MRT_BGP4MP)][BGP4MP_MESSAGE_AS4]               = F_VALID | F_AS32 | F_IS_BGP | F_WRAPS_BGP,
    [SHIFT(MRT_BGP4MP)][BGP4MP_STATE_CHANGE_AS4]          = F_VALID | F_AS32 | F_IS_BGP | F_HAS_STATE,
    [SHIFT(MRT_BGP4MP)][BGP4MP_MESSAGE_LOCAL]             = F_VALID | F_IS_BGP | F_WRAPS_BGP,
    [SHIFT(MRT_BGP4MP)][BGP4MP_MESSAGE_AS4_LOCAL]         = F_VALID | F_AS32 | F_IS_BGP | F_WRAPS_BGP,
    [SHIFT(MRT_BGP4MP)][BGP4MP_MESSAGE_AS4_ADDPATH]       = F_VALID | F_AS32 | F_IS_BGP | F_WRAPS_BGP,
    [SHIFT(MRT_BGP4MP)][BGP4MP_MESSAGE_LOCAL_ADDPATH]     = F_VALID | F_IS_BGP | F_WRAPS_BGP,
    [SHIFT(MRT_BGP4MP)][BGP4MP_MESSAGE_AS4_LOCAL_ADDPATH] = F_VALID | F_AS32 | F_IS_BGP | F_WRAPS_BGP,

    [SHIFT(MRT_BGP4MP_ET)][BGP4MP_STATE_CHANGE]              = F_VALID | F_IS_EXT | F_IS_BGP | F_HAS_STATE,
    [SHIFT(MRT_BGP4MP_ET)][BGP4MP_MESSAGE]                   = F_VALID | F_IS_EXT | F_IS_BGP | F_WRAPS_BGP,
    [SHIFT(MRT_BGP4MP_ET)][BGP4MP_MESSAGE_AS4]               = F_VALID | F_IS_EXT | F_AS32 | F_IS_BGP | F_WRAPS_BGP,
    [SHIFT(MRT_BGP4MP_ET)][BGP4MP_STATE_CHANGE_AS4]          = F_VALID | F_IS_EXT | F_AS32 | F_IS_BGP | F_HAS_STATE,
    [SHIFT(MRT_BGP4MP_ET)][BGP4MP_MESSAGE_LOCAL]             = F_VALID | F_IS_EXT | F_IS_BGP | F_WRAPS_BGP,
    [SHIFT(MRT_BGP4MP_ET)][BGP4MP_MESSAGE_AS4_LOCAL]         = F_VALID | F_IS_EXT | F_AS32 | F_IS_BGP | F_WRAPS_BGP,
    [SHIFT(MRT_BGP4MP_ET)][BGP4MP_MESSAGE_AS4_ADDPATH]       = F_VALID | F_IS_EXT | F_AS32 | F_IS_BGP | F_WRAPS_BGP,
    [SHIFT(MRT_BGP4MP_ET)][BGP4MP_MESSAGE_LOCAL_ADDPATH]     = F_VALID | F_IS_EXT | F_IS_BGP | F_WRAPS_BGP,
    [SHIFT(MRT_BGP4MP_ET)][BGP4MP_MESSAGE_AS4_LOCAL_ADDPATH] = F_VALID | F_IS_EXT | F_AS32 | F_IS_BGP | F_WRAPS_BGP
};

/// @brief Packet reader/writer instance
static _Thread_local mrt_msg_t curmsg, curpimsg;

// extern version of inline function
extern const char *mrtstrerror(int err);

static int mrtflags(mrt_header_t *hdr)
{
    return masktab[SHIFT(hdr->type)][hdr->subtype];
}

mrt_msg_t *getmrt(void)
{
    return &curmsg;
}

mrt_msg_t *getmrtpi(void)
{
    return &curpimsg;
}

int mrterror(void)
{
    return mrterror_r(&curmsg);
}

int mrterror_r(mrt_msg_t *msg)
{
    return msg->err;
}

// read section

static int readmrtheader(mrt_msg_t *msg, const unsigned char *hdr)
{
    uint32_t time;
    memcpy(&time, &hdr[TIMESTAMP_OFFSET], sizeof(time));
    msg->hdr.stamp.tv_sec = frombig32(time);
    msg->hdr.stamp.tv_nsec = 0;

    uint16_t type, subtype;
    memcpy(&type, &hdr[TYPE_OFFSET], sizeof(type));
    memcpy(&subtype, &hdr[SUBTYPE_OFFSET], sizeof(subtype));
    msg->hdr.type = frombig16(type);
    msg->hdr.subtype = frombig16(subtype);

    uint32_t len;
    memcpy(&len, &hdr[LENGTH_OFFSET], sizeof(len));
    msg->hdr.len = frombig32(len);
    int flags = mrtflags(&msg->hdr);
    if (unlikely((flags & F_VALID) == 0))
        return MRT_EBADHDR;

    if (flags & F_IS_EXT) {
        memcpy(&time, &hdr[MICROSECOND_TIMESTAMP_OFFSET], sizeof(time));
        msg->hdr.stamp.tv_nsec = frombig32(time) * 1000ull;
    }

    msg->flags = flags;
    return MRT_ENOERR;
}

static int endpending(mrt_msg_t *msg)
{
    // small optimization for common case
    if (likely((msg->flags & (F_PE | F_RE)) == 0))
        return msg->err;

    // only one flag can be set
    if (msg->flags & F_RE)
        return endribents_r(msg);

    assert(msg->flags & F_PE);
    return endpeerents_r(msg);
}

static int setuppitable(mrt_msg_t *msg, mrt_msg_t *pi)
{
    // FIXME: this invalidates pi's peer entry iterator, it would be wise not doing this...
    msg->peer_index = pi;
    if (likely(pi->pitab))
        return MRT_ENOERR;

    size_t n = startpeerents_r(pi, &n);

    pi->pitab = pi->fastpitab;
    if (unlikely(n > nelems(msg->fastpitab))) {
        pi->pitab = malloc(n * sizeof(*pi->pitab));
        if (unlikely(!pi->pitab))
            return MRT_ENOMEM;
    }
    for (size_t i = 0; i < n; i++) {
        msg->pitab[i] = (msg->peptr - msg->buf) - MESSAGE_OFFSET;
        nextpeerent_r(pi);
    }

    endpeerents_r(pi);
    return MRT_ENOERR;
}

int setmrtpi_r(mrt_msg_t *msg, mrt_msg_t *pi)
{
    if (likely((msg->flags & F_NEEDS_PI) && (pi->flags & F_IS_PI)))
        return setuppitable(msg, pi);

    return MRT_EINVOP;
}

int setmrtread(const void *data, size_t n)
{
    int res = setmrtread_r(&curmsg, data, n);
    if (likely(res == MRT_ENOERR && curpimsg.flags != 0))
        setuppitable(&curmsg, &curpimsg);

    return res;
}

int setmrtread_r(mrt_msg_t *msg, const void *data, size_t n)
{
    assert(n <= UINT32_MAX);
    if (msg->flags & F_RDWR)
        mrtclose_r(msg);

    int res = readmrtheader(msg, data);
    if (unlikely(res != MRT_ENOERR))
        return res;

    msg->buf = msg->fastbuf;
    if (unlikely(n > sizeof(msg->fastbuf)))
        msg->buf = malloc(n);
    if (unlikely(!msg->buf))
        return MRT_ENOMEM;

    msg->flags |= F_RD;
    msg->err = MRT_ENOERR;
    msg->bufsiz = n;
    msg->peer_index = NULL;
    memcpy(msg->buf, data, n);
    return MRT_ENOERR;
}

int setmrtreadfd(int fd)
{
    io_rw_t io = IO_FD_INIT(fd);
    return setmrtreadfrom(&io);
}

int setmrtreadfd_r(mrt_msg_t *msg, int fd)
{
    io_rw_t io = IO_FD_INIT(fd);
    return setmrtreadfrom_r(msg, &io);
}

int setmrtreadfrom(io_rw_t *io)
{
    int res = setmrtreadfrom_r(&curmsg, io);
    if (unlikely(res != MRT_ENOERR))
        return res;

    if ((curmsg.flags & F_NEEDS_PI) && (curpimsg.flags & F_RD))
        curmsg.peer_index = &curpimsg;

    return res;
}

int setmrtreadfrom_r(mrt_msg_t *msg, io_rw_t *io)
{
    if (msg->flags & F_RDWR)
        mrtclose_r(msg);

    unsigned char hdr[MRT_HDRSIZ];
    if (io->read(io, hdr, sizeof(hdr)) != sizeof(hdr))
        return MRT_EIO;

    readmrtheader(msg, hdr);
 
    msg->buf = msg->fastbuf;
    size_t n = msg->hdr.len + sizeof(hdr);
    if (unlikely(n > sizeof(msg->fastbuf)))
        msg->buf = malloc(n);
    if (unlikely(!msg->buf))
        return MRT_ENOMEM;

    memcpy(msg->buf, hdr, sizeof(hdr));
    if (io->read(io, &msg->buf[sizeof(hdr)], msg->hdr.len) != msg->hdr.len)
        return MRT_EIO;

    msg->flags |= F_RD;
    msg->err = MRT_ENOERR;
    msg->bufsiz = n;

    return MRT_ENOERR;
}

// header section

mrt_header_t *getmrtheader(void)
{
    return getmrtheader_r(&curmsg);
}

int setmrtheader(const mrt_header_t *hdr, ...)
{
    va_list va;

    va_start(va, hdr);
    int res = setmrtheaderv(hdr, va);
    va_end(va);
    return res;
}

int setmrtheaderv(const mrt_header_t *hdr, va_list va)
{
    return setmrtheaderv_r(&curmsg, hdr, va);
}

int setmrtheaderv_r(mrt_msg_t *msg, const mrt_header_t *hdr, va_list va)
{
    (void)msg, (void)hdr, (void)va;
    return MRT_ENOERR;  // TODO
}

int setmrtheader_r(mrt_msg_t *msg, const mrt_header_t *hdr, ...)
{
    va_list va;

    va_start(va, hdr);
    int res = setmrtheaderv_r(msg, hdr, va);
    va_end(va);
    return res;
}

mrt_header_t *getmrtheader_r(mrt_msg_t *msg)
{
    if (unlikely((msg->flags & F_RD) == 0))
        msg->err = MRT_EINVOP;
    if (unlikely(msg->err))
        return NULL;

    return &msg->hdr;
}

int mrtclose(void)
{
    return mrtclose_r(&curmsg);
}

int mrtclosepi(void)
{
    return mrtclose_r(&curpimsg);
}

int mrtclose_r(mrt_msg_t *msg)
{
    int err = MRT_ENOERR;
    if (msg->flags & F_RDWR) {
        err = mrterror_r(msg);
        if (unlikely(msg->buf != msg->fastbuf))
            free(msg->buf);

        // memset(msg, 0, sizeof(*msg));  // XXX: optimize
        msg->flags = 0;
    }
    return err;
}

// Peer Index

struct in_addr getpicollector(void)
{
    return getpicollector_r(&curmsg);
}

struct in_addr getpicollector_r(mrt_msg_t *msg)
{
    struct in_addr addr = {0};

    int flags = mrtflags(&msg->hdr);
    if (unlikely((flags & F_IS_PI) == 0))
        msg->err = MRT_EINVOP;
    if (unlikely(msg->err))
        return addr;

    memcpy(&addr, &msg->buf[MESSAGE_OFFSET], sizeof(addr));
    return addr;
}

size_t getpiviewname(char *buf, size_t n)
{
    return getpiviewname_r(&curmsg, buf, n);
}

size_t getpiviewname_r(mrt_msg_t *msg, char *buf, size_t n)
{
    int flags = mrtflags(&msg->hdr);
    if (unlikely((flags & F_IS_PI) == 0))
        msg->err = MRT_EINVOP;
    if (unlikely(msg->err))
        return 0;

    unsigned char *ptr = &msg->buf[MESSAGE_OFFSET];
    ptr += sizeof(uint32_t);

    uint16_t len;
    memcpy(&len, ptr, sizeof(len));
    len = frombig16(len);
    if (n > (size_t)len + 1)
        n = (size_t)len + 1;

    ptr += sizeof(len);
    if (n > 0) {
        memcpy(buf, ptr, n - 1);
        buf[n - 1] = '\0';
    }
    return len;
}

void *getpeerents(size_t *pcount, size_t *pn)
{
    return getpeerents_r(&curmsg, pcount, pn);
}

void *getpeerents_r(mrt_msg_t *msg, size_t *pcount, size_t *pn)
{
    int flags = mrtflags(&msg->hdr);
    if (unlikely((flags & F_IS_PI) == 0))
        msg->err = MRT_EINVOP;
    if (unlikely(msg->err))
        return NULL;

    unsigned char *ptr = &msg->buf[MESSAGE_OFFSET];
    ptr += sizeof(struct in_addr);  // collector id

    // view name
    uint16_t len;
    memcpy(&len, ptr, sizeof(len));
    ptr += sizeof(len);
    ptr += frombig16(len);

    // peer count
    if (pcount) {
        memcpy(&len, ptr, sizeof(len));
        *pcount = frombig16(len);
    }
    ptr += sizeof(len);

    if (pn)
        *pn = msg->hdr.len - (ptr - msg->buf);

    return ptr;
}

int startpeerents(size_t *pcount)
{
    return startpeerents_r(&curmsg, pcount);
}

int startpeerents_r(mrt_msg_t *msg, size_t *pcount)
{
    int flags = mrtflags(&msg->hdr);
    if (unlikely((flags & F_IS_PI) == 0))
        msg->err = MRT_EINVOP;
    if (unlikely(msg->err))
        return msg->err;

    endpending(msg);

    msg->peptr = getpeerents_r(msg, pcount, NULL);
    msg->flags |= F_PE;
    return msg->err;
}

peer_entry_t *nextpeerent(void)
{
    return nextpeerent_r(&curmsg);
}

enum {
    PT_IPV6 = 1 << 0,
    PT_AS32 = 1 << 1
};

peer_entry_t *nextpeerent_r(mrt_msg_t *msg)
{
    if (unlikely((msg->flags & F_PE) == 0))
        msg->err = MRT_EINVOP;
    if (unlikely(msg->err))
        return NULL;

    // NOTE: RFC 6396 "The Length field does not include the length of the MRT Common Header."
    unsigned char *end = msg->buf + msg->hdr.len + MESSAGE_OFFSET;
    if (msg->peptr == end)
        return NULL;

    uint8_t flags = *msg->peptr++;
    memcpy(&msg->pe.id, msg->peptr, sizeof(msg->pe.id));
    msg->peptr += sizeof(msg->pe.id);
    if (flags & PT_IPV6) {
        msg->pe.afi = AFI_IPV6;
        memcpy(&msg->pe.in6, msg->peptr, sizeof(msg->pe.in6));
        msg->peptr += sizeof(msg->pe.in6);
    } else {
        msg->pe.afi = AFI_IPV4;
        memcpy(&msg->pe.in, msg->peptr, sizeof(msg->pe.in));
        msg->peptr += sizeof(msg->pe.in);
    }
    if (flags & PT_AS32) {
        uint32_t as;

        msg->pe.as_size = sizeof(as);
        memcpy(&as, msg->peptr, sizeof(as));
        msg->pe.as = frombig32(as);
    } else {
        uint16_t as;

        msg->pe.as_size = sizeof(as);
        memcpy(&as, msg->peptr, sizeof(as));
        msg->pe.as = frombig16(as);
    }
    msg->peptr += msg->pe.as_size;
    return &msg->pe;
}

int endpeerents(void)
{
    return endpeerents_r(&curmsg);
}

int endpeerents_r(mrt_msg_t *msg)
{
    if (unlikely((msg->flags & F_PE) == 0))
        msg->err = MRT_EINVOP;
    if (unlikely(msg->err))
        return msg->err;

    msg->flags &= ~F_PE;
    return msg->err;  // TODO;
}

// RIB entries

int setribpi(void)
{
    if (unlikely((curmsg.flags & F_IS_PI) == 0))
        return MRT_NOTPEERIDX;

    memcpy(&curpimsg, &curmsg, sizeof(curmsg));
    curmsg.flags = 0;
    return MRT_ENOERR;
}

void *getribents(size_t *pcount, size_t *pn)
{
    return getribents_r(&curmsg, pcount, pn);
}

void *getribents_r(mrt_msg_t *msg, size_t *pcount, size_t *pn)
{
    if (unlikely(!msg->peer_index))
        msg->err = MRT_EINVOP;
    if (unlikely(msg->err))
        return NULL;

    unsigned char *ptr = &msg->buf[MESSAGE_OFFSET];

    uint32_t seqno;
    memcpy(&seqno, ptr, sizeof(seqno));
    seqno = frombig32(seqno);
    ptr += sizeof(seqno);

    uint16_t afi;
    uint8_t safi;
    switch (msg->hdr.subtype) {
    case MRT_TABLE_DUMPV2_RIB_GENERIC:
        memcpy(&afi, ptr, sizeof(afi));
        afi = frombig16(afi);
        ptr += sizeof(afi);
        safi = *ptr++;
        break;
    case MRT_TABLE_DUMPV2_RIB_IPV4_UNICAST:
        afi = AFI_IPV4;
        safi = SAFI_UNICAST;
        break;
    case MRT_TABLE_DUMPV2_RIB_IPV4_MULTICAST:
        afi = AFI_IPV4;
        safi = SAFI_MULTICAST;
        break;
    case MRT_TABLE_DUMPV2_RIB_IPV6_UNICAST:
        afi = AFI_IPV6;
        safi = SAFI_UNICAST;
        break;
    case MRT_TABLE_DUMPV2_RIB_IPV6_MULTICAST:
        afi = AFI_IPV6;
        safi = SAFI_MULTICAST;
        break;
    default:
        goto unsup;
    }
    if (unlikely(safi != SAFI_UNICAST && safi != SAFI_MULTICAST))
        goto unsup;

    sa_family_t fam;
    switch (afi) {
    case AFI_IPV4:
        fam = AF_INET;
        break;
    case AFI_IPV6:
        fam = AF_INET6;
        break;
    default:
        goto unsup;
    }

    size_t bitlen = *ptr++;
    msg->ribhdr.seqno = seqno;
    msg->ribhdr.afi = afi;
    msg->ribhdr.safi = safi;

    memset(&msg->ribhdr.nlri, 0, sizeof(msg->ribhdr.nlri));
    msg->ribhdr.nlri.family = fam;
    msg->ribhdr.nlri.bitlen = bitlen;

    size_t n = naddrsize(bitlen);
    memcpy(msg->ribhdr.nlri.bytes, ptr, n);
    ptr += n;

    uint16_t count;
    if (pcount) {
        memcpy(&count, ptr, sizeof(count));
        *pcount = frombig16(count);
    }
    ptr += sizeof(count);

    if (pn)
        *pn = msg->hdr.len + MESSAGE_OFFSET - (ptr - msg->buf);

    return ptr;

unsup:
    msg->err = MRT_ERIBNOTSUP;
    return NULL;
}

int setribents(const void *buf, size_t n)
{
    return setribents_r(&curmsg, buf, n);
}

int setribents_r(mrt_msg_t *msg, const void *buf, size_t n)
{
    if (unlikely(!msg->peer_index))
        msg->err = MRT_EINVOP;
    if (unlikely(msg->err))
        return msg->err;

    // TODO implement
    return MRT_ENOERR;
}

rib_header_t *startribents(size_t *pcount)
{
    return startribents_r(&curmsg, pcount);
}

rib_header_t *startribents_r(mrt_msg_t *msg, size_t *pcount)
{
    if (unlikely(!msg->peer_index))
        msg->err = MRT_EINVOP;
    if (unlikely(msg->err))
        return NULL;

    endpending(msg);

    msg->reptr = getribents_r(msg, pcount, NULL);
    msg->flags |= F_RE;
    return &msg->ribhdr;
}

rib_entry_t *nextribent(void)
{
    return nextribent_r(&curmsg);
}

rib_entry_t *nextribent_r(mrt_msg_t *msg)
{
    if (unlikely(!msg->peer_index))
        msg->err = MRT_EINVOP;
    if (unlikely(msg->err))
        return NULL;

    unsigned char *end = msg->buf + MESSAGE_OFFSET + msg->hdr.len;
    if (msg->reptr == end)
        return NULL;

    uint16_t idx;
    memcpy(&idx, msg->reptr, sizeof(idx));
    idx = frombig16(idx);
    msg->reptr += sizeof(idx);

    uint32_t originated;
    memcpy(&originated, msg->reptr, sizeof(originated));
    originated = frombig32(originated);
    msg->reptr += sizeof(originated);

    uint16_t attr_len;
    memcpy(&attr_len, msg->reptr, sizeof(attr_len));
    attr_len = frombig16(attr_len);
    msg->reptr += sizeof(attr_len);

    msg->ribent.peer_idx = idx;
    msg->ribent.originated = (time_t) originated;
    msg->ribent.attr_length = attr_len;
    msg->ribent.attrs = (bgpattr_t *) msg->reptr;

    msg->reptr += attr_len;
    return &msg->ribent;
}

int endribents(void)
{
    return endribents_r(&curmsg);
}

int endribents_r(mrt_msg_t *msg)
{
    if (unlikely((msg->flags & F_RE) == 0))
        msg->err = MRT_EINVOP;
    if (unlikely(msg->err))
        return msg->err;

    msg->flags |= F_RE;
    return MRT_ENOERR;
}

bgp4mp_header_t *getbgp4mpheader_r(mrt_msg_t *msg)
{
    if (unlikely(msg->flags & F_RD) == 0)
        msg->err = MRT_EINVOP;
    if (unlikely((msg->flags & F_IS_BGP) == 0))
        msg->err = MRT_EINVOP;
    if (unlikely(msg->err))
        return NULL;

    // FIXME range check

    unsigned char *ptr = &msg->buf[MESSAGE_OFFSET];
    if (msg->flags & F_IS_EXT)
        ptr = &msg->buf[MESSAGE_EXTENDED_OFFSET];

    bgp4mp_header_t *hdr = &msg->bgp4mphdr;
    memset(hdr, 0, sizeof(*hdr));
    if (msg->flags & F_AS32) {
        memcpy(&hdr->peer_as, ptr, sizeof(hdr->peer_as));
        hdr->peer_as = frombig32(hdr->peer_as);
        ptr += sizeof(hdr->peer_as);
        memcpy(&hdr->local_as, ptr, sizeof(hdr->local_as));
        hdr->local_as = frombig32(hdr->local_as);
        ptr += sizeof(hdr->local_as);
    } else {
        uint16_t as;

        memcpy(&as, ptr, sizeof(as));
        hdr->peer_as = frombig16(as);
        ptr += sizeof(as);
        memcpy(&as, ptr, sizeof(as));
        hdr->local_as = frombig16(as);
        ptr += sizeof(as);
    }

    memcpy(&hdr->iface, ptr, sizeof(hdr->iface));
    hdr->iface = frombig16(hdr->iface);
    ptr += sizeof(hdr->iface);

    uint16_t afi;
    memcpy(&afi, ptr, sizeof(afi));
    afi = frombig16(afi);
    ptr += sizeof(afi);
    switch (afi) {
    case AFI_IPV4:
        makenaddr(&hdr->peer_addr, ptr, 32);
        ptr += sizeof(struct in_addr);
        makenaddr(&hdr->local_addr, ptr, 32);
        ptr += sizeof(struct in_addr);
        break;
    case AFI_IPV6:
        makenaddr6(&hdr->peer_addr, ptr, 128);
        ptr += sizeof(struct in6_addr);
        makenaddr6(&hdr->local_addr, ptr, 128);
        ptr += sizeof(struct in6_addr);
        break;
    default:
        msg->err = MRT_EAFINOTSUP;
        return NULL;
    }

    if (msg->flags & F_HAS_STATE) {
        memcpy(&hdr->old_state, ptr, sizeof(hdr->old_state));
        hdr->old_state = frombig16(hdr->old_state);
        ptr += sizeof(hdr->old_state);
        memcpy(&hdr->new_state, ptr, sizeof(hdr->new_state));
        hdr->new_state = frombig16(hdr->new_state);
    } else {
        hdr->old_state = hdr->new_state = 0;
    }

    return hdr;
}

bgp4mp_header_t *getbgp4mpheader(void)
{
    return getbgp4mpheader_r(&curmsg);
}

void *unwrapbgp4mp(size_t *pn)
{
    return unwrapbgp4mp_r(&curmsg, pn);
}

void *unwrapbgp4mp_r(mrt_msg_t *msg, size_t *pn)
{
    if (unlikely(msg->flags & F_RD) == 0)
        msg->err = MRT_EINVOP;
    if (unlikely((msg->flags & F_IS_BGP) == 0))
        msg->err = MRT_EINVOP;
    if (unlikely(msg->err))
        return NULL;

    // FIXME bounds check

    unsigned char *ptr = &msg->buf[MESSAGE_OFFSET];
    if (msg->flags & F_IS_EXT)
        ptr = &msg->buf[MESSAGE_EXTENDED_OFFSET];

    unsigned char *end = ptr + msg->hdr.len;

    ptr += (msg->flags & F_AS32) ? 2 * sizeof(uint32_t) : 2 * sizeof(uint16_t);
    ptr += sizeof(uint16_t);

    uint16_t afi;
    memcpy(&afi, ptr, sizeof(afi));
    afi = frombig16(afi);
    ptr += sizeof(afi);
    switch (afi) {
    case AFI_IPV4:
        ptr += 2 * sizeof(struct in_addr);
        break;
    case AFI_IPV6:
        ptr += 2 * sizeof(struct in6_addr);
        break;
    default:
        msg->err = MRT_EAFINOTSUP;
        return NULL;
    }
    if (msg->flags & F_HAS_STATE)
        ptr += 2 * sizeof(uint16_t);

    if (likely(pn))
        *pn = end - ptr;

    return ptr;
}

