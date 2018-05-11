#ifndef ISOLARIO_DUMPPACKET_H_
#define ISOLARIO_DUMPPACKET_H_

#include <isolario/bgp.h>
#include <stdio.h>

void print_bgp_r(const bgp_msg_t *pkt, FILE *out, const char *fmt, ...);

#endif
