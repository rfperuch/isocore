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

#include <stdio.h>
#include <zlib.h>

typedef struct io_rw_s io_rw_t;

struct io_rw_s {
    union {
        struct {
            int fd;
            int err;
        } un;
        void *ptr;
        FILE *file;
    };

    size_t (*read)(io_rw_t *io, void *dst, size_t n);
    size_t (*write)(io_rw_t *io, const void *buf, size_t n);
    int    (*error)(io_rw_t *io);
    int    (*close)(io_rw_t *io);
};

size_t io_fread(io_rw_t *io, void *dst, size_t n);
size_t io_fwrite(io_rw_t *io, const void *src, size_t n);
int    io_ferror(io_rw_t *io);
int    io_fclose(io_rw_t *io);

size_t io_fdread(io_rw_t *io, void *dst, size_t n);
size_t io_fdwrite(io_rw_t *io, const void *src, size_t n);
int    io_fderror(io_rw_t *io);
int    io_fdclose(io_rw_t *io);

#define IO_FILE_INIT(file) { \
    .file  = file,           \
    .read  = io_fread,       \
    .write = io_fwrite,      \
    .error = io_ferror,      \
    .close = io_fclose       \
}

#define IO_FD_INIT(fd) {   \
    .un    = { .fd = fd }, \
    .read  = io_fdread,    \
    .write = io_fdwrite,   \
    .error = io_fderror,   \
    .close = io_fdclose    \
}

#endif

