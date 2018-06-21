#include "test.h"

#include <CUnit/CUnit.h>
#include <isolario/filterintrin.h>
#include <isolario/filterpacket.h>
#include <isolario/mrt.h>
#include <isolario/bgp.h>
#include <stdbool.h>

enum {
    MY_FUN
};

static void miao(filter_vm_t *vm)
{
    printf("Test from callback! %p\n", (void *) vm);
    vm_pushvalue(vm, true);
}

void testmrtfilter(void)
{
    filter_vm_t vm;

    int err = filter_compile(&vm, "( packet.nlri EXACT 127.0.0.1 OR packet.withdrawn SUPERNET [ 127.0.0.1/22 192.0.0.0/24 $0 ] ) AND CALL $[0]", MY_FUN);
    if (err != 0)
        printf("compilation failed: %s\n", filter_last_error());

    stonaddr(&vm.kp[0].addr, "192.0.0.0/24");
    vm.funcs[MY_FUN] = miao;

    netaddr_t addr;
    stonaddr(&addr, "127.0.0.1");

    setbgpwrite(BGP_UPDATE);
    startnlri();
    putnlri(&addr);
    endnlri();

    stonaddr(&addr, "192.0.0.1");

    startwithdrawn();
    putwithdrawn(&addr);
    endwithdrawn();
    if (!bgpfinish(NULL))
        CU_FAIL_FATAL("BGP packet creation failed!");

    filter_dump(stdout, &vm);

    int result = bgp_filter(&vm);
    printf("result: %d (%s)\n", result, filter_strerror(result));
    filter_destroy(&vm);
}

