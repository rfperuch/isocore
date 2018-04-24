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
#include <isolario/branch.h>
#include <isolario/endian.h>
//#include <isolario/util.h>
//#include <limits.h>
//#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>


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

/** \defgroup offsets_functions 
 *  @{
 */

static int mrtheaderlen(int type)
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



/// @brief Packet reader/writer status flags
enum {
    F_DEFAULT = 0,
    F_RD = 1 << 0,         ///< Packet opened for read
    F_WR = 1 << 1,         ///< Packet opened for write
    F_RDWR = F_RD | F_WR,  ///< Shorthand for \a (F_RD | F_WR).
    BGP_WRAPPER = 1 << 2   ///< Commodity flag to not check types
};


/// @brief Packet reader/writer instance
static _Thread_local mrt_msg_t curmsg, curpimsg;

// extern version of inline function
extern const char *mrtstrerror(int err);

int mrterror(void)
{
    return mrterror_r(&curmsg);
}

int mrtpierror(void)
{
    return mrterror_r(&curpimsg);
}

int mrterror_r(mrt_msg_t *msg)
{
    return msg->err;
}

static int checkpi(int err)
{
    if(err != MRT_ENOERR)
        return err;
    
    int type = getmrtpitype();
    type = frombig16(type);
    if(type != MRT_TABLE_DUMPV2)
        return MRT_NOTPI;

    int subtype = getmrtpisubtype();
    subtype = frombig16(subtype);
    if(subtype != MRT_TABLE_DUMPV2_PEER_INDEX_TABLE)
        return MRT_NOTPI;

    return MRT_ENOERR;
}

//read section
int setmrtread(const void *data, size_t n)
{
    return setmrtread_r(&curmsg, data, n);
}

int setmrtpiread(const void *data, size_t n)
{
    return checkpi(setmrtread_r(&curpimsg, data, n));
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
    msg->pktlen = n;
    msg->bufsiz = n;

    memcpy(msg->buf, data, n);
    return MRT_ENOERR;
}

int setmrtreadfd(int fd)
{
    return setmrtreadfd_r(&curmsg, fd);
}

int setmrtpireadfd(int fd)
{
    return checkpi(setmrtreadfd_r(&curpimsg, fd));
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

int setmrtpireadfrom(io_rw_t *io)
{
    return checkpi(setmrtreadfrom_r(&curmsg, io));
}

int setmrtreadfrom_r(mrt_msg_t *msg, io_rw_t *io)
{
    if (msg->flags & F_RDWR)
        mrtclose_r(msg);

    unsigned char hdr[BASE_PACKET_LENGTH];
    if (io->read(io, hdr, sizeof(hdr)) != sizeof(hdr))
        return MRT_EIO;

    uint32_t len;
    memcpy(&len, &hdr[LENGTH_OFFSET], sizeof(len));
    len = frombig32(len);

    if (len < BASE_PACKET_LENGTH)
        return MRT_EBADHDR;

    msg->buf = msg->fastbuf;
    if (unlikely(len > sizeof(msg->fastbuf)))
        msg->buf = malloc(len);
    if (unlikely(!msg->buf))
        return MRT_ENOMEM;

    memcpy(msg->buf, hdr, sizeof(hdr));
    size_t n = len - sizeof(hdr);
    if (io->read(io, &msg->buf[sizeof(hdr)], n) != n)
        return MRT_EIO;

    msg->flags = F_RD;
    msg->err = MRT_ENOERR;
    msg->pktlen = len;
    msg->bufsiz = len;
    return MRT_ENOERR;
}

//header section 

size_t getmrtlen(void) {
    return getmrtlength_r(&curmsg);
}

size_t getmrtpilen(void) {
    return getmrtlength_r(&curmsg);
}

size_t getmrtlength_r(mrt_msg_t *msg)
{
    if (unlikely((msg->flags & F_RD) == 0))
        msg->err = MRT_EINVOP;
    if (unlikely(msg->err))
        return 0;

    uint32_t len;
    memcpy(&len, msg->buf, sizeof(len));
    return frombig32(len);
}

int getmrttype(void)
{
    return getmrttype_r(&curmsg);
}

int getmrtpitype(void)
{
    return getmrttype_r(&curpimsg);
}

int getmrttype_r(mrt_msg_t *msg)
{
    if (unlikely((msg->flags & F_RDWR) == 0))
        return MRT_EBADTYPE;

    return msg->buf[TYPE_OFFSET];
}

int getmrtsubtype(void)
{
    return getmrtsubtype_r(&curmsg);
}

int getmrtpisubtype(void)
{
    return getmrtsubtype_r(&curpimsg);
}

int getmrtsubtype_r(mrt_msg_t *msg)
{
    if (unlikely((msg->flags & F_RDWR) == 0))
        return MRT_EBADTYPE;

    return msg->buf[SUBTYPE_OFFSET];
}

int mrtclose(void) {
    return mrtclose_r(&curmsg);
}

int mrtpiclose(void) {
    return mrtclose_r(&curpimsg);
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
