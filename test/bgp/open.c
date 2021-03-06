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

#include <CUnit/CUnit.h>
#include <isolario/bgp.h>
#include <isolario/bgpparams.h>
#include <isolario/util.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void testopencreate(void)
{
    unsigned char buf[128];
    bgpcap_t cap;

    // struct in_addr ip;
    // inet_pton(AF_INET, "127.0.0.1", &ip);

    bgp_open_t op = {
        .version = BGP_VERSION,
        .my_as = AS_TRANS,
        .hold_time = BGP_HOLD_SECS
//        .iden = ip
    };

    // start writing a BGP open
    setbgpwrite(BGP_OPEN, BGPF_DEFAULT);
    // write relevant BGP open data
    setbgpopen(&op);

    startbgpcaps();
        cap.code = ASN32BIT_CODE;
        cap.len = ASN32BIT_LENGTH;
        putbgpcap(setasn32bit(&cap, 0xffffffff));

        cap.code = MULTIPROTOCOL_CODE;
        cap.len = MULTIPROTOCOL_LENGTH;
        putbgpcap(setmultiprotocol(&cap, AFI_IPV6, SAFI_UNICAST));

        cap.code = GRACEFUL_RESTART_CODE;
        cap.len  = GRACEFUL_RESTART_BASE_LENGTH;
        setgracefulrestart(&cap, RESTART_FLAG, 1600);
        putgracefulrestarttuple(&cap, AFI_IPV6, SAFI_UNICAST, FORWARDING_STATE);
        putbgpcap(&cap);
    endbgpcaps();

    // finish and obtain packet bytes
    size_t n;
    void *pkt = bgpfinish(&n);

    CU_ASSERT_PTR_NOT_NULL(pkt);
    CU_ASSERT_FATAL(n < sizeof(buf));

    memcpy(buf, pkt, n);

    printf("BGP_OPEN packet with size %zd\n", n);

    // start reading a BGP packet from buffer
    setbgpread(buf, n, BGPF_DEFAULT);

    // we want to find back what we wrote
    CU_ASSERT_EQUAL(getbgptype(), BGP_OPEN);
    CU_ASSERT_EQUAL(getbgpopen()->version, BGP_VERSION);
    CU_ASSERT_EQUAL(getbgpopen()->my_as, AS_TRANS);
    CU_ASSERT_EQUAL(getbgpopen()->hold_time, BGP_HOLD_SECS);
    // CU_ASSERT_EQUAL(memcmp(&(getbgpopen()->iden), &ip, sizeof(ip), 0);

    // parse BGP capabilities (we do expect the same we put in...)
    startbgpcaps();

    afi_safi_t tuples[1];
    bgpcap_t *pcap;
    while ((pcap = nextbgpcap()) != NULL) {
        switch (pcap->code) {
        case ASN32BIT_CODE:
            CU_ASSERT_EQUAL(pcap->len, ASN32BIT_LENGTH);
            CU_ASSERT_EQUAL(getasn32bit(pcap), 0xffffffff);
            break;
        case MULTIPROTOCOL_CODE:
            CU_ASSERT_EQUAL(pcap->len, MULTIPROTOCOL_LENGTH);
            CU_ASSERT_EQUAL(getmultiprotocol(pcap).afi, AFI_IPV6);
            CU_ASSERT_EQUAL(getmultiprotocol(pcap).safi, SAFI_UNICAST);
            CU_ASSERT_EQUAL(getmultiprotocol(pcap).flags, 0);
            break;
        case GRACEFUL_RESTART_CODE:
            CU_ASSERT_EQUAL(pcap->len, GRACEFUL_RESTART_BASE_LENGTH + sizeof(*tuples));
            CU_ASSERT_EQUAL(getgracefulrestartflags(pcap), RESTART_FLAG);
            CU_ASSERT_EQUAL(getgracefulrestarttime(pcap), 1600);

            n = getgracefulrestarttuples(tuples, nelems(tuples), pcap);

            CU_ASSERT_EQUAL(n, nelems(tuples));
            CU_ASSERT_EQUAL(tuples->afi, AFI_IPV6);
            CU_ASSERT_EQUAL(tuples->safi, SAFI_UNICAST);
            CU_ASSERT_EQUAL(tuples->flags, FORWARDING_STATE);
            break;
        default:
            CU_FAIL_FATAL("unexpected capability");
        }
    }
    endbgpcaps();

    // close BGP reading, we expect no error
    CU_ASSERT_EQUAL(bgpclose(), BGP_ENOERR);
}


