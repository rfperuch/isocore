#include <CUnit/CUnit.h>
#include <isolario/patriciatrie.h>
#include <isolario/util.h>
#include <test_util.h>

#include "test.h"

void testpatbase(void)
{
    patricia_trie_t pt;
    patinit(&pt, AFI_IPV4);
    
    int inserted;
    trienode_t *n = patinsertc(&pt, "8.2.0.0/16", &inserted);
    CU_ASSERT_FATAL(n != NULL);
    CU_ASSERT(n->prefix.family == AF_INET);
    CU_ASSERT(strcmp(naddrtos(&n->prefix, NADDR_CIDR), "8.2.0.0/16") == 0);
    
    trienode_t *m = patsearchexactc(&pt, "8.2.0.0/16");
    CU_ASSERT(m == n);
    
    n = patinsertc(&pt, "9.2.0.0/16", &inserted);
    CU_ASSERT_FATAL(n != NULL);
    CU_ASSERT(n->prefix.family == AF_INET);
    CU_ASSERT(strcmp(naddrtos(&n->prefix, NADDR_CIDR), "9.2.0.0/16") == 0);
    
    m = patsearchexactc(&pt, "9.2.0.0/16");
    CU_ASSERT(m == n);
    
    n = patsearchbestc(&pt, "8.2.2.0/24");
    
    CU_ASSERT_FATAL(n != NULL);
    CU_ASSERT(n->prefix.family == AF_INET);
    CU_ASSERT(strcmp(naddrtos(&n->prefix, NADDR_CIDR), "8.2.0.0/16") == 0);
    
    patremovec(&pt, "8.2.0.0/16");
    m = patsearchexactc(&pt, "8.2.0.0/16");
    CU_ASSERT(m == NULL);
    
    patdestroy(&pt);
}
