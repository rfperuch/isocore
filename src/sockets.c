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

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <isolario/sockets.h>
#include <netinet/in.h>
#include <stdatomic.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>

enum  {
    DEFAULT_BACKLOG = 32
};

static atomic_int backlog_length = DEFAULT_BACKLOG;

int setbacklog(int length)
{
    return atomic_exchange_explicit(&backlog_length, length, memory_order_relaxed);
}

int getbacklog(void)
{
    return atomic_load(&backlog_length);
}

int socketflags(int fd, int flags)
{
    int mask = fcntl(fd, F_GETFL, NULL);
    if (mask < 0)
        return -1;

    mask |= flags;
    if (fcntl(fd, F_SETFL, mask) < 0)
        return -1;

    return 0;
}

int listeningsocket(int family, const struct sockaddr *addr, socklen_t addrlen, int flags)
{
    int err;

    int fd = socket(family, SOCK_STREAM, 0);
    if (fd < 0)
        return fd;

    if (bind(fd, addr, addrlen) < 0)
        goto error;

    if (listen(fd, getbacklog()) < 0)
        goto error;

    if (socketflags(fd, flags) < 0)
        goto error;

    return fd;

error:
    err = errno;  // don't clobber errno with close()
    close(fd);
    errno = err;

    return -1;
}

int connectsocket(int family, const struct sockaddr *addr, socklen_t addrlen, int flags)
{
    int err;

    int fd = socket(family, SOCK_STREAM, 0);
    if (fd < 0)
        return fd;

    if (connect(fd, addr, addrlen) < 0)
        goto error;

    if (socketflags(fd, flags) < 0)
        goto error;

    return fd;

error:
    err = errno;  // don't clobber errno with close()
    close(fd);
    errno = err;

    return -1;
}

