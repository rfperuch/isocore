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
#include <bzlib.h>
#include <ctype.h>
#include <errno.h>
#include <isolario/branch.h>
#include <isolario/io.h>
#include <isolario/util.h>
#include <limits.h>
#include <lz4frame.h>
#include <lzma.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>

// stdio.h =====================================================================

size_t io_fread(io_rw_t *io, void *dst, size_t n)
{
#ifdef __GNUC__
    return fread_unlocked(dst, 1, n, io->file);
#else
    return fread(dst, 1, n, io->file);
#endif
}

size_t io_fwrite(io_rw_t *io, const void *src, size_t n)
{
#ifdef __GNUC__
    return fwrite_unlocked(src, 1, n, io->file);
#else
    return fwrite(src, 1, n, io->file);
#endif
}

int io_ferror(io_rw_t *io)
{
#ifdef __GNUC__
    return ferror_unlocked(io->file);
#else
    return ferror(io->file);
#endif
}

int io_fclose(io_rw_t *io)
{
    return fclose(io->file);
}

extern void io_file_init(io_rw_t *io, FILE *file);

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

extern void io_fd_init(io_rw_t *io, int fd);

// Dynamically allocated I/O types =============================================

static void *io_getstate(io_rw_t *io)
{
    return &io->padding;
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
    int bufsiz;
    z_stream stream;
    unsigned char buf[];  // bufsiz large
} io_zstate;