/*

Pick bytes from another source other than the bytes generated by our code
and try to read them

Frame 6: 145 bytes on wire (1160 bits), 145 bytes captured (1160 bits)
Linux cooked capture
Internet Protocol Version 4, Src: 127.1.1.2, Dst: 127.1.1.3
Transmission Control Protocol, Src Port: 33683, Dst Port: 179, Seq: 1, Ack: 1, Len: 77
Border Gateway Protocol - OPEN Message
    Marker: ffffffffffffffffffffffffffffffff
    Length: 77
    Type: OPEN Message (1)
    Version: 4
    My AS: 65517
    Hold Time: 180
    BGP Identifier: 127.1.1.2
    Optional Parameters Length: 48
    Optional Parameters
        Optional Parameter: Capability
            Parameter Type: Capability (2)
            Parameter Length: 6
            Capability: Multiprotocol extensions capability
                Type: Multiprotocol extensions capability (1)
                Length: 4
                AFI: IPv4 (1)
                Reserved: 00
                SAFI: Unicast (1)
        Optional Parameter: Capability
            Parameter Type: Capability (2)
            Parameter Length: 2
            Capability: Route refresh capability (Cisco)
                Type: Route refresh capability (Cisco) (128)
                Length: 0
        Optional Parameter: Capability
            Parameter Type: Capability (2)
            Parameter Length: 2
            Capability: Route refresh capability
                Type: Route refresh capability (2)
                Length: 0
        Optional Parameter: Capability
            Parameter Type: Capability (2)
            Parameter Length: 6
            Capability: Support for 4-octet AS number capability
                Type: Support for 4-octet AS number capability (65)
                Length: 4
                AS Number: 65517
        Optional Parameter: Capability
            Parameter Type: Capability (2)
            Parameter Length: 6
            Capability: Support for Additional Paths
                Type: Support for Additional Paths (69)
                Length: 4
                AFI: IPv4 (1)
                SAFI: Unicast (1)
                Send/Receive: Both (3)
        Optional Parameter: Capability
            Parameter Type: Capability (2)
            Parameter Length: 8
            Capability: FQDN Capability
                Type: FQDN Capability (73)
                Length: 6
                Hostname Length: 4
                Hostname: bgpd
                Domain Name Length: 0
                Domain Name: 
        Optional Parameter: Capability
            Parameter Type: Capability (2)
            Parameter Length: 4
            Capability: Graceful Restart capability
                Type: Graceful Restart capability (64)
                Length: 2
                    [Expert Info (Chat/Request): Graceful Restart Capability supported in Helper mode only]
                        [Graceful Restart Capability supported in Helper mode only]
                        [Severity level: Chat]
                        [Group: Request]
                Restart Timers: 0x0078
                    0... .... .... .... = Restart: No
                    .... 0000 0111 1000 = Time: 120

*/
void testopenread(void)
{
    unsigned char buf[] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0x00, 0x4d, 0x01, 0x04, 0xff, 0xed, 0x00, 0xb4,
        0x7f, 0x01, 0x01, 0x02, 0x30, 0x02, 0x06, 0x01, 0x04, 0x00, 0x01, 0x00,
        0x01, 0x02, 0x02, 0x80, 0x00, 0x02, 0x02, 0x02, 0x00, 0x02, 0x06, 0x41,
        0x04, 0x00, 0x00, 0xff, 0xed, 0x02, 0x06, 0x45, 0x04, 0x00, 0x01, 0x01,
        0x03, 0x02, 0x08, 0x49, 0x06, 0x04, 0x62, 0x67, 0x70, 0x64, 0x00, 0x02,
        0x04, 0x40, 0x02, 0x00, 0x78
    };

    setbgpread(buf, sizeof(buf), BGPF_DEFAULT);

    CU_ASSERT_EQUAL(getbgptype(), BGP_OPEN);
    CU_ASSERT_EQUAL(getbgpopen()->version, BGP_VERSION);
    CU_ASSERT_EQUAL(getbgpopen()->my_as, 65517);
    CU_ASSERT_EQUAL(getbgpopen()->hold_time, 180);

    struct in_addr iden;
    inet_pton(AF_INET, "127.1.1.2", &iden);
    CU_ASSERT_EQUAL(memcmp(&(getbgpopen()->iden), &iden, sizeof(iden)), 0);

    startbgpcaps();
    bgpcap_t *cap;
    while ((cap = nextbgpcap()) != NULL) {
        switch (cap->code) {
        case MULTIPROTOCOL_CODE:
            CU_ASSERT_EQUAL(cap->len, MULTIPROTOCOL_LENGTH);
            CU_ASSERT_EQUAL(getmultiprotocol(cap).afi, AFI_IPV4);
            CU_ASSERT_EQUAL(getmultiprotocol(cap).safi, SAFI_UNICAST);
            break;
        default:
            CU_FAIL_FATAL("unexpected capability");
        }
    }
    endbgpcaps();

    bgpclose();
}
