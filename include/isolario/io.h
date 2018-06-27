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

#ifndef ISOLARIO_IO_H_
#define ISOLARIO_IO_H_

#include <isolario/funcattribs.h>
#include <stddef.h>
#include <stdio.h>

typedef struct io_rw_s io_rw_t;

struct io_rw_s {
    size_t (*read)(io_rw_t *io, void *dst, size_t n);
    size_t (*write)(io_rw_t *io, const void *buf, size_t n);
    int    (*error)(io_rw_t *io);
    int    (*close)(io_rw_t *io);

    union {
        // Unix file descriptor
        struct {
            int fd;
            int err;
        } un;

        // stdio.h standard FILE
        FILE *file;

        // User-defined data (generic)
        void *ptr;

        max_align_t padding; /// @private
    };
};

// stdio FILE abstaction

size_t io_fread(io_rw_t *io, void *dst, size_t n);
size_t io_fwrite(io_rw_t *io, const void *src, size_t n);
int    io_ferror(io_rw_t *io);
int    io_fclose(io_rw_t *io);

// convenience initialization for stdio FILE
inline nonnull(1, 2) void io_file_init(io_rw_t *io, FILE *file)
{
    io->file  = file;
    io->read  = io_fread;
    io->write = io_fwrite;
    io->error = io_ferror;
    io->close = io_fclose;
}

#define IO_FILE_INIT(file) { \
    .file  = file,           \
    .read  = io_fread,       \
    .write = io_fwrite,      \
    .error = io_ferror,      \
    .close = io_fclose       \
}

// POSIX fd abstraction

size_t io_fdread(io_rw_t *io, void *dst, size_t n);
size_t io_fdwrite(io_rw_t *io, const void *src, size_t n);
int    io_fderror(io_rw_t *io);
int    io_fdclose(io_rw_t *io);

// convenience initialization for POSIX fd
inline nonnull(1) void io_fd_init(io_rw_t *io, int fd)
{
    io->un.fd  = fd;
    io->un.err = 0;
    io->read   = io_fdread;
    io->write  = io_fdwrite;
    io->error  = io_fderror;
    io->close  = io_fdclose;
}

#define IO_FD_INIT(fd) {   \
    .un    = { .fd = fd }, \
    .read  = io_fdread,    \
    .write = io_fdwrite,   \
    .error = io_fderror,   \
    .close = io_fdclose    \
}

// compressed I/O (io_rw_t * structures are malloc()ed, but free()ed by their close() function)

malloclike wur nonnull(3) io_rw_t *io_zopen(int fd, size_t bufsiz, const char *mode, ...);
malloclike wur nonnull(3) io_rw_t *io_bz2open(int fd, size_t bufsiz, const char *mode, ...);
malloclike wur nonnull(3) io_rw_t *io_lz4open(int fd, size_t bufsiz, const char *mode, ...);
malloclike wur nonnull(3) io_rw_t *io_xzopen(int fd, size_t bufsiz, const char *mode, ...);

#endif
