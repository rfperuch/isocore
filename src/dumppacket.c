#include <ctype.h>
#include <inttypes.h>
#include <isolario/bgp.h>
#include <isolario/bgpattribs.h>
#include <isolario/branch.h>
#include <isolario/dumppacket.h>
#include <isolario/strutil.h>
#include <isolario/util.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

enum {
    BGPF_ISRIB   = 1 << 0,
    BGPF_HASFDR  = 1 << 1,
    BGPF_HASTIME = 1 << 2
};

typedef struct bgp_formatter_s bgp_formatter_t;

struct bgp_formatter_s {
    void (*dumpbgp)(FILE *out, bgp_msg_t *pkt, const bgp_formatter_t *fmt);
    void (*dumpstatechange)(FILE *out, const bgp4mp_header_t *bgphdr, const bgp_formatter_t *fmt);

    size_t assiz;
    struct timespec stamp;
    netaddr_t fdrip;
    uint32_t fdras;
    int comm_mode;
    unsigned int flags;
    unsigned int attr_mask[0xff / sizeof(unsigned int) + 1];  // interesting attributes mask
};

/// @brief Optimized unlocked write string to FILE.
static void writestr_unlocked(const char *s, FILE *out)
{
#ifdef __GNUC__
    extern int fputs_unlocked(const char *s, FILE *out);

    fputs_unlocked(s, out);
#else
    char c;
    while ((c = *s++) != '\0')
        putc_unlocked(c, out);
#endif
}

static void printbgp_row_attribs(FILE *out, bgp_msg_t *pkt, const bgp_formatter_t *fmt)
{
    netaddr_t *addr;
    as_pathent_t *p;

    int segno = -1;
    int type = AS_SEGMENT_SEQ;

    char buf[digsof(unsigned long) + 1];

    // real AS path
    int idx = 0;
    startrealaspath_r(pkt, fmt->assiz);
    while ((p = nextaspath_r(pkt)) != NULL) {
        if (segno != p->segno) {
            if (type == AS_SEGMENT_SET)
                putc_unlocked('}', out);

            if (idx > 0)
                putc_unlocked(' ', out);

            if (p->type == AS_SEGMENT_SET)
                putc_unlocked('{', out);

            type  = p->type;
            segno = p->segno;
            idx   = 0;
        }
        if (idx > 0)
            putc_unlocked((type == AS_SEGMENT_SET) ? ',' : ' ', out);

        writestr_unlocked(ultoa(buf, NULL, p->as), out);
        idx++;
    }
    if (type == AS_SEGMENT_SET)
        putc_unlocked('}', out);  // close pending segment

    endaspath_r(pkt);

    putc_unlocked('|', out);
    // NEXT_HOP attributes
    idx = 0;

    startnhop_r(pkt);
    while ((addr = nextnhop_r(pkt)) != NULL) {
        if (idx > 0)
            putc_unlocked(' ', out);

        writestr_unlocked(naddrtos(addr, NADDR_PLAIN), out);
    }

    endnhop_r(pkt);

    // Origin
    putc_unlocked('|', out);
    bgpattr_t *attr = getbgporigin_r(pkt);
    if (attr) {
        char c;
        switch (getorigin(attr)) {
        case ORIGIN_IGP:
            c = 'i';
            break;
        case ORIGIN_EGP:
            c = 'e';
            break;
        case ORIGIN_INCOMPLETE:
            c = '?';
            break;
        default:
            c = '\0';
            break;
        }
        if (c != '\0')
            putc_unlocked(c, out);
    }
    putc_unlocked('|', out);

    // Atomic aggregate
    attr = getbgpatomicaggregate_r(pkt);

     if (attr)
        writestr_unlocked("AT", out);

    putc_unlocked('|', out);

    // Aggregator
    attr = getrealbgpaggregator_r(pkt, fmt->assiz);
    if (attr) {
        char asbuf[INET_ADDRSTRLEN + 1];

        uint32_t as = getaggregatoras(attr);
        struct in_addr in = getaggregatoraddress(attr);
        if (inet_ntop(AF_INET, &in, buf, sizeof(buf))) {
            writestr_unlocked(ultoa(asbuf, NULL, as), out);
            putc_unlocked(' ', out);
            writestr_unlocked(buf, out);
        }
    }

    putc_unlocked('|', out);

    // Communities
    size_t comm_count = 0, lcomm_count = 0;
    attr = getbgpcommunities_r(pkt);
    if (attr) {
        community_t *comms = getcommunities(attr, sizeof(*comms), &comm_count);
        for (size_t i = 0; i < comm_count; i++) {
            if (i > 0)
                putc_unlocked(' ', out);

            writestr_unlocked(communitytos(comms[i], fmt->comm_mode), out);
        }
    }

    attr = getbgplargecommunities_r(pkt);
    if (attr) {
        large_community_t *comms = getcommunities(attr, sizeof(*comms), &lcomm_count);
        for (size_t i = 0; i < lcomm_count; i++) {
            // we want to emit a space even when packet had any community
            if (i + comm_count > 0)
                putc_unlocked(' ', out);

            writestr_unlocked(largecommunitytos(comms[i]), out);
        }
    }
}