static size_t io_zread(io_rw_t *io, void *dst, size_t n)
{
    io_zstate *z = io_getstate(io);

    if (unlikely(z->err != Z_OK))
        return 0;

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
    io_zstate *z  = io_getstate(io);

    if (unlikely(z->err != Z_OK))
        return 0;

    z_stream *str = &z->stream;

    str->next_in  = (void *) src;
    str->avail_in = n;
    while (str->avail_in > 0) {
        if (str->avail_out == 0) {
            ssize_t nw = write(z->fd, z->buf, z->bufsiz);
            if (unlikely(nw < z->bufsiz)) {
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

    int err = z->err;  // function's result
    switch (z->mode) {
    case 'r':
        inflateEnd(str);
        break;
    case 'w':
        if (err == 0) {  // don't attempt to finalize write upon previous error
            int res = deflate(str, Z_FINISH);
            int n = z->bufsiz - str->avail_out;
            if (res == Z_STREAM_END && write(z->fd, z->buf, n) != n)
                err = Z_ERRNO;
        }

        deflateEnd(str);
        break;
    default:
        assert(false);
        break;
    }

    if (close(z->fd) != 0)
        err = Z_ERRNO;

    free(io);
    return err;
}

io_rw_t *io_zopen(int fd, size_t bufsiz, const char *mode, ...)
{
    if (bufsiz == 0)
        bufsiz = BUFSIZ;
    if (unlikely(bufsiz > INT_MAX))
        bufsiz = INT_MAX;  // ssize_t used for Unix I/O is signed, pick safe values

    io_zstate *z;
    io_rw_t *io = malloc(io_getsize(sizeof(*z) + bufsiz));
    if (unlikely(!io))
        return NULL;

    io->read  = io_zread;
    io->write = io_zwrite;
    io->error = io_zerror;
    io->close = io_zclose;

    va_list va;
    va_start(va, mode);

    z         = io_getstate(io);
    z->fd     = fd;
    z->err    = 0;
    z->mode   = *mode++;
    z->bufsiz = bufsiz;

    int compression = Z_DEFAULT_COMPRESSION;
    int wbits = 15;
    switch (z->mode) {
    case 'r':
        mode++;
        break;
    case 'w':
        if (*mode == '*') {
            compression = va_arg(va, int);
            mode++;
        } else if (isdigit((unsigned char) *mode)) {
            compression = atoi(mode);

            do mode++; while (isdigit((unsigned char) *mode));
        }

        break;
    default:
        va_end(va);
        errno = EINVAL;
        goto fail;
    }

    // normalize compression level
    compression = clamp(compression, 1, 9);

    // additional arguments
    if (*mode == 'b') {
        // manual specification of window bits
        mode++;
        if (*mode == '*') {
            wbits = va_arg(va, int);
            mode++;
        } else if (isdigit((unsigned char) *mode)) {
            wbits = atoi(mode);

            do mode++; while (isdigit((unsigned char) *mode));
        }
    }

    // normalize window bits
    wbits = clamp(wbits, 8, 15);
    switch (*mode) {
    case 'd':
        // use the original deflate format RFC 1951
        wbits = -wbits;
        break;
    case 'z':
        // use the zlib format RFC 1950
        break;
    case 'g':
    default:
        // support gzip compression (default) RFC 1952
        wbits += 16;
        break;
    }

    va_end(va);

    // open stream
    z_stream *str = &z->stream;
    memset(str, 0, sizeof(*str));

    int err;
    if (z->mode == 'r') {
        err = inflateInit2(str, wbits);
    } else {
        err = deflateInit2(str, compression, Z_DEFLATED, wbits, 8, Z_DEFAULT_STRATEGY);

        str->next_out  = z->buf;
        str->avail_out = z->bufsiz;
    }

    if (err != Z_OK)
        goto fail;

    return io;

fail:
    free(io);
    return NULL;
}

// BZip2 =======================================================================

typedef struct {
    int fd;
    int err;
    int mode;  // either 'r' or 'w'
    int32_t bufsiz;
    bz_stream stream;
    char buf[];  // bufsiz large
} io_bz2state;

static size_t io_bz2read(io_rw_t *io, void *dst, size_t n)
{
    io_bz2state *bz = io_getstate(io);
    if (unlikely(bz->err != BZ_OK))
        return 0;

    bz_stream *str  = &bz->stream;

    str->next_out  = dst;
    str->avail_out = n;
    while (str->avail_out > 0) {
        if (str->avail_in == 0) {
            ssize_t rd = read(bz->fd, bz->buf, bz->bufsiz);
            if (rd < 0) {
                bz->err = BZ_IO_ERROR;
                break;
            }
            if (rd == 0)
                break;  // EOF

            str->next_in  = bz->buf;
            str->avail_in = rd;
        }

        int err = BZ2_bzDecompress(str);
        if (err == BZ_STREAM_END)
            break;

        if (err != BZ_OK) {
            bz->err = err;
            break;
        }
    }

    return n - str->avail_out;
}

static size_t io_bz2write(io_rw_t *io, const void *src, size_t n)
{
    io_bz2state *bz = io_getstate(io);
    if (unlikely(bz->err != BZ_OK))
        return 0;

    bz_stream *str  = &bz->stream;

    str->next_in  = (char *) src;
    str->avail_in = n;
    while (str->avail_in) {
        if (str->avail_out == 0) {
            ssize_t wr = write(bz->fd, bz->buf, bz->bufsiz);
            if (unlikely(wr != bz->bufsiz)) {
                bz->err = BZ_IO_ERROR;
                break;
            }

            str->next_out  = bz->buf;
            str->avail_out = bz->bufsiz;
        }

        int err = BZ2_bzCompress(str, BZ_RUN);
        if (err != BZ_OK)  {
            bz->err = err;
            break;
        }
    }

    return n - str->avail_in;
}

static int io_bz2error(io_rw_t *io)
{
    io_bz2state *bz = io_getstate(io);
    return bz->err;
}

static void io_bz2finish(io_bz2state *bz)
{
    bz_stream *str = &bz->stream;

    if (unlikely(bz->err != BZ_OK))
        return;

    int err;
    do {
        if (str->avail_out == 0) {
            if (write(bz->fd, bz->buf, bz->bufsiz) != bz->bufsiz) {
                bz->err = BZ_IO_ERROR;
                return;
            }

            str->next_out  = bz->buf;
            str->avail_out = bz->bufsiz;
        }

        err = BZ2_bzCompress(str, BZ_FINISH);
        if (err != BZ_OK) {
            bz->err = err;
            break;
        }
    } while (err != BZ_STREAM_END);

    // flush
    ssize_t n = bz->bufsiz - str->avail_out;
    if (write(bz->fd, bz->buf, n) != n)
        bz->err = BZ_IO_ERROR;
}

static int io_bz2close(io_rw_t *io)
{
    io_bz2state *bz = io_getstate(io);
    if (bz->mode == 'r') {
        BZ2_bzDecompressEnd(&bz->stream);
    } else {
        io_bz2finish(bz);
        BZ2_bzCompressEnd(&bz->stream);
    }

    if (close(bz->fd) != 0)
        bz->err = BZ_IO_ERROR;

    int err = bz->err;
    free(io);
    return err;
}

io_rw_t *io_bz2open(int fd, size_t bufsiz, const char *mode, ...)
{
    if (bufsiz == 0)
        bufsiz = BUFSIZ;

    if (unlikely(bufsiz > INT_MAX))
        bufsiz = INT_MAX; // restrict to signed values for Unix ssize_t

    io_bz2state *bz;
    io_rw_t *io = malloc(io_getsize(sizeof(*bz) + bufsiz));
    if (unlikely(!io))
        return NULL;

    io->read  = io_bz2read;
    io->write = io_bz2write;
    io->error = io_bz2error;
    io->close = io_bz2close;

    va_list va;
    va_start(va, mode);

    bz         = io_getstate(io);
    bz->fd     = fd;
    bz->err    = 0;
    bz->mode   = *mode++;
    bz->bufsiz = bufsiz;

    bz_stream *str = &bz->stream;
    memset(str, 0, sizeof(*str));

    // defaults for BZ2
    int verbosity = 0;
    int small = false;
    int compression = 9;
    int factor = 0;

    switch (bz->mode) {
    case 'r':
        if (*mode == '-') {
            small = true;
            mode++;
        }

        break;
    case 'w':
        if (*mode == '*') {
            compression = va_arg(va, int);
            mode++;
        } else if (isdigit(*mode)) {
            compression = atoi(mode);
            do mode++; while (isdigit(*mode));
        }

        if (*mode == '+') {
            mode++;
            if (*mode == '*') {
                factor = va_arg(va, int);
                mode++;
            } else if (isdigit(*mode)) {
                factor = atoi(mode);
                do mode++; while (isdigit(*mode));
            }
        }

        break;
    default:
        // bad mode string
        va_end(va);
        errno = EINVAL;
        goto fail;
    }

    if (*mode == 'v') {
        mode++;

        if (*mode == '*')
            verbosity = va_arg(va, int);
        else
            verbosity = atoi(mode);  // on bad value verbosity = 0, which is good
    }
    va_end(va);

    // normalize arguments for BZ2 (silently ignore bad values)
    compression = clamp(compression, 1, 9);
    verbosity   = clamp(verbosity, 0, 4);
    factor      = clamp(factor, 0, 255);

    int err;
    if (bz->mode == 'r') {
        err = BZ2_bzDecompressInit(str, verbosity, small);
    } else {
        err = BZ2_bzCompressInit(str, compression, verbosity, factor);

        str->next_out  = bz->buf;
        str->avail_out = bufsiz;
    }
    if (unlikely(err != BZ_OK)) {
        errno = ENOMEM;
        goto fail;
    }

    return io;

fail:
    free(io);
    return NULL;
}

// xz (LZMA) ==================================================================

typedef struct {
    int fd;
    lzma_ret err;
    int mode;  // either 'r' or 'w'
    int bufsiz;
    lzma_stream stream;
    unsigned char buf[];  // bufsiz large
} io_xzstate;

static size_t io_xzread(io_rw_t *io, void *dst, size_t n)
{
    io_xzstate *xz = io_getstate(io);
    if (unlikely(xz->err != LZMA_OK))
        return 0;

    lzma_stream *str = &xz->stream;
    str->next_out = dst;
    str->avail_out = n;
    while (str->avail_out > 0) {
        if (str->avail_in == 0) {
            ssize_t nr = read(xz->fd, xz->buf, xz->bufsiz);
            if (nr < 0) {
                xz->err = errno;
                break;
            }
            if (nr == 0)
                break;  // EOF

            str->next_in = xz->buf;
            str->avail_in = nr;
        }

        lzma_ret err = lzma_code(str, LZMA_RUN);
        if (unlikely(err != LZMA_OK && err != LZMA_STREAM_END)) {
            xz->err = err;
            break;
        }
    }
    return n - str->avail_out;
}

static size_t io_xzwrite(io_rw_t *io, const void *src, size_t n)
{
    io_xzstate *xz = io_getstate(io);
    if (unlikely(xz->err != LZMA_OK))
        return 0;

    lzma_stream *str = &xz->stream;

    str->next_in  = (void *) src;
    str->avail_in = n;
    while (str->avail_in > 0) {
        if (str->avail_out == 0) {
            ssize_t nw = write(xz->fd, xz->buf, xz->bufsiz);
            if (unlikely(nw < xz->bufsiz)) {
                xz->err = errno;
                break;
            }

            str->next_out  = xz->buf;
            str->avail_out = xz->bufsiz;
        }

        int err = lzma_code(str, LZMA_RUN);
        if (unlikely(err != LZMA_OK)) {
            xz->err = err;
            break;
        }
    }
    return n - str->avail_in;
}

static int io_xzerror(io_rw_t *io)
{
    io_xzstate *lz = io_getstate(io);
    return lz->err;
}

static int io_xzclose(io_rw_t *io)
{
    io_xzstate *xz = io_getstate(io);

    int err = xz->err;
    lzma_stream *str = &xz->stream;

    switch (xz->mode) {
    case 'w':
        if (err == LZMA_OK) {
            // flush LZMA to disk
            err = lzma_code(str, LZMA_FINISH);
            if (err == LZMA_STREAM_END) {
                ssize_t n = xz->bufsiz - str->avail_out;
                if (write(xz->fd, xz->buf, n) != n)
                    err = errno;
            }
        }

        // fallthrough
    case 'r':
        lzma_end(str);
        break;
    default:
        assert(false);
        break;
    }

    if (close(xz->fd) != 0)
        err = errno;

    free(io);
    return err;
}

io_rw_t *io_xzopen(int fd, size_t bufsiz, const char *mode, ...)
{
    if (bufsiz == 0)
        bufsiz = BUFSIZ;
    if (unlikely(bufsiz > INT_MAX))
        bufsiz = INT_MAX;  // ssize_t used for Unix I/O is signed, pick safe values

    io_xzstate *xz;
    io_rw_t *io = malloc(io_getsize(sizeof(*xz) + bufsiz));
    if (unlikely(!io))
        return NULL;

    io->read  = io_xzread;
    io->write = io_xzwrite;
    io->error = io_xzerror;
    io->close = io_xzclose;

    xz         = io_getstate(io);
    xz->fd     = fd;
    xz->err    = LZMA_OK;
    xz->mode   = *mode++;
    xz->bufsiz = bufsiz;

    va_list va;

    va_start(va, mode);
    uint32_t compression = 6;
    switch (xz->mode) {
    case 'r':
        break;

    case 'w':
        if (isdigit((unsigned char) *mode)) {
            compression = atoi(mode);

            do mode++; while (isdigit((unsigned char) *mode));
        } else if (*mode == '*') {
            compression = va_arg(va, int);

            mode++;
        }
        break;

    default:
        va_end(va);
        goto fail;
    }

    // parse optional arguments
    char c;
    uint32_t presets  = 0;
    uint32_t flags    = 0;
    uint64_t memusage = UINT64_MAX;
    lzma_check check  = LZMA_CHECK_CRC64;
    while ((c = *mode++) != '\0') {
        switch (c) {
        case 'e':
            presets |= LZMA_PRESET_EXTREME;
            break;
        case '-':
            flags |= LZMA_IGNORE_CHECK;
            check = LZMA_CHECK_NONE;
            break;
        case 'c':
            check = LZMA_CHECK_CRC32;
            break;
        case 'C':
            check = LZMA_CHECK_CRC64;
            break;
        case 's':
            check = LZMA_CHECK_SHA256;
            break;
        case '+':
            // explicit default checksum settings
            flags &= ~LZMA_IGNORE_CHECK;
            check = LZMA_CHECK_CRC64;
            break;
        case 'm':
            if (*mode == '*') {
                memusage = va_arg(va, uint64_t);
            } else if (isdigit((unsigned char) *mode)) {
                memusage = atoll(mode);

                do mode++; while (isdigit((unsigned char) *mode));
            }

            break;
        default:
            break;
        }
    }

    va_end(va);

    // normalize compression value
    compression = clamp(compression, 0, 9);

    // setup LZMA stream
    lzma_stream *str = &xz->stream;
    memset(str, 0, sizeof(*str));

    int err;
    if (xz->mode == 'r') {
        err = lzma_auto_decoder(str, memusage, flags);
    } else {
        err = lzma_easy_encoder(str, compression | presets, check);

        str->next_out  = xz->buf;
        str->avail_out = xz->bufsiz;
    }

    if (err != LZMA_OK)
        goto fail;

    return io;

fail:
    free(io);
    return NULL;
}

// LZ4 =========================================================================

typedef struct {
    int fd;
    LZ4F_errorCode_t err;
    unsigned char *bufptr;
    unsigned char *cbufptr;
    int bufavail;
    int bufsiz;
    size_t cbufsiz;
    size_t cbufavail;
    LZ4F_compressionContext_t cctx;
    LZ4F_decompressionContext_t dctx;
    unsigned char buf[];
} io_lz4state;

#ifndef LZ4F_HEADER_SIZE_MAX
#define LZ4F_HEADER_SIZE_MAX 19
#endif

#define LZ4_END_FRAME_SIZE_MAX 8

static LZ4F_errorCode_t io_lz4begincompress(io_lz4state *lz, const LZ4F_preferences_t *prefs)
{
    lz->err = LZ4F_createCompressionContext(&lz->cctx, LZ4F_VERSION);
    if (LZ4F_isError(lz->err))
        return lz->err;

    size_t hdrsiz = LZ4F_compressBegin(lz->cctx, lz->cbufptr, lz->cbufsiz, prefs);
    if (LZ4F_isError(hdrsiz)) {
        LZ4F_freeCompressionContext(lz->cctx);
        return hdrsiz;
    }

    lz->cbufptr   += hdrsiz;
    lz->cbufavail -= hdrsiz;
    return lz->err;
}

static ssize_t io_lz4flush(io_lz4state *lz)
{
    ssize_t size = lz->cbufsiz - lz->cbufavail;

    ssize_t n = write(lz->fd, &lz->buf[lz->bufsiz], size);
    if (unlikely(n != size)) {
        lz->err = errno;
        return -1;
    }

    lz->cbufptr   = &lz->buf[lz->bufsiz];
    lz->cbufavail = lz->cbufsiz;
    return n;
}

static ssize_t io_lz4docompress(io_lz4state *lz)
{
    size_t cn = LZ4F_compressUpdate(lz->cctx, lz->cbufptr, lz->cbufsiz - lz->cbufavail, lz->buf, lz->bufsiz - lz->bufavail, NULL);
    if (unlikely(LZ4F_isError(cn))) {
        lz->err = 1;
        return -1;
    }

    lz->cbufavail -= cn;
    lz->cbufptr   += cn;

    ssize_t n = io_lz4flush(lz);
    if (likely(n >= 0)) {
        lz->bufptr   = lz->buf;
        lz->bufavail = lz->bufsiz;
    }
    return n;
}

static ssize_t io_lz4fillbuf(io_lz4state *lz)
{
    // LZ4 wants unconsumed data to be available to subsequent calls
    memmove(&lz->buf[lz->bufsiz], lz->cbufptr, lz->cbufavail);

    lz->cbufptr   = &lz->buf[lz->bufsiz] + lz->cbufavail;
    lz->cbufavail = lz->cbufsiz - lz->cbufavail;

    ssize_t n = read(lz->fd, lz->cbufptr, lz->cbufavail);
    if (unlikely(n < 0)) {
        lz->err = errno;
        return -1;
    }
    if (n == 0)
        return 0; // FIXME EOF

    lz->cbufptr   += n;
    lz->cbufavail -= n;

    size_t dstsize = lz->bufsiz;
    size_t srcsize = lz->cbufsiz - lz->cbufavail;
    size_t dn = LZ4F_decompress(lz->dctx, lz->buf, &dstsize, &lz->buf[lz->bufsiz], &srcsize, NULL);
    if (dn == 0) {
        // FIXME sucks
        LZ4F_frameInfo_t frame;

        const unsigned char *src = &lz->buf[lz->bufsiz];
        dn = LZ4F_getFrameInfo(lz->dctx, &frame, src, &srcsize);
        if (!LZ4F_isError(dn)) {
            src     += srcsize;
            srcsize  = (lz->cbufsiz - lz->cbufavail) - srcsize;
            dstsize  = lz->bufsiz;

            dn = LZ4F_decompress(lz->dctx, lz->buf, &dstsize, src, &srcsize, NULL);
        }
    }
    if (LZ4F_isError(dn)) {
        lz->err = dn;
        return -1;
    }

    lz->cbufptr   += srcsize;
    lz->cbufavail -= srcsize;

    lz->bufptr   = lz->buf;
    lz->bufavail = dstsize;
    return n;
}

static size_t io_lz4read(io_rw_t *io, void *dst, size_t n)
{
    io_lz4state *lz = io_getstate(io);

    if (unlikely(LZ4F_isError(lz->err)))
        return 0;

    unsigned char *ptr = dst;
    size_t avail       = n;
    while (avail > 0) {
        size_t size = min(avail, (size_t) lz->bufavail);
        memcpy(ptr, lz->bufptr, size);

        ptr   += size;
        avail -= size;

        lz->bufptr   += size;
        lz->bufavail -= size;

        if (lz->bufavail == 0 && io_lz4fillbuf(lz) <= 0)
            break;
    }

    return n - avail;
}

static size_t io_lz4write(io_rw_t *io, const void *src, size_t n)
{
    io_lz4state *lz = io_getstate(io);

    if (unlikely(LZ4F_isError(lz->err)))
        return 0;

    const unsigned char *ptr = src;
    size_t avail             = n;
    while (avail > 0) {
        size_t size = min(avail, (size_t) lz->bufavail);
        memcpy(lz->bufptr, ptr, size);

        lz->bufptr   += size;
        lz->bufavail -= size;
        if (io_lz4docompress(lz) < 0)
            break;

        ptr   += size;
        avail -= size;
    }
    return n - avail;
}

static void io_lz4finish(io_lz4state *lz)
{
    if (lz->cbufavail < LZ4_END_FRAME_SIZE_MAX)
        io_lz4flush(lz);  // TODO error check

    size_t n = LZ4F_compressEnd(lz->cctx, lz->cbufptr, lz->cbufavail, NULL);
    if (LZ4F_isError(n)) {
        lz->err = 1;
        return;
    }

    // lz->cbufptr   += n; useless
    lz->cbufavail -= n;
    io_lz4flush(lz);
}

static int io_lz4error(io_rw_t *io)
{
    io_lz4state *lz = io_getstate(io);
    return lz->err;
}

static int io_lz4close(io_rw_t *io)
{
    io_lz4state *lz = io_getstate(io);
    LZ4F_freeDecompressionContext(lz->dctx);
    if (lz->cctx) {
        io_lz4finish(lz);
        LZ4F_freeCompressionContext(lz->cctx);
    }
    if (close(lz->fd) != 0)
        lz->err = BZ_IO_ERROR;

    int err = lz->err;
    free(io);
    return err;
}

io_rw_t *io_lz4open(int fd, size_t bufsiz, const char *mode, ...)
{
    if (bufsiz == 0)
        bufsiz = BUFSIZ;
    if (unlikely(bufsiz > INT_MAX))
        bufsiz = INT_MAX;
    if (unlikely(bufsiz < LZ4F_HEADER_SIZE_MAX))
        bufsiz = LZ4F_HEADER_SIZE_MAX;  // at least enough room for LZ4 header

    LZ4F_preferences_t prefs = {
        .frameInfo = { .frameType = LZ4F_frame }
    };

    va_list va;
    va_start(va, mode);

    int rw = *mode++;
    switch (rw) {
    case 'r':
        break;
    case 'w':
        if (*mode == '*')
            prefs.compressionLevel = va_arg(va, int);
        else
            prefs.compressionLevel = atoi(mode);

        break;
    default:
        va_end(va);
        errno = EINVAL;
        return NULL;
    }
    va_end(va);

    size_t bound = LZ4F_compressBound(bufsiz, &prefs);

    io_lz4state *lz;
    io_rw_t *io = malloc(io_getsize(sizeof(*lz) + bufsiz + bound));
    if (unlikely(!io))
        return NULL;

    io->read  = io_lz4read;
    io->write = io_lz4write;
    io->error = io_lz4error;
    io->close = io_lz4close;

    lz            = io_getstate(io);
    lz->fd        = fd;
    lz->err       = 0;
    lz->bufptr    = lz->buf;
    lz->cbufptr   = &lz->buf[bufsiz];
    lz->bufavail  = bufsiz;
    lz->bufsiz    = bufsiz;
    lz->cbufsiz   = bound;
    lz->cbufavail = bound;
    lz->cctx      = NULL;
    lz->dctx      = NULL;
    if (rw == 'r') {
        lz->bufavail = 0; // FIXME
        lz->cbufavail = 0; // FIXME
        lz->err = LZ4F_createDecompressionContext(&lz->dctx, LZ4F_VERSION);
    } else {
        lz->err = io_lz4begincompress(lz, &prefs);
    }
    if (LZ4F_isError(lz->err)) {
        free(io);
        return NULL;
    }

    return io;
}

