#include <isolario/bgp.h>
#include <isolario/bgpattribs.h>
#include <isolario/dumppacket.h>
#include <stdio.h>

enum {
    MAX_PRINTABLE_ATTR_CODE = 35
};

typedef struct {
    void (*print_hdr)(const void* pkt, FILE *out);
    void (*print_wd)(const void* pkt, FILE *out);
    void (*print_nlri)(const void* pkt, FILE *out);
    void (*funcs[MAX_PRINTABLE_ATTR_CODE])(const void*, FILE *out);
} bgp_formatter_t;

/*
 * utility function: scan each attribute and calls the appropriate printing function
 *
 * WARNING: the formatter must have been properly initialized!
 * 
 */
static void print_bgp_attrs(const bgp_formatter_t *formatter, const bgp_msg_t *pkt, FILE *out)
{


}

// row formatting
static void print_bgphdr_row(const bgp_msg_t *pkt, FILE *out)
{
    int type = getbgptype();
    size_t length = getbgplength();
    fprintf(out, "%d|%d", type, length);
}

static void print_wd_row(const bgp_msg_t *pkt, FILE *out)
{

}

static void print_attr_origin_row(const void *origin, FILE *out)
{

}

static void print_attr_aspath_row(const void *aspath, FILE *out)
{

}

static void print_nlri_row(const void *nlri, FILE *out)
{

}

static void setup_bgp_formatter_row(bgp_formatter_t *bgp_formatter)
{
    bgp_formatter->print_hdr = print_bgphdr_row;
    bgp_formatter->print_wd = print_wd_row;
    bgp_formatter->funcs[ORIGIN_CODE] = print_attr_origin_row;
    bgp_formatter->funcs[AS_PATH_CODE] = print_attr_aspath_row;
    bgp_formatter->print_nlri = print_nlri_row;
}

void *print_bgp_r(const bgp_msg_t *pkt, FILE *out, const char *fmt, ...)
{
    bgp_formatter_t bgp_formatter;
  
    // on the basis of fmt call the appropriate setup
    while (*fmt != '\0') {
        switch(*fmt++) {
        case 'r':
            setup_bgp_formatter_row(&bgp_formatter);
            break;
        default:
            break;
        }
    }
    
    bgp_formatter.print_hdr(pkt, out);
    bgp_formatter.print_wd(pkt, out);
    print_bgp_attrs(&bgp_formatter, pkt, out);
    bgp_formatter.print_nlri(pkt, out);
}


