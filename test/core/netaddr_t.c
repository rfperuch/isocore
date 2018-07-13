#include <CUnit/CUnit.h>
#include <isolario/netaddr.h>
#include <isolario/util.h>
#include <test_util.h>

#include "test.h"

void testnetaddr(void)
{
    struct {
        const char* ip;
        const char* cidr;
        short bitlen;
        short family;
    } table[] = {
        {"127.0.0.1", "127.0.0.1/32", 32, AF_INET},
        {"8.2.0.0", "8.2.0.0/16", 16, AF_INET},
        {"::", "::/0", 0, AF_INET6},
        {"2a00:1450:4002:800::2002", "2a00:1450:4002:800::2002/127", 127, AF_INET6},
        {"2a00:1450:4002:800::2003", "2a00:1450:4002:800::2003/128", 128, AF_INET6},
        {"2001:67c:1b08:3:1::1", "2001:67c:1b08:3:1::1/128", 128, AF_INET6}
    };

    netaddr_t prefix;
    int res;

    for (size_t i = 0; i < nelems(table); i++) {
        res = stonaddr(&prefix, table[i].cidr);
        CU_ASSERT_EQUAL(res, 0);
        CU_ASSERT_EQUAL(prefix.family, table[i].family);
        CU_ASSERT_EQUAL(prefix.bitlen, table[i].bitlen);
        CU_ASSERT_STRING_EQUAL(naddrtos(&prefix, NADDR_CIDR), table[i].cidr);
        CU_ASSERT_STRING_EQUAL(naddrtos(&prefix, NADDR_PLAIN), table[i].ip);

        netaddr_t cloned;
        if (table[i].family == AF_INET)
            makenaddr(&cloned, &prefix.sin, prefix.bitlen);
        else
            makenaddr6(&cloned, &prefix.sin, prefix.bitlen);

        if ((table[i].bitlen == 32 && table[i].family == AF_INET) || (table[i].bitlen == 128 && table[i].family == AF_INET6)) {
            res = stonaddr(&prefix, table[i].ip); // NB
            CU_ASSERT_EQUAL(res, 0);
            CU_ASSERT_EQUAL(prefix.family, table[i].family);
            CU_ASSERT_EQUAL(prefix.bitlen, table[i].bitlen);
            CU_ASSERT_STRING_EQUAL(naddrtos(&prefix, NADDR_CIDR), table[i].cidr);
            CU_ASSERT_STRING_EQUAL(naddrtos(&prefix, NADDR_PLAIN), table[i].ip);
        }

        CU_ASSERT_EQUAL(cloned.family, table[i].family);
        CU_ASSERT_EQUAL(cloned.bitlen, table[i].bitlen);
        CU_ASSERT_STRING_EQUAL(naddrtos(&cloned, NADDR_CIDR), table[i].cidr);
        CU_ASSERT_STRING_EQUAL(naddrtos(&cloned, NADDR_PLAIN), table[i].ip);
    }
}
