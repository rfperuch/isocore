#include <CUnit/CUnit.h>
#include <isolario/patriciatrie.h>
#include <isolario/util.h>
#include <test_util.h>
#include <stdlib.h>

#include "test.h"

void testpatbase(void)
{
    patricia_trie_t pt;
    patinit(&pt, AFI_IPV4);
    
    int inserted;
    trienode_t *n = patinsertc(&pt, "8.2.0.0/16", &inserted);
    CU_ASSERT_FATAL(n != NULL);
    CU_ASSERT(inserted == PREFIX_INSERTED);
    CU_ASSERT(n->prefix.family == AF_INET);
    CU_ASSERT(strcmp(naddrtos(&n->prefix, NADDR_CIDR), "8.2.0.0/16") == 0);
    
    trienode_t *m = patsearchexactc(&pt, "8.2.0.0/16");
    CU_ASSERT(m == n);
    
    n = patinsertc(&pt, "9.2.0.0/16", &inserted);
    CU_ASSERT_FATAL(n != NULL);
    CU_ASSERT(inserted == PREFIX_INSERTED);
    CU_ASSERT(n->prefix.family == AF_INET);
    CU_ASSERT(strcmp(naddrtos(&n->prefix, NADDR_CIDR), "9.2.0.0/16") == 0);
    
    m = patsearchexactc(&pt, "9.2.0.0/16");
    CU_ASSERT(m == n);
    
    n = patsearchbestc(&pt, "8.2.2.0/24");
    
    CU_ASSERT_FATAL(n != NULL);
    CU_ASSERT(inserted == PREFIX_INSERTED);
    CU_ASSERT(n->prefix.family == AF_INET);
    CU_ASSERT(strcmp(naddrtos(&n->prefix, NADDR_CIDR), "8.2.0.0/16") == 0);
    
    patremovec(&pt, "8.2.0.0/16");
    m = patsearchexactc(&pt, "8.2.0.0/16");
    CU_ASSERT(m == NULL);
    
    patdestroy(&pt);
}

void testpatgetfuncs(void)
{
    patricia_trie_t pt;
    patinit(&pt, AFI_IPV4);
    
    patinsertc(&pt, "8.0.0.0/8", NULL);
    patinsertc(&pt, "8.2.0.0/16", NULL);
    patinsertc(&pt, "8.2.2.0/24", NULL);
    patinsertc(&pt, "8.2.2.1/32", NULL);
    patinsertc(&pt, "9.2.2.1/32", NULL);
    
    trienode_t** supernets = patgetsupernetsofc(&pt, "8.2.2.1/32");
    CU_ASSERT_FATAL(supernets != NULL);
    CU_ASSERT_FATAL(supernets[0] != NULL);
    CU_ASSERT_FATAL(supernets[1] != NULL);
    CU_ASSERT_FATAL(supernets[2] != NULL);
    CU_ASSERT_FATAL(supernets[3] != NULL);
    
    printf("\nExpected supernets of 8.2.2.1/32: 8.0.0.0/8, 8.2.0.0/16, 8.2.2.1/32\n");
    printf("Result:\n");
    int i;
    for (i = 0; supernets[i] != NULL; i++) {
        printf("%s\n", naddrtos(&supernets[i]->prefix, NADDR_CIDR));
    }
    printf("--\n");
    
    CU_ASSERT(i == 4);
    CU_ASSERT(supernets[4] == NULL);
    free(supernets);
    
    supernets = patgetsupernetsofc(&pt, "9.2.2.1/32");
    CU_ASSERT_FATAL(supernets != NULL);
    CU_ASSERT_FATAL(supernets[0] != NULL);
    CU_ASSERT_FATAL(supernets[1] == NULL);
    free(supernets);
    
    patdestroy(&pt);
}

void testpatiterator(void)
{
    patricia_trie_t pt;
    patinit(&pt, AFI_IPV4);
    
    patinsertc(&pt, "0.0.0.0/0", NULL);
    patinsertc(&pt, "8.0.0.0/8", NULL);
    patinsertc(&pt, "8.2.0.0/16", NULL);
    patinsertc(&pt, "8.2.2.0/24", NULL);
    patinsertc(&pt, "8.2.2.1/32", NULL);
    patinsertc(&pt, "9.2.2.1/32", NULL);
    patinsertc(&pt, "128.2.2.1/32", NULL);
    
    printf("\n");
    
    patiteratorinit(&pt);
    while (!patiteratorend()) {
        trienode_t *node = patiteratorget();
        printf("%s\n", naddrtos(&node->prefix, NADDR_CIDR));
        patiteratornext();
    }
    
    patdestroy(&pt);
}
