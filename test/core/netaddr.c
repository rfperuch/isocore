#include <CUnit/CUnit.h>
#include <isolario/netaddr.h>
#include <isolario/util.h>
#include <test_util.h>

#include "test.h"

void testnetaddr(void)
{
    netaddr_t prefix;
    int res = stonaddr(&prefix, "127.0.0.1/32");
    CU_ASSERT_EQUAL(res, 0);
    CU_ASSERT_EQUAL(prefix.family, AF_INET);
    CU_ASSERT_EQUAL(prefix.bitlen, 32);
    CU_ASSERT_STRING_EQUAL(naddrtos(&prefix, NADDR_CIDR), "127.0.0.1/32");
    
    netaddr_t cloned;
    makenaddr(&cloned, &prefix.sin, prefix.bitlen);
    CU_ASSERT_EQUAL(cloned.family, AF_INET);
    CU_ASSERT_EQUAL(cloned.bitlen, 32);
    CU_ASSERT_STRING_EQUAL(naddrtos(&cloned, NADDR_CIDR), "127.0.0.1/32");
    
    res = stonaddr(&prefix, "2a00:1450:4002:800::2003/128");
    CU_ASSERT_EQUAL(res, 0);
    CU_ASSERT_EQUAL(prefix.bitlen, 128);
    CU_ASSERT_STRING_EQUAL(naddrtos(&prefix, NADDR_CIDR), "2a00:1450:4002:800::2003/128");
    CU_ASSERT_STRING_EQUAL(naddrtos(&prefix, NADDR_PLAIN), "2a00:1450:4002:800::2003");
    
    makenaddr6(&cloned, &prefix.sin6, prefix.bitlen);
    CU_ASSERT_EQUAL(cloned.family, AF_INET6);
    CU_ASSERT_EQUAL(cloned.bitlen, 128);
    CU_ASSERT_STRING_EQUAL(naddrtos(&cloned, NADDR_CIDR), "2a00:1450:4002:800::2003/128");
    
}