static void printbgp_row_trailer(FILE *out, const bgp_formatter_t *fmt)
{
    char buf[digsof(unsigned long long) + 1];

    // Feeder address
    if (fmt->flags & BGPF_HASFDR) {
        writestr_unlocked(naddrtos(&fmt->fdrip, NADDR_PLAIN), out);
        putc_unlocked(' ', out);
        writestr_unlocked(ultoa(buf, NULL, fmt->fdras), out);
    }

    putc_unlocked('|', out);

    // Timestamp
    if (fmt->flags & BGPF_HASTIME) {
        writestr_unlocked(ulltoa(buf, NULL, fmt->stamp.tv_sec), out);

        unsigned long long msec = fmt->stamp.tv_nsec;
        if (msec > 0) {
            putc_unlocked('.', out);
            writestr_unlocked(ulltoa(buf, NULL, msec), out);
        }
    }

    putc_unlocked('|', out);
    // ASN32bit
    putc_unlocked((fmt->assiz == sizeof(uint32_t)) ? '1' : '0', out);
}

static void printbgp_row(FILE *out, bgp_msg_t *pkt, const bgp_formatter_t *fmt)
{
    char c = '+';
    if (fmt->flags & BGPF_ISRIB)
        c = '=';

    int idx = 0;
    netaddr_t *addr;

    startallnlri_r(pkt);
    while ((addr = nextnlri_r(pkt)) != NULL) {
        if (idx == 0) {
            putc_unlocked(c, out);
            putc_unlocked('|', out);
        } else {
            putc_unlocked(' ', out);
        }
        writestr_unlocked(naddrtos(addr, NADDR_CIDR), out);
        idx++;
    }
    endnlri_r(pkt);

    if (idx > 0) {
        putc_unlocked('|', out);
        printbgp_row_attribs(out, pkt, fmt);
        putc_unlocked('|', out);
        printbgp_row_trailer(out, fmt);
        putc_unlocked('\n', out);
    }

    if ((fmt->flags & BGPF_ISRIB) == 0) {
        idx = 0;

        startallwithdrawn_r(pkt);
        while ((addr = nextwithdrawn_r(pkt)) != NULL) {
            if (idx == 0) {
                putc_unlocked('-', out);
                putc_unlocked('|', out);
            } else {
                putc_unlocked(' ', out);
            }
            writestr_unlocked(naddrtos(addr, NADDR_CIDR), out);
            idx++;
        }
        endwithdrawn_r(pkt);

        if (idx > 0) {
            writestr_unlocked("|||||||", out);
            printbgp_row_trailer(out, fmt);
            putc_unlocked('\n', out);
        }
    }
}

static void printstatechange_row(FILE *out, const bgp4mp_header_t *bgphdr, const bgp_formatter_t *fmt)
{
    char buf[digsof(int) + 1 + digsof(int) + 1];
    char *ptr;

    putc_unlocked('#', out);
    putc_unlocked('|', out);
    utoa(buf, &ptr, bgphdr->old_state);
    *ptr++ = '-';
    utoa(ptr, NULL, bgphdr->new_state);
    writestr_unlocked(buf, out);
    writestr_unlocked("|||||||", out);
    printbgp_row_trailer(out, fmt);
    putc_unlocked('\n', out);
}

