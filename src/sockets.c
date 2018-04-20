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
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <isolario/sockets.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

enum  {
    DEFAULT_BACKLOG = 32
};

extern int hashv4(const struct in_addr *addr);

extern int hashv6(const struct in6_addr *addr6);

extern int sockaddrincmp(const struct sockaddr *a, const struct sockaddr *b);

extern int sockaddrin6cmp(const struct sockaddr *a, const struct sockaddr *b);

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

int setmd5key(int sd, char *md5key)
{
#ifdef __linux__

    struct sockaddr_storage addr;
    socklen_t len = sizeof(addr);
    if (getpeername(sd, (struct sockaddr*)&addr, &len) < 0)
        return -1;

    struct tcp_md5sig md5sig;
    memset(&md5sig, 0, sizeof(md5sig));
    md5sig.tcpm_addr = addr;
    strcpy(md5sig.tcpm_key, md5key);

    return setsockopt(sd, IPPROTO_TCP, TCP_MD5SIG, &md5sig, sizeof(md5sig));

#else
    #error Please provide an implementation for that OS!
#endif
}

int sopen(const struct sockaddr *addr, socklen_t addrlen, const char *options, ...)
{
    struct sockaddr *saddr = NULL;
    socklen_t saddrlen;
    bool should_listen = false;

    int backlog = 0;
    int family = AF_INET;
    int type = SOCK_STREAM;
    int reuse = 0;
    int flags = 0;
    int err;

    va_list va;
    va_start(va, options);

    while (*options != '\0') {
        switch (*options++) {
        case '4':
            family = AF_INET;
            break;

        case '6':
            family = AF_INET6;
            break;

        case '#':
            type = SOCK_DGRAM;
            break;

        case 'b':
            if (*options == '*') {
                backlog = va_arg(va, int);
            } else {
                backlog = atoi(options);
                do options++; while (isdigit(*options));
            }

            break;

        case 'C':
            saddr = va_arg(va, struct sockaddr *);
            saddrlen = va_arg(va, socklen_t);
            should_listen = false;
            break;

        case 'c':
            should_listen = false;
            break;

        case 'l':
            should_listen = true;
            break;

        case 'n':
            flags |= O_NONBLOCK;
            break;

        case 'r':
            reuse = 1;
            break;

        case 'u':
            family = AF_UNIX;
            break;

        default:
            va_end(va);
            errno= EINVAL;
            return -1;
        }
    }

    va_end(va);

    int fd = socket(family, type, 0);
    if (fd < 0)
        return -1;

    if (reuse && setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0)
        goto error;

    if (flags && socketflags(fd, flags) < 0)
        goto error;

    if (should_listen) {
        if (backlog < 1)
            backlog = DEFAULT_BACKLOG;

        if (listen(fd, backlog) < 0)
            goto error;
    } else {
        if (saddr && bind(fd, saddr, saddrlen) < 0)
            goto error;

        if (connect(fd, addr, addrlen) < 0 && errno != EINPROGRESS)
            goto error;
    }

    return fd;

error:
    err = errno;
    close(fd);
    errno = err;
    return -1;
}


