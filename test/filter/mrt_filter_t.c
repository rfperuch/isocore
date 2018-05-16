#include "test.h"

#include <isolario/filterpacket.h>
#include <isolario/mrt.h>
#include <isolario/bgp.h>


void testmrtfilter(void)
{
    filter_vm_t vm;

    int err = filter_compile(&vm, "NOT packet.withdrawn IN 127.0.0.1/22");
    if (err != 0)
        printf("compilation failed: %s\n", filter_last_error());

    netaddr_t addr;
    stonaddr(&addr, "127.0.0.1/20");

    setbgpwrite(BGP_UPDATE);
    startwithdrawn();
    putwithdrawn(&addr);
    endwithdrawn();
    bgpfinish(NULL);

    bgp_filter(&vm, NULL, 0);
    filter_destroy(&vm);
}