// FMT: <format>:<mask>
void printbgpv_r(FILE *out, bgp_msg_t *pkt, const char *fmt, va_list va)
{
    bgp_formatter_t bgpfmt;
    memset(&bgpfmt, 0, sizeof(bgpfmt));

    if (*fmt == '#') {
        // BGP from RIB snapshot
        bgpfmt.flags |= BGPF_ISRIB;
        fmt++;
    }

    switch (*fmt) {
    case 'r':
        bgpfmt.dumpbgp = printbgp_row;
        fmt++;
        break;
    default:
        bgpfmt.dumpbgp = printbgp_row;
        break;
    }

    char c;
    while ((c = *fmt++) != '\0') {
        switch (c) {
        case 'A':
            // AS size
            if (isdigit((unsigned char) *fmt)) {
                bgpfmt.assiz = (unsigned long long) atoll(fmt);
                do fmt++; while (isdigit((unsigned char) *fmt));
            } else if (*fmt == '*') {
                bgpfmt.assiz = va_arg(va, int);
            }

            if (*fmt == 'b') {
                bgpfmt.assiz /= 8;  // size specied in bit
                fmt++;
            } else if (*fmt == 'B') {
                fmt++;  // explicit tag for size in bytes, for symmetry
            }

            if (bgpfmt.assiz != sizeof(uint16_t) && bgpfmt.assiz != sizeof(uint32_t))
                bgpfmt.assiz = sizeof(uint16_t);

            break;
        case 'F':
            // feeder
            if (*fmt == '*') {
                bgpfmt.fdrip  = *va_arg(va, const netaddr_t *);
                bgpfmt.fdras  = va_arg(va, uint32_t);
                bgpfmt.flags |= BGPF_HASFDR;
                fmt++;
            } else {
                // direct address + AS
                char buf[INET6_ADDRSTRLEN + 1];
                int i;

                for (i = 0; i < (int) sizeof(buf); i++) {
                    if (isspace((unsigned char) *fmt))
                        break;

                    buf[i] = *fmt++;
                }
                if (i == (int) sizeof(buf))
                    break;  // bad address

                buf[i] = '\0';

                // skip spaces
                do fmt++; while (isspace((unsigned char) *fmt));

                // AS
                if (!isdigit((unsigned char) *fmt))
                    break;

                long long as = atoll(fmt);
                do fmt++; while (isdigit((unsigned char) *fmt));

                if (unlikely(as > UINT32_MAX))
                    break;

                bgpfmt.fdras = as;
                if (inet_pton(AF_INET6, buf, &bgpfmt.fdrip.sin6) > 0) {
                    bgpfmt.fdrip.family = AF_INET6;
                    bgpfmt.fdrip.bitlen = 128;
                    bgpfmt.flags |= BGPF_HASFDR;
                } else if (inet_pton(AF_INET, buf, &bgpfmt.fdrip.sin) > 0) {
                    bgpfmt.fdrip.family = AF_INET;
                    bgpfmt.fdrip.bitlen = 32;
                    bgpfmt.flags |= BGPF_HASFDR;
                }
            }
            break;
        case 'p':
            bgpfmt.comm_mode = COMMSTR_PLAIN;
            break;
        case 't':
            bgpfmt.stamp.tv_sec  = va_arg(va, time_t);
            bgpfmt.stamp.tv_nsec = 0;
            bgpfmt.flags |= BGPF_HASTIME;
            break;
        case 'T':
            bgpfmt.stamp = *va_arg(va, const struct timespec *);
            bgpfmt.flags |= BGPF_HASTIME;
            break;
        default:
            break;
        }
    }

    bgpfmt.dumpbgp(out, pkt, &bgpfmt);
}

void printbgp_r(FILE *out, bgp_msg_t *msg, const char *fmt, ...)
{
    va_list va;

    va_start(va, fmt);
    printbgpv_r(out, msg, fmt, va);
    va_end(va);
}

