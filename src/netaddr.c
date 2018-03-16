#include <isolario/log.h>

void makenaddr(netaddr_t *ip, const void *sin, int bitlen)
{
    ip->bitlen = bitlen;
    memcpy(&ip->sin, sin, sizeof(struct in_addr));
}

void makenaddr6(netaddr_t *ip, const void *sin6, int bitlen)
{
    ip->bitlen = bitlen;
    memcpy(&ip->sin6, sin6, sizeof(struct in6_addr));
}

int stonaddr(netaddr_t *ip, const char *s)
{
    char *last = strrchr(s, '/');

    int af = AF_INET;
    void *dst = &ip->sin;
    unsigned int maxbitlen = 32;
    
    for (int i = 0; i <= 4; i++) {
        if (s[i] == ':') {
            af = AF_INET6;
            dst = &ip->sin6;
            maxbitlen = 128;
            break;
        }
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

    return 0;
}

