#include <isolario/endian.h>
#include <isolario/netaddr.h>
#include <isolario/strutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

extern int naddrsize(int bitlen);
extern sa_family_t saddrfamily(const char *s);
extern void makenaddr(netaddr_t *ip, sa_family_t family, const void *sin, int bitlen);

int stonaddr(netaddr_t *ip, const char *s)
{
    char *last = strrchr(s, '/');

    int af = saddrfamily(s);

    void *dst = &ip->sin;
    unsigned int maxbitlen = 32;

    if (af == AF_INET6) {
        dst = &ip->sin6;
        maxbitlen = 128;
    }
    
    ip->bitlen = 0;
    ip->family = AF_UNSPEC;
    memset(&ip->sin6, 0, sizeof(ip->sin6));

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

extern int prefixeqwithmask(const netaddr_t *a, const netaddr_t *b, unsigned int mask);

extern int prefixeq(const netaddr_t *a, const netaddr_t *b);

extern int naddreq(const netaddr_t *a, const netaddr_t *b);

char* naddrtos(const netaddr_t *ip, int mode)
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

int isnaddrreserved(const netaddr_t *ip)
{
    if (ip->family == AF_INET6) {
        if (ip->bitlen == 0)
            return 1;

        uint16_t a = ip->u16[0];
        a = frombig16(a);

        uint16_t b = ip->u16[1];
        b = frombig16(b);

        // 2000::/3 is the only one routable
        if (a < 8192 || a > 16383)
            return 1;

        // 2001:0000::/23
        if (a == 8193)
            if (b <= 511)
                return 1;

        // 2001:db8::/32
        if (a == 8193 && b == 3512)
            return 1;

        // 2001:10::/28
        if (a == 8193)
            if (b >= 16 && b <= 31)
                return 1;

        // 2002::/16
        if (a == 8194)
            return 1;
    } else {
    
        if (ip->bitlen == 0)
            return 1;

        unsigned char a = ip->bytes[0];
        unsigned char b = ip->bytes[1];
        unsigned char c = ip->bytes[2];

        if (a == 10 || a == 127 || a >= 224)
            return 1;

        if (a == 100)
            if (b >= 64 && b <= 127)
                return 1;

        if (a == 169 && b == 254)
            return 1;

        if (a == 172)
            if (b >= 16 && b <= 31)
                return 1;

        if (a == 192 && b == 0 && c == 2)
            return 1;

        if (a == 192 && b == 88 && c == 99)
            return 1;

        if (a == 192 && b == 0 && c == 0)
            return 1;

        if (a == 198)
            if (b == 18 || b == 19)
                return 1;

        if (a == 198 && b == 51 && c == 100)
            return 1;

        if (a == 203 && b == 0 && c == 113)
            return 1;
    }
    return 0;
}
