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

#include <bzlib.h>
#include <zlib.h>
#include <lz4.h>
#include <isolario/branch.h>
#include <isolario/io.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// stdio.h =====================================================================

size_t io_fread(io_rw_t *io, void *dst, size_t n)
{
    return fread(dst, 1, n, io->file);
}

size_t io_fwrite(io_rw_t *io, const void *src, size_t n)
{
    return fwrite(src, 1, n, io->file);
}

int io_ferror(io_rw_t *io)
{
    return ferror(io->file);
}

int io_fclose(io_rw_t *io)
{
    return fclose(io->file);
}

// Unix fd =====================================================================

size_t io_fdread(io_rw_t *io, void *dst, size_t n)
{
    ssize_t nr = read(io->un.fd, dst, n);
    if (nr < 0) {
        io->un.err = 1;
        nr = 0;
    }
    return nr;
}

size_t io_fdwrite(io_rw_t *io, const void *src, size_t n)
{
    ssize_t nw = write(io->un.fd, src, n);
    if (nw < 0) {
        io->un.err = 1;
        nw = 0;
    }
    return nw;
}

int io_fderror(io_rw_t *io)
{
    return io->un.err;
}

int io_fdclose(io_rw_t *io)
{
    return close(io->un.fd);
}

// Dynamically allocated I/O types =============================================

static void *io_getstate(io_rw_t *io)
{
    return &io->padding[0];
}

static size_t io_getsize(size_t size)
{
    return offsetof(io_rw_t, padding) + size;
}

// Zlib ========================================================================

typedef struct {
    int fd;
    int err;
    int mode;  // either 'r' or 'w'
    z_stream stream;
    size_t bufsiz;
    unsigned char buf[];
} io_zstate;

static size_t io_zread(io_rw_t *io, void *dst, size_t n)
{
    io_zstate *z = io_getstate(io);

    z_stream *str = &z->stream;
    str->next_out = dst;
    str->avail_out = n;
    while (str->avail_out > 0) {
        if (str->avail_in == 0) {
            ssize_t nr = read(z->fd, z->buf, z->bufsiz);
            if (nr < 0) {
                z->err = Z_ERRNO;
                break;
            }
            if (nr == 0)
                break;  // EOF

            str->next_in = z->buf;
            str->avail_in = nr;
        }

        int err = inflate(str, Z_NO_FLUSH);
        if (unlikely(err == Z_NEED_DICT))
            err = Z_DATA_ERROR;
        if (unlikely(err != Z_OK && err != Z_STREAM_END)) {
            z->err = err;
            break;
        }
    }
    return n - str->avail_out;
}

static size_t io_zwrite(io_rw_t *io, const void *src, size_t n)
{
    io_zstate *z = io_getstate(io);

    z_stream *str = &z->stream;
    str->next_in  = (void *) src;
    str->avail_in = n;
    while (str->avail_in > 0) {
        if (str->avail_out == 0) {
            ssize_t nw = write(z->fd, z->buf, z->bufsiz);
            if (unlikely(nw < (ssize_t) z->bufsiz)) {
                z->err = Z_ERRNO;
                break;
            }

            str->next_out  = z->buf;
            str->avail_out = z->bufsiz;
        }

        int err = deflate(str, Z_NO_FLUSH);
        if (unlikely(err == Z_NEED_DICT))
            err = Z_DATA_ERROR;
        if (unlikely(err != Z_OK)) {
            z->err = err;
            break;
        }
    }
    return n - str->avail_in;
}

static int io_zerror(io_rw_t *io)
{
    io_zstate *z = io_getstate(io);
    return z->err;
}

static int io_zclose(io_rw_t *io)
{
    io_zstate *z  = io_getstate(io);
    z_stream *str = &z->stream;

    int err;
    switch (z->mode) {
    case 'r':
        inflateEnd(str);
        break;
    case 'w':
        err = deflate(str, Z_FINISH);
        if (err == Z_STREAM_END)
            write(z->fd, z->buf, z->bufsiz - str->avail_out);

        deflateEnd(str);
        break;
    default:
        break;
    }

    err = close(z->fd);
    free(io);
    return err;
}

io_rw_t *io_zopen(int fd, size_t bufsiz, const char *mode)
{
    if (bufsiz == 0)
        bufsiz = BUFSIZ;

    io_zstate *z;
    io_rw_t *io = malloc(io_getsize(sizeof(*z) + bufsiz));
    if (unlikely(!io))
        return NULL;

    io->read  = io_zread;
    io->write = io_zwrite;
    io->error = io_zerror;
    io->close = io_zclose;

    z         = io_getstate(io);
    z->fd     = fd;
    z->err    = 0;
    z->mode   = *mode;  // mode is not NULL, actual mode correctness checked later
    z->bufsiz = bufsiz;

    z_stream *str = &z->stream;
    memset(str, 0, sizeof(*str));
    int err = Z_ERRNO;
    if (strcmp(mode, "r") == 0) {
        err = inflateInit(str);
    } else if (strcmp(mode, "w") == 0) {
        err = deflateInit(str, Z_DEFAULT_COMPRESSION);

        str->next_out  = z->buf;
        str->avail_out = z->bufsiz;
    }

    if (err != Z_OK) {
        free(io);
        return NULL;
    }

    return io;
}

// BZip2 =======================================================================

io_rw_t *io_bz2open(int fd, size_t bufsiz, const char *mode)
{
    (void) fd; (void) bufsiz; (void) mode;
    // FIXME
    return NULL;
}

// LZ4 =========================================================================

io_rw_t *io_lz4open(int fd, size_t bufsiz, const char *mode)
{
    (void) fd; (void) bufsiz; (void) mode;
    // FIXME
    return NULL;
}

