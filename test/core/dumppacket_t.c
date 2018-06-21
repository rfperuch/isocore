#include <CUnit/CUnit.h>
#include <isolario/bgp.h>
#include <isolario/bgpattribs.h>
#include <isolario/dumppacket.h>
#include <isolario/util.h>
#include <arpa/inet.h>
#include <test_util.h>

#include "test.h"

void testbgpdumppacketrow(void)
{
    setbgpwrite(BGP_UPDATE);

    unsigned char buf[255];

    startbgpattribs();

    bgpattr_t *origin = (bgpattr_t *)buf;
    origin->code = ORIGIN_CODE;
    origin->len = ORIGIN_LENGTH;
    origin->flags = DEFAULT_ORIGIN_FLAGS;

    setorigin(origin, ORIGIN_IGP);
    putbgpattrib(origin);

    struct sockaddr_in nh;
    inet_pton(AF_INET, "1.2.3.4", &nh);
    bgpattr_t *nexthop = (bgpattr_t *)buf;
    nexthop->code = NEXT_HOP_CODE;
    nexthop->len = NEXT_HOP_LENGTH;
    nexthop->flags = DEFAULT_NEXT_HOP_FLAGS;
    setnexthop(nexthop, nh.sin_addr);
    putbgpattrib(nexthop);

    bgpattr_t *aspath = (bgpattr_t *)buf;
    aspath->code = AS_PATH_CODE;
    aspath->len = 0;
    aspath->flags = DEFAULT_AS_PATH_FLAGS;

    uint32_t as[] = {2598, 137, 3356};
    putasseg32(aspath, AS_SEGMENT_SEQ, as, 3);
    putbgpattrib(aspath);

    endbgpattribs();

    struct in_addr nlri_s;
    inet_pton(AF_INET, "10.0.0.0", &nlri_s);
    netaddr_t nlri;
    makenaddr(&nlri, &nlri_s, 8);
    startnlri();
    putnlri(&nlri);
    endnlri();

    size_t pktlen;
    bgpfinish(&pktlen);

    bgp_msg_t *pkt = getbgp();

    printbgp_r(stdout, pkt, "r");
}
