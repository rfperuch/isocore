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

#include <CUnit/CUnit.h>
#include <isolario/io.h>
#include <isolario/util.h>
#include <string.h>
#include <test_util.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "test.h"

typedef io_rw_t *(*open_func_t)(int fd, size_t bufsiz, const char *mode, ...);

static void write_and_read(const char* where, open_func_t open_func, const char *what)
{
    int fd = open(where, O_CREAT | O_TRUNC | O_WRONLY, 0666);
    CU_ASSERT_TRUE_FATAL(fd >= 0);

    size_t len = strlen(what);

    io_rw_t *io = open_func(fd, 0, "w");
    CU_ASSERT_PTR_NOT_NULL_FATAL(io);

    size_t n = io->write(io, what, len);
    CU_ASSERT_TRUE_FATAL(n == len);
    CU_ASSERT_TRUE_FATAL(io->error(io) == 0);

    int err = io->close(io);
    CU_ASSERT_TRUE_FATAL(err == 0);

    fd = open(where, O_RDONLY);
    CU_ASSERT_TRUE_FATAL(fd >= 0);

    char buf[len + 1];
    io = open_func(fd, 0, "r");
    CU_ASSERT_PTR_NOT_NULL_FATAL(io);

    n = io->read(io, buf, len);
    CU_ASSERT_TRUE_FATAL(n == len);
    CU_ASSERT_TRUE_FATAL(io->error(io) == 0);

    buf[len] = '\0';
    CU_ASSERT_STRING_EQUAL(buf, what);

    err = io->close(io);
    CU_ASSERT_TRUE_FATAL(err == 0);

    unlink(where);
}

#define DEFAULT_STRING "the quick brown fox jumps over the lazy dog\n"

void testzio(void)
{
    write_and_read("miao.Z", io_zopen, DEFAULT_STRING);
}

void testbz2(void)
{
    write_and_read("miao.bz2", io_bz2open, DEFAULT_STRING);
}

void testxz(void)
{
    write_and_read("miao.xz", io_xzopen, DEFAULT_STRING);
}

void testlz4(void)
{
    write_and_read("miao.lz4", io_lz4open, DEFAULT_STRING);
}

void testlz4smallwrites(void)
{
    const char *filename    = "hello.txt";
    const char *test_string = "hello to everyone";

    int fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0666);
    CU_ASSERT_TRUE_FATAL(fd >= 0);

    io_rw_t *io = io_lz4open(fd, 0, "w*", 0);
    CU_ASSERT_PTR_NOT_NULL_FATAL(io);
    CU_ASSERT_TRUE_FATAL(io->error(io) == 0);
    char c = 'd';

    size_t n = io->write(io, &c, sizeof(c));
    CU_ASSERT_TRUE_FATAL(n == sizeof(c));
    CU_ASSERT_TRUE_FATAL(io->error(io) == 0);

    n = io->write(io, " ", 1);
    CU_ASSERT_TRUE_FATAL(n == 1);
    CU_ASSERT_TRUE_FATAL(io->error(io) == 0);

    n = io->write(io, test_string, strlen(test_string));
    CU_ASSERT_TRUE_FATAL(n == strlen(test_string));
    CU_ASSERT_TRUE_FATAL(io->error(io) == 0);

    int err = io->close(io);
    CU_ASSERT_TRUE_FATAL(err == 0);

    fd = open(filename, O_RDONLY);
    CU_ASSERT_TRUE_FATAL(fd >= 0);

    io = io_lz4open(fd, 0, "r");
    CU_ASSERT_PTR_NOT_NULL_FATAL(io);
    CU_ASSERT_TRUE_FATAL(io->error(io) == 0);

    char ci;
    char buf[strlen(test_string) + 1];

    n = io->read(io, &ci, sizeof(ci));
    CU_ASSERT_TRUE_FATAL(n == sizeof(ci));
    CU_ASSERT_TRUE_FATAL(io->error(io) == 0);
    CU_ASSERT_TRUE(ci == c);

    n = io->read(io, &ci, sizeof(ci));
    CU_ASSERT_TRUE_FATAL(n == sizeof(ci));
    CU_ASSERT_TRUE_FATAL(io->error(io) == 0);
    CU_ASSERT_TRUE(ci == ' ');

    n = io->read(io, buf, strlen(test_string));
    CU_ASSERT_TRUE_FATAL(n == strlen(test_string));
    CU_ASSERT_TRUE_FATAL(io->error(io) == 0);

    buf[strlen(test_string)] = '\0';
    CU_ASSERT_STRING_EQUAL(buf, test_string);

    err = io->close(io);
    CU_ASSERT_TRUE_FATAL(err == 0);

    unlink(filename);
}
