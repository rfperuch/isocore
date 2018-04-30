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
#include <isolario/mrt.h>
#include <isolario/branch.h>
#include <isolario/endian.h>
//#include <isolario/util.h>
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
    BGP4MP_MESSAGE_LOCAL_AS_NUMBER = BGP4MP_MESSAGE_PEER_AS_NUMBER + sizeof(uint16_t),
    BGP4MP_MESSAGE_INTERFACE_INDEX = BGP4MP_MESSAGE_LOCAL_AS_NUMBER + sizeof(uint16_t),
    BGP4MP_MESSAGE_ADDRESS_FAMILY = BGP4MP_MESSAGE_INTERFACE_INDEX + sizeof(uint16_t),
    BGP4MP_MESSAGE_PEER_IP = BGP4MP_MESSAGE_ADDRESS_FAMILY + sizeof(uint16_t),
    // min size calculated with static member compreding old/new state but ip length 0 (no sense pkg)
    BGP4MP_MESSAGE_CHANGE_MIN_LENGTH = BGP4MP_STATE_CHANGE_PEER_IP + sizeof(uint8_t) * 2 + sizeof(uint16_t) * 2,

    // BGP4MP_MESSAGE_AS4
    // BGP4MP_STATE_CHANGE_AS4
    // BGP4MP_MESSAGE_LOCAL
    // BGP4MP_MESSAGE_AS4_LOCAL
};

enum {
    UNHANDLED = -1,
    NOTEXTENDED = 0,
    EXTENDED = 1,
};

