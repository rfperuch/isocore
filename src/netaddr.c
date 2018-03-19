#include <isolario/netaddr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

extern int naddrsize(int bitlen);
extern int saddrfamily(const char *s);

void makenaddr(netaddr_t *ip, const void *sin, int bitlen)
{
    ip->bitlen = bitlen;
    ip->family = AF_INET;
    memcpy(&ip->sin, sin, sizeof(struct in_addr));
}

void makenaddr6(netaddr_t *ip, const void *sin6, int bitlen)
{
    ip->bitlen = bitlen;
    ip->family = AF_INET6;
    memcpy(&ip->sin6, sin6, sizeof(struct in6_addr));
}

int stonaddr(netaddr_t *ip, const char *s)
{
    char *last = strrchr(s, '/');

    int af = saddrfamily(s);

    void *dst = &ip->sin;
    unsigned int maxbitlen = 32;
    
    if (af == AF_INET6) {
        dst = &ip->sin6;
        dst = &ip->sin6;
        maxbitlen = 128;
    }

    size_t n;
    long bitlen;
    if (last) {
        n = last - s;
        last++;
        char* endptr;
        errno = 0;
        bitlen = strtol(last, &endptr, 10);

        if (bitlen < 0 || bitlen > maxbitlen)
            errno = ERANGE;
        if (last == endptr || *endptr != '\0')
            errno = EINVAL;
        if (errno != 0)
            return -1;
    } else {
        n =  strlen(s);
        bitlen = maxbitlen;
    }

    char buf[n + 1];
    memcpy(buf, s, n);
    buf[n] = '\0';

    if (inet_pton(af, buf, dst) != 1)
        return -1;

    ip->bitlen = bitlen;
    ip->family = af;

    return 0;
}

char* naddrtos(const netaddr_t* ip, int mode)
{
    static _Thread_local char buf[INET6_ADDRSTRLEN + 5];
    
    if (inet_ntop(ip->family, &ip->sin, buf, sizeof(buf)) == NULL)
        return "invalid";
    
    if (mode == NADDR_CIDR)
        sprintf(buf + strlen(buf), "/%d", ip->bitlen);
    
    return buf;
}

