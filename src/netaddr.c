#include <isolario/endian.h>
#include <isolario/netaddr.h>
#include <isolario/strutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

extern int naddrsize(int bitlen);
extern sa_family_t saddrfamily(const char *s);

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

extern int prefixeqwithmask(const void *a, const void *b, unsigned int mask);

extern int prefixeq(const netaddr_t *a, const netaddr_t *b);

extern int naddreq(const netaddr_t *a, const netaddr_t *b);

char* naddrtos(const netaddr_t* ip, int mode)
{
    static _Thread_local char buf[INET6_ADDRSTRLEN + 1 + digsof(ip->bitlen) + 1];

    char *ptr;
    int i, best, max;
    switch (ip->family) {
    case AF_INET:
        utoa(buf, &ptr, ip->bytes[0]);
        *ptr++ = '.';
        utoa(ptr, &ptr, ip->bytes[1]);
        *ptr++ = '.';
        utoa(ptr, &ptr, ip->bytes[2]);
        *ptr++ = '.';
        utoa(ptr, &ptr, ip->bytes[3]);
        break;
    case AF_INET6:
        if (ip->u32[0] == 0 && ip->u32[1] == 0 && ip->u16[4] == 0 && ip->u16[5] == 0xff) {
            // v4 mapped to v6 (starts with 0:0:0:0:0:0:ffff)
            memcpy(buf, "::ffff:", 7);
            ptr = buf + 7;

            utoa(ptr, &ptr, ip->bytes[0]);
            *ptr++ = '.';
            utoa(ptr, &ptr, ip->bytes[1]);
            *ptr++ = '.';
            utoa(ptr, &ptr, ip->bytes[2]);
            *ptr++ = '.';
            utoa(ptr, &ptr, ip->bytes[3]);
            break;
        }

        // typical v6
        xtoa(buf, &ptr, frombig16(ip->u16[0]));
        *ptr++ = ':';
        xtoa(ptr, &ptr, frombig16(ip->u16[1]));
        *ptr++ = ':';
        xtoa(ptr, &ptr, frombig16(ip->u16[2]));
        *ptr++ = ':';
        xtoa(ptr, &ptr, frombig16(ip->u16[3]));
        *ptr++ = ':';
        xtoa(ptr, &ptr, frombig16(ip->u16[4]));
        *ptr++ = ':';
        xtoa(ptr, &ptr, frombig16(ip->u16[5]));
        *ptr++ = ':';
        xtoa(ptr, &ptr, frombig16(ip->u16[6]));
        *ptr++ = ':';
        xtoa(ptr, &ptr, frombig16(ip->u16[7]));

        // replace longest /(^0|:)[:0]{2,}/ with "::"
        for (i = best = 0, max = 2; buf[i] != '\0'; i++) {
            if (i > 0 && buf[i] != ':')
                continue;

            // count the number of consecutive 0 in this substring
            int n = 0;
            for (int j = i; buf[j] != '\0'; j++) {
                if (buf[j] != ':' && buf[j] != '0')
                    break;

                n++;
            }

            // store the position if this is the best we've seen so far
            if (n > max) {
                best = i;
                max = n;
            }
        }

        ptr = buf + i;
        if (max > 3) {
            // we can compress the string
            buf[best] = ':';
            buf[best + 1] = ':';

            int len = i - best - max;
            memmove(buf + best + 2, buf + best + max, len + 1);
            ptr = buf + best + 2 + len;
        }

        break;
    default:
        return "invalid";
    }


    if (mode == NADDR_CIDR) {
        *ptr++ = '/';
        utoa(ptr, NULL, ip->bitlen);
    }

    return buf;
}

