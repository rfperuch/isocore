#include <CUnit/CUnit.h>
#include <isolario/bgp.h>
#include <isolario/dumppacket.h>
#include <isolario/util.h>
#include <arpa/inet.h>
#include <test_util.h>

#include "test.h"

void testbgpdumppacketrow(void)
{
    setbgpwrite(BGP_UPDATE);
    
    bgpattr_t origin;
    setbgporigin(&origin, ORIGIN_IGP);

    struct sockaddr_in nh;
    inet_pton(AF_INET, "1.2.3.4", &nh);
    bgpattr_t nexthop;
    setnexthop(&nexthop, nh.sin_addr);

    bgpattr_t aspath;

    uint32_t as = 2598;
    putasseg32(&aspath, AS_SEGMENT_SEQ, &as, 1);
    as = 137;
    putasseg32(&aspath, AS_SEGMENT_SEQ, &as, 2);
    as = 3356;
    putasseg32(&aspath, AS_SEGMENT_SEQ, &as, 3);

    startbgpattribs();
    putbgpattrib(&origin);
    putbgpattrib(&nexthop);
    putbgpattrib(&aspath);
    endbgpattribs();

    struct sockaddr_in nlri_s;
    inet_pton(AF_INET, "10.0.0.0", &nlri_s);
    netaddr_t nlri;
    makenaddr(&nlri, &nlri_s.sin_addr, 8);
    startnlri();
    putnlri(&nlri);
    endnlri();

    size_t pktlen;
    void *pkt = bgpfinish(&pktlen);
        
}