void printbgpv(FILE *out, const char *fmt, va_list va)
{
    printbgpv_r(out, getbgp(), fmt, va);
}

void printbgp(FILE *out, const char *fmt, ...)
{
    va_list va;

    va_start(va, fmt);
    printbgpv(out, fmt, va);
    va_end(va);
}

typedef struct mrt_formatter_s mrt_formatter_t;

struct mrt_formatter_s {
    bgp_formatter_t bgpfmt;

    void (*printer)(FILE *out, mrt_msg_t *msg, const mrt_formatter_t *fmt);
};

static void printmrt_bgp4mp(FILE *out, mrt_msg_t *msg, const mrt_header_t *hdr, bgp_formatter_t *bgpfmt)
{
    bgp_msg_t bgp;
    size_t n;
    void *data;

    const bgp4mp_header_t *bgphdr = getbgp4mpheader_r(msg);
    if (unlikely(!bgphdr))
        return; // FIXME should report bad packet?

    bgpfmt->fdrip  = bgphdr->peer_addr;
    bgpfmt->fdras  = bgphdr->peer_as;
    bgpfmt->stamp  = hdr->stamp;
    bgpfmt->flags |= BGPF_HASFDR | BGPF_HASTIME;

    switch (hdr->subtype) {
    case BGP4MP_STATE_CHANGE:
    case BGP4MP_STATE_CHANGE_AS4:
        bgpfmt->assiz = (hdr->subtype == BGP4MP_STATE_CHANGE_AS4) ? sizeof(uint32_t) : sizeof(uint16_t);
        bgpfmt->dumpstatechange(out, bgphdr, bgpfmt);
        break;
    case BGP4MP_MESSAGE:
    case BGP4MP_MESSAGE_LOCAL:
    case BGP4MP_MESSAGE_AS4:
    case BGP4MP_MESSAGE_AS4_LOCAL:
        data = unwrapbgp4mp_r(msg, &n);
        setbgpread_r(&bgp, data, n, BGPF_NOCOPY);
        if (hdr->subtype == BGP4MP_MESSAGE_AS4 || hdr->subtype == BGP4MP_MESSAGE_AS4_LOCAL)
            bgpfmt->assiz = sizeof(uint32_t);
        else
            bgpfmt->assiz = sizeof(uint16_t);

        bgpfmt->dumpbgp(out, &bgp, bgpfmt);
        bgpclose_r(&bgp);
        break;
    default:
        // FIXME print hex dump?
        break;
    }
}

void printmrtv_r(FILE *out, mrt_msg_t *msg, const char *fmt, va_list va)
{
    (void) va; // unused right now...

    mrt_formatter_t mrtfmt;
    memset(&mrtfmt, 0, sizeof(mrtfmt));

    bgp_formatter_t *bgpfmt = &mrtfmt.bgpfmt;
    switch (*fmt) {
    case 'r':
        bgpfmt->dumpbgp         = printbgp_row;
        bgpfmt->dumpstatechange = printstatechange_row;
        fmt++;
        break;
    default:
        bgpfmt->dumpbgp         = printbgp_row;
        bgpfmt->dumpstatechange = printstatechange_row;
        break;
    }

    // dump contents
    const mrt_header_t *hdr = getmrtheader_r(msg);
    switch (hdr->type) {
    case MRT_TABLE_DUMPV2:
        // handle_dumpv2(hdr);  // TODO
        break;

    case MRT_BGP4MP:
    case MRT_BGP4MP_ET:
        printmrt_bgp4mp(out, msg, hdr, bgpfmt);
        break;

    default:
        break; // XXX maybe print hex dump?
    }
}

void printmrt_r(FILE *out, mrt_msg_t *msg, const char *fmt, ...)
{
    va_list va;

    va_start(va, fmt);
    printmrtv_r(out, msg, fmt, va);
    va_end(va);
}

void printmrtv(FILE *out, const char *fmt, va_list va)
{
    printmrtv_r(out, getmrt(), fmt, va);
}

void printmrt(FILE *out, const char *fmt, ...)
{
    va_list va;

    va_start(va, fmt);
    printmrtv(out, fmt, va);
    va_end(va);
}