static int mrtisext(int type)
{
    switch (type) {
    case MRT_BGP:          // Deprecated
    case MRT_BGP4PLUS:     // Deprecated
    case MRT_BGP4PLUS_01:  // Deprecated
    case MRT_TABLE_DUMP:
    case MRT_TABLE_DUMPV2:
    case MRT_BGP4MP:
        return NOTEXTENDED;
    case MRT_BGP4MP_ET:
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

static int mrtbgp(int type)
{
    switch (type) {
    case MRT_BGP:          // Deprecated
    case MRT_BGP4PLUS:     // Deprecated
    case MRT_BGP4PLUS_01:  // Deprecated
    case MRT_BGP4MP:
    case MRT_BGP4MP_ET:
        return 0;
    default:
        return 1;
    }
}

/// @brief Packet reader/writer status flags
enum {
    F_DEFAULT = 0,
    F_RD = 1 << 0,         ///< Packet opened for read
    F_WR = 1 << 1,         ///< Packet opened for write
    F_RDWR = F_RD | F_WR,  ///< Shorthand for \a (F_RD | F_WR).
    F_EXT = 1 << 2,        ///< Commodity flag to not check type 1 Extended 0 not Extended
    F_AST = 1 << 3,        ///< Commodity flag to not check subtype 1 AS32 0 AS16
    F_PE  = 1 << 4
};

static int mrtisas32(int type, int subtype)
{
    switch (type) {
    case MRT_BGP:          // Deprecated
    case MRT_BGP4PLUS:     // Deprecated
    case MRT_BGP4PLUS_01:  // Deprecated
    case MRT_TABLE_DUMP:
        return false;   // AS16
    case MRT_TABLE_DUMPV2:
    default:    //depreacted or unhandled
        return false;   // AS32 or AS16 but not in the header

    case MRT_BGP4MP:
    case MRT_BGP4MP_ET:
        return subtype == BGP4MP_MESSAGE_AS4         ||
               subtype == BGP4MP_STATE_CHANGE_AS4    ||
               subtype == BGP4MP_MESSAGE_AS4_LOCAL   ||
               subtype == BGP4MP_MESSAGE_AS4_ADDPATH ||
               subtype == BGP4MP_MESSAGE_AS4_LOCAL_ADDPATH;
    }
}

/// @brief Packet reader/writer instance
static _Thread_local mrt_msg_t curmsg, curpimsg;

// extern version of inline function
extern const char *mrtstrerror(int err);

static size_t mrtoffsetof(mrt_msg_t *msg, size_t base_offset)
{
    // add 4 bytes if extended
    return base_offset + ((!!(msg->flags & F_EXT)) << 2);
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

static int ismrtpi(const mrt_header_t *hdr)
{
    return hdr->type == MRT_TABLE_DUMPV2 && hdr->subtype == MRT_TABLE_DUMPV2_PEER_INDEX_TABLE;
}

// read section

static void readmrtheader(mrt_msg_t *msg, const unsigned char *hdr)
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
    memcpy(&len, hdr, sizeof(len));
    msg->hdr.len = frombig32(len);
    if (msg->flags & F_EXT) {
        memcpy(&time, &hdr[MICROSECOND_TIMESTAMP_OFFSET], sizeof(time));
        msg->hdr.stamp.tv_nsec = frombig32(time) * 1000ull;
    }
}

static int endpending(mrt_msg_t *msg)
{
    // small optimization for common case
    if (likely((msg->flags & F_PE) == 0))
        return msg->err;

    // only one flag can be set

    assert(msg->flags & F_PE);
    return endpeerents_r(msg);
}

int setmrtread(const void *data, size_t n)
{
    return setmrtread_r(&curmsg, data, n);
}

int setmrtread_r(mrt_msg_t *msg, const void *data, size_t n)
{
    assert(n <= UINT32_MAX);
    if (msg->flags & F_RDWR)
        mrtclose_r(msg);

    msg->buf = msg->fastbuf;
    if (unlikely(n > sizeof(msg->fastbuf)))
        msg->buf = malloc(n);

    if (unlikely(!msg->buf))
        return MRT_ENOMEM;

    msg->flags = F_RD;
    msg->err = MRT_ENOERR;
    msg->bufsiz = n;
    memcpy(msg->buf, data, n);

    readmrtheader(msg, msg->buf);

    /*
    int type = getmrttype_r(msg), subtype = getmrtsubtype_r(msg);
    msg->flags |= (mrtisext(type) == EXTENDED)?F_EXT:0;
    type = mrtisas32(type, subtype);
    if(unlikely(type == -1))
        return MRT_EBADTYPE;

    msg->flags |= type;
*/
    return MRT_ENOERR;
}

int setmrtreadfd(int fd)
{
    return setmrtreadfd_r(&curmsg, fd);
}

int setmrtreadfd_r(mrt_msg_t *msg, int fd)
{
    io_rw_t io = IO_FD_INIT(fd);
    return setmrtreadfrom_r(msg, &io);
}

int setmrtreadfrom(io_rw_t *io)
{
    return setmrtreadfrom_r(&curmsg, io);
}

int setmrtreadfrom_r(mrt_msg_t *msg, io_rw_t *io)
{
    if (msg->flags & F_RDWR)
        mrtclose_r(msg);

    unsigned char hdr[BASE_PACKET_LENGTH];
    if (io->read(io, hdr, sizeof(hdr)) != sizeof(hdr))
        return MRT_EIO;

    readmrtheader(msg, hdr);
    if (msg->hdr.len < BASE_PACKET_LENGTH)
        return MRT_EBADHDR;

    msg->buf = msg->fastbuf;
    if (unlikely(msg->hdr.len > sizeof(msg->fastbuf)))
        msg->buf = malloc(msg->hdr.len);
    if (unlikely(!msg->buf))
        return MRT_ENOMEM;

    memcpy(msg->buf, hdr, sizeof(hdr));
    size_t n = msg->hdr.len - sizeof(hdr);
    if (io->read(io, &msg->buf[sizeof(hdr)], n) != n)
        return MRT_EIO;

    msg->flags = F_RD;
/*    int type = getmrttype_r(msg), subtype = getmrtsubtype_r(msg);
    msg->flags |= (mrtisext(type) == EXTENDED)?F_EXT:0;
    type = mrtisas32(type, subtype);
    if(unlikely(type == -1))
        return MRT_EBADTYPE;

    msg->flags |= type;
*/
    msg->err = MRT_ENOERR;
    msg->bufsiz = msg->hdr.len;

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
    (void) msg, (void) hdr, (void) va;
    return MRT_ENOERR; // TODO

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

int mrtclose_r(mrt_msg_t *msg)
{
    int err = MRT_ENOERR;
    if (msg->flags & F_RDWR) {
        err = mrterror_r(msg);
        if (unlikely(msg->buf != msg->fastbuf))
            free(msg->buf);

        memset(msg, 0, sizeof(*msg));  // XXX: optimize
    }
    return err;
}

// Peer Index

int startpeerents(void)
{
    return startpeerents_r(&curmsg);
}

int startpeerents_r(mrt_msg_t *msg)
{
    if (unlikely(!ismrtpi(&msg->hdr)))
        msg->err = MRT_EINVOP;
    if (unlikely(msg->err))
        return msg->err;

    endpending(msg);
}

int endpeerents(void)
{
    return endpeerents_r(&curmsg);
}

int endpeerents_r(mrt_msg_t *msg)
{
    return msg->err; // TODO;
}

