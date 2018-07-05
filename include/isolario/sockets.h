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

#ifndef ISOLARIO_SOCKETS_H_
#define ISOLARIO_SOCKETS_H_

#include <isolario/funcattribs.h>
#include <string.h>
#include <sys/socket.h>

#define TCP_LISTENING_SOCKET "4l"
#define TCP6_LISTENING_SOCKET "6l"
#define TCP_NONBLOCK_LISTENING_SOCKET "4ln"
#define TCP6_NONBLOCK_LISTENING_SOCKET "6ln"
#define UNIX_LISTENING_SOCKET "ul"
#define UNIX_NONBLOCK_LISTENING_SOCKET "uln"

#define TCP_CONNECT_SOCKET "4c"
#define TCP6_CONNECT_SOCKET "6c"
#define TCP_NONBLOCK_CONNECT_SOCKET "4cn"
#define TCP6_NONBLOCK_CONNECT_SOCKET "6cn"
#define UNIX_CONNECT_SOCKET "uc"
#define UNIX_NONBLOCK_CONNECT_SOCKET "ucn"

nonnull(1, 3) wur int fsockopen(const struct sockaddr *addr, socklen_t addrlen, const char *mode, ...);

nonnull(1) purefunc inline int hashv4(const struct in_addr *addr)
{
    return addr->s_addr;
}

nonnull(1) purefunc inline int hashv6(const struct in6_addr *addr6)
{
    uint64_t hi, lo;
    memcpy(&hi, &addr6->s6_addr[0], sizeof(hi));
    memcpy(&lo, &addr6->s6_addr[sizeof(hi)], sizeof(lo));

    return ((hi << 5) + hi + lo);
}

nonnull(1, 2) purefunc inline int sockaddrincmp(const struct sockaddr *a, const struct sockaddr *b)
{
    const struct sockaddr_in *first = (const struct sockaddr_in *)a;
    const struct sockaddr_in *second = (const struct sockaddr_in *)b;

    return (first->sin_addr.s_addr > second->sin_addr.s_addr) - (second->sin_addr.s_addr > first->sin_addr.s_addr);
}

nonnull(1, 2) purefunc inline int sockaddrin6cmp(const struct sockaddr *a, const struct sockaddr *b)
{
    const struct sockaddr_in6 *first = (const struct sockaddr_in6 *)a;
    const struct sockaddr_in6 *second = (const struct sockaddr_in6 *)b;

    return memcmp(first->sin6_addr.s6_addr, second->sin6_addr.s6_addr, sizeof(first->sin6_addr.s6_addr));
}

#endif

