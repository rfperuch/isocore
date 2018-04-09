#include <isolario/patriciatrie.h>
#include <isolario/util.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

enum {
    PATRICIA_GLUE_NODE = 1,
};

/*
    the "public" node is trienode_t. (see patriciatrie.h).
    It contains a prefix and a payload.
    
    the actual node is a pnode_s.
*/
union pnode_s {
    trienode_t pub;  // keep underlying structure in sync!
    struct {
        netaddr_t prefix;
        void *payload;
        union pnode_s *parent; ///< the least significant bit indicates whether the node is glue
        union pnode_s *children[2]; ///< 0 = left, 1 = right
    };
    union pnode_s *next; ///< used when the node is a free node (to create a linked list of free nodes)
};

/*
    The patricia memory is allocated in pages.
    A list of pages is maintained.
    Each page is a block of 256 nodes.
    Each node can be free or not. Each node has the possibility to be in a list
    of free nodes. When no more free nodes are available, a new page is allocated.

*/
struct nodepage_s {
    struct nodepage_s *next;
    pnode_t block[256];
};

static void pnodeinit(pnode_t *n, const netaddr_t *prefix)
{
    memset(n, 0, sizeof(*n));

    if (prefix)
        n->prefix = *prefix;
    else
        n->prefix.family = AF_BAD;
}

static pnode_t *getpnodeparent(pnode_t *n)
{
    return (pnode_t *) (((uintptr_t) n->parent) & ~PATRICIA_GLUE_NODE);
}

static void setpnodeparent(pnode_t *n, pnode_t *parent)
{
    n->parent = (pnode_t *) ((uintptr_t) n->parent & PATRICIA_GLUE_NODE);
    n->parent = (pnode_t *) ((uintptr_t) n->parent | (uintptr_t) parent);
}

static bool ispnodeglue(pnode_t *n)
{
    return ((uintptr_t)n->parent) & PATRICIA_GLUE_NODE;
}

static void setpnodeglue(pnode_t *n)
{
    n->parent = (pnode_t *) ((uintptr_t) n->parent | PATRICIA_GLUE_NODE);
}

static void resetpnodeglue(pnode_t* n)
{
    n->parent = (pnode_t *)((uintptr_t) n->parent & ~PATRICIA_GLUE_NODE);
}

static void patallocpage(patricia_trie_t *pt)
{
    nodepage_t *p = malloc(sizeof(*p));
    if (unlikely(!p))
        return;

    for (size_t i = 0; i < nelems(p->block); i++) {
        pnode_t *n = &p->block[i];

        n->next = pt->freenodes;
        pt->freenodes = n;
    }

    p->next = pt->pages;
    pt->pages = p;
}

static pnode_t* getfreenode(patricia_trie_t *pt)
{
    pnode_t *n = pt->freenodes;
    if (!n) {
        patallocpage(pt);
        n = pt->freenodes;
        if (unlikely(!n))
            return NULL;
    }
    pt->freenodes = pt->freenodes->next;

    return n;
}

static void putfreenode(patricia_trie_t *pt, pnode_t *n)
{
    pnode_t* save = pt->freenodes;
    pt->freenodes = n;
    n->next = save;
}

void patinit(patricia_trie_t *pt, afi_t type)
{
    pt->head = NULL;
    pt->type = type;
    pt->nprefs = 0;
    pt->pages = NULL;
    pt->freenodes = NULL;
}

void patdestroy(patricia_trie_t *pt)
{
    nodepage_t *ptr = pt->pages;

    while (ptr) {
        nodepage_t* next = ptr->next;
        free(ptr);
        ptr = next;
    }
}

static int patmaxbits(const patricia_trie_t *pt)
{
    return (pt->type == AFI_IPV4) ? 32 : 128;
}

static int patcompwithmask(const netaddr_t *addr, const netaddr_t *dest, int mask)
{
    if (memcmp(&addr->bytes[0], &dest->bytes[0], mask / 8) == 0) {
        int n = mask / 8;
        int m = ((unsigned int)(~0) << (8 - (mask % 8)));

        if (((mask & 0x7) == 0) || ((addr->bytes[n] & m) == (dest->bytes[n] & m)))
            return 1;
    }
    
    return 0;
}

trienode_t* patinsertn(patricia_trie_t *pt, const netaddr_t *prefix, int *inserted)
{
    int dummy;
    if (!inserted)
        inserted = &dummy;

    pnode_t *n;

    *inserted = PREFIX_ALREADY_PRESENT;

    if (pt->head == NULL) {
        pnode_t *n = getfreenode(pt);
        if (unlikely(!n))
            return NULL;

        pnodeinit(n, prefix);
        pt->head = n;
        pt->nprefs++;
        *inserted = PREFIX_INSERTED;

        return &n->pub;
    }

    n = pt->head;
    int maxbits = patmaxbits(pt);

    while (n->prefix.bitlen < prefix->bitlen || ispnodeglue(n)) {
        int bit = (n->prefix.bitlen < maxbits) && (prefix->bytes[n->prefix.bitlen >> 3] & (0x80 >> n->prefix.bitlen & 0x07));
        if (n->children[bit] == NULL)
            break;
            
        n = n->children[bit];
    }
    
    
    int check_bit = (n->prefix.bitlen < prefix->bitlen) ? n->prefix.bitlen : prefix->bitlen;
    int differ_bit = 0;
    
    int r;
    for (int i = 0, z = 0; z < check_bit; i++, z += 8) {
        if ((r = (prefix->bytes[i] ^ n->prefix.bytes[i])) == 0) {
            differ_bit = z + 8;
            continue;
        }

        int j;
        for (j = 0; j < 8; j++) {
            if (r & (0x80 >> j))
                break;
        }

        differ_bit = z + j;
        break;
    }
    
    if (differ_bit > check_bit)
        differ_bit = check_bit;

    pnode_t* parent = getpnodeparent(n);
    while (parent && parent->prefix.bitlen >= differ_bit) {
        n = parent;
        parent = getpnodeparent(n);
    }
    
    if (differ_bit == prefix->bitlen && n->prefix.bitlen == prefix->bitlen) {
        if (ispnodeglue(n))
            return &n->pub;

        pt->nprefs++;
        n->prefix = *prefix;
        resetpnodeglue(n);

        return &n->pub;
    }
    
    pnode_t *newnode = getfreenode(pt);
    if (unlikely(!newnode))
        return NULL;

    pnodeinit(newnode, prefix);
    pt->nprefs++;
   
    if (n->prefix.bitlen == differ_bit) {
        setpnodeparent(newnode, n);
        
        int bit = (n->prefix.bitlen < maxbits) && (prefix->bytes[n->prefix.bitlen >> 3] & 0x80 >> (n->prefix.bitlen & 0x07));
        n->children[bit] = newnode;

        *inserted = PREFIX_INSERTED;

        return &newnode->pub;
    }
    
    if (n->prefix.bitlen == differ_bit) {
        int bit = (prefix->bitlen < maxbits) && (n->prefix.bytes[n->prefix.bitlen >> 3] & 0x80 >> (prefix->bitlen & 0x07));
        newnode->children[bit] = n;
        setpnodeparent(newnode, n);
        
        if (!getpnodeparent(n)) {
            pt->head = newnode;
        } else if (getpnodeparent(n)->children[1] == n) {
            int b = (getpnodeparent(n)->children[1] == n);
            getpnodeparent(n)->children[b] = n;
        }
        setpnodeparent(n, newnode);
    } else {
        pnode_t *glue = getfreenode(pt);
        if (unlikely(!glue))
            return NULL;
            
        pnodeinit(glue, NULL);
        glue->prefix.bitlen = differ_bit;
        setpnodeglue(glue);
        setpnodeparent(glue, getpnodeparent(n));

        int bit = (differ_bit < maxbits) && (prefix->bytes[differ_bit >> 3] & (0x80 >> (differ_bit & 0x07)));

        glue->children[bit] = newnode;
        glue->children[!bit] = n;

        setpnodeparent(newnode, glue);
        if (!getpnodeparent(n)) {
            pt->head = glue;
        } else {
            int bit = (getpnodeparent(n)->children[1] == n);
            getpnodeparent(n)->children[bit] = glue;
        }

        setpnodeparent(n, glue);
    }
    
    *inserted = PREFIX_INSERTED;

    return &newnode->pub;
}

trienode_t* patinsertc(patricia_trie_t *pt, const char *cprefix, int *inserted)
{
    netaddr_t prefix;
    if (stonaddr(&prefix, cprefix) != 0)
        return NULL;

    return patinsertn(pt, &prefix, inserted);
}

trienode_t* patsearchexactn(const patricia_trie_t *pt, const netaddr_t *prefix)
{
    if (!pt->head)
        return NULL;

    pnode_t *n = pt->head;
    
    while (n->prefix.bitlen < prefix->bitlen) {
        int bit = (prefix->bytes[n->prefix.bitlen >> 3] & (0x80 >> (n->prefix.bitlen & 0x07)));
        n = n->children[bit];

        if (!n)
            return NULL;
    }
    
    if (n->prefix.bitlen > prefix->bitlen || ispnodeglue(n))
        return NULL;

    if (patcompwithmask(&n->prefix, prefix, prefix->bitlen))
        return &n->pub;

    return NULL;
}

trienode_t* patsearchexactc(const patricia_trie_t *pt, const char *cprefix)
{
    netaddr_t prefix;
    if (stonaddr(&prefix, cprefix) != 0)
        return NULL;
    
    return patsearchexactn(pt, &prefix);
}

trienode_t* patsearchbestn(const patricia_trie_t *pt, const netaddr_t *prefix)
{
    if (!pt->head)
        return NULL;
    
    pnode_t *n = pt->head;
    pnode_t *last = NULL;
    
    while (n->prefix.bitlen < prefix->bitlen) {
        if (!ispnodeglue(n)) {
            if ((n->prefix.bitlen <= prefix->bitlen) && (patcompwithmask(&n->prefix, prefix, n->prefix.bitlen)))
                last = n;
            else
                break;
        }
        
        int bit = ((prefix->bytes[n->prefix.bitlen >> 3] & (0x80 >> (n->prefix.bitlen & 0x07))) != 0);
        n = n->children[bit];
        
        if (!n)
            break;
    }
    
    return &last->pub;
}

trienode_t* patsearchbestc(const patricia_trie_t *pt, const char *cprefix)
{
    netaddr_t prefix;
    if (stonaddr(&prefix, cprefix) != 0)
        return NULL;
    
    return patsearchbestn(pt, &prefix);
}

void* patremoven(patricia_trie_t *pt, const netaddr_t *prefix)
{
    pnode_t *n = (pnode_t*)patsearchexactn(pt, prefix);
    
    pt->nprefs--;
    
    if (!n)
        return NULL;
        
    void* payload = n->payload;
    
    if (n->children[0] && n->children[1]) {
        putfreenode(pt, n);
        setpnodeglue(n);
        return payload;
    }
    
    pnode_t *parent;
    pnode_t *child;
    
    if (!n->children[0] && !n->children[1]) {
        parent = getpnodeparent(n);
        putfreenode(pt, n);
        
        if (!parent) {
            pt->head = NULL;
            return payload;
        }
        
        int bit = (parent->children[1] == n);
        parent->children[bit] = NULL;
        child = parent->children[!bit];
        
        if (!ispnodeglue(parent))
            return payload;
            
        // if here, the parent is glue then we need to remove the parent too
        
        if (!getpnodeparent(parent)) {
            pt->head = child;
        } else {
            int bit = (getpnodeparent(parent)->children[1] == parent);
            getpnodeparent(parent)->children[bit] = child;
        }
        setpnodeparent(child, getpnodeparent(parent));
        putfreenode(pt, parent);
        
        return payload;
    }
    
    
    int bit = (n->children[1] != NULL);
    child = n->children[bit];
    
    parent = getpnodeparent(n);
    setpnodeparent(child, parent);
    
    putfreenode(pt, n);
    
    if (!parent) {
        pt->head = child;
        return payload;
    }
    
    bit = (parent->children[1] == n);
    parent->children[bit] = child;
    
    return payload;
}

void* patremovec(patricia_trie_t *pt, const char *cprefix)
{
    netaddr_t prefix;
    if (stonaddr(&prefix, cprefix) != 0)
        return NULL;
    
    return patremoven(pt, &prefix);
}

trienode_t** patgetsupernetsofn(const patricia_trie_t *pt, const netaddr_t *prefix)
{
    if (!pt->head)
        return NULL;

    pnode_t *n = pt->head;
    
    trienode_t **res = calloc(1, (prefix->bitlen + 2) * sizeof(trienode_t *));
    
    int i = 0;

    while (n && n->prefix.bitlen < prefix->bitlen) {
        if (!ispnodeglue(n)) {
            if (n->prefix.bitlen < patmaxbits(pt) && patcompwithmask(&n->prefix, prefix, n->prefix.bitlen)) {
                res[i++] = &n->pub;
            } else {
                res[i] = NULL;
                return res;
            }
        }

        int bit = (n->prefix.bitlen < patmaxbits(pt)) && (prefix->bytes[n->prefix.bitlen >> 3] & (0x80 >> (n->prefix.bitlen & 0x07)));
        n = n->children[bit];
    }
    
    if (n && !ispnodeglue(n) && n->prefix.bitlen <= prefix->bitlen && patcompwithmask(&n->prefix, prefix, prefix->bitlen))
        res[i++] = &n->pub;
    
    res[i] = NULL;
    
    return res;
}

trienode_t** patgetsupernetsofc(const patricia_trie_t *pt, const char *cprefix)
{
    netaddr_t prefix;
    if (stonaddr(&prefix, cprefix) != 0)
        return NULL;
    
    return patgetsupernetsofn(pt, &prefix);
}

trienode_t** patgetsubnetsofn(const patricia_trie_t *pt, const netaddr_t *prefix)
{
    if (!pt->head)
        return NULL;
    
    pnode_t *start = pt->head;
    
    while (start && start->prefix.bitlen < prefix->bitlen) {
        int bit = prefix->bytes[start->prefix.bitlen >> 3] & (0x80 >> (start->prefix.bitlen & 0x07));
        start = start->children[bit];
    }
    
    uint64_t n;
    if (prefix->family == AF_INET || prefix->bitlen >= 96) {
        n = (1ull << (patmaxbits(pt) - prefix->bitlen));
        if (pt->nprefs < n)
            n = pt->nprefs;
    } else {
        n = pt->nprefs;
    }
    
    n++;
    
    trienode_t **res = calloc(1, n*sizeof(trienode_t*));
    
    int i = 0;
    
    pnode_t *node;
    
    pnode_t *stack[patmaxbits(pt)+1];
    pnode_t **sp = stack;
    pnode_t *next = start;
    while ((node = next)) {
        if (!ispnodeglue(node)) {
            if (patcompwithmask(&node->prefix, prefix, prefix->bitlen)) {
                res[i++] = &node->pub;
            } else {
                break;
            }
        }
        
        if (next->children[0]) {
            if (next->children[1]) {
                *sp++ = next->children[1];
            }
            next = next->children[0];
        } else if (next->children[1]) {
            next = next->children[1];
        } else if (sp != stack) {
            next = *(sp--);
        } else {
            next = NULL;
        }
    }
    
    res[i] = NULL;
    
    return res;
}

trienode_t** patgetsubnetsofc(const patricia_trie_t *pt, const char *cprefix)
{
    netaddr_t prefix;
    if (stonaddr(&prefix, cprefix) != 0)
        return NULL;
    
    return patgetsubnetsofn(pt, &prefix);
}

trienode_t** patgetrelatedofn(const patricia_trie_t *pt, const netaddr_t *prefix)
{
    if (!pt->head)
        return NULL;
    
    pnode_t *start = pt->head;
    
    uint64_t n;
    if (prefix->family == AF_INET || prefix->bitlen >= 96) {
        n = (1ull << (patmaxbits(pt) - prefix->bitlen));
        if (pt->nprefs < n)
            n = pt->nprefs;
    } else {
        n = pt->nprefs;
    }
    
    n += prefix->bitlen + 3;
    
    trienode_t **res = calloc(1, n*sizeof(trienode_t*));
    
    int i = 0;
    while (start && start->prefix.bitlen < prefix->bitlen) {
        if (!ispnodeglue(start)) {
            if (patcompwithmask(&start->prefix, prefix, start->prefix.bitlen)) {
                res[i++] = &start->pub;
            } else {
                return res;
            }
        }
        
        int bit = prefix->bytes[start->prefix.bitlen >> 3] & (0x80 >> (start->prefix.bitlen) & 0x07);
        start = start->children[bit];
    }
    
    pnode_t *node;
    pnode_t *stack[patmaxbits(pt)+1];
    pnode_t **sp = stack;
    pnode_t *next = start;
    while ((node = next)) {
        if (!ispnodeglue(node)) {
            if (patcompwithmask(&node->prefix, prefix, prefix->bitlen)) {
                res[i++] = &node->pub;
            } else {
                break;
            }
        }
        
        if (next->children[0]) {
            if (next->children[1]) {
                *sp++ = next->children[1];
            }
            next = next->children[0];
        } else if (next->children[1]) {
            next = next->children[1];
        } else if (sp != stack) {
            next = *(sp--);
        } else {
            next = NULL;
        }
    }
    
    res[i] = NULL;
    
    return res;
    
}

trienode_t** patgetrelatedofc(const patricia_trie_t *pt, const char *cprefix)
{
    netaddr_t prefix;
    if (stonaddr(&prefix, cprefix) != 0)
        return NULL;
    
    return patgetrelatedofn(pt, &prefix);
}

uint128_t patcoverage(const patricia_trie_t *pt)
{
    uint128_t coverage = UINT128_ZERO;
    
    pnode_t *n;
    pnode_t *stack[patmaxbits(pt) + 1];
    pnode_t **sp = stack;
    pnode_t *next = pt->head;
    
    while ((n = next)) {
        if (!ispnodeglue(n) && n->prefix.bitlen != 0) {
            coverage = u128add(coverage, u128shl(UINT128_ONE, (patmaxbits(pt) - n->prefix.bitlen)));
            if (sp != stack)
                next = *(--sp);
            else
                next = NULL;
            
            continue;
        }
        
        if (next->children[0]) {
            if (next->children[1])
                *sp++ = next->children[1];
            next = n->children[0];
        } else if (next->children[1]) {
            next = next->children[1];
        } else if (sp != stack) {
            next = *(--sp);
        } else {
            next = NULL;
        }
    }
    
    return coverage;
}

trienode_t** patgetfirstsubnetsofn(const patricia_trie_t *pt, const netaddr_t *prefix)
{
    if (!pt->head)
        return NULL;
    
    pnode_t *node = pt->head;

    while (node && node->prefix.bitlen < prefix->bitlen) {
        int bit = prefix->bytes[node->prefix.bitlen >> 3] & (0x80 >> (node->prefix.bitlen & 0x07));
        node = node->children[bit];
    }

    pnode_t *stack[patmaxbits(pt) + 1];
    pnode_t **sp = stack;
    pnode_t *next = node;

    uint64_t n;
    if (prefix->family == AF_INET || prefix->bitlen >= 96) {
        n = (1ull << (patmaxbits(pt) - prefix->bitlen));
        if (pt->nprefs < n)
            n = pt->nprefs;
    } else {
        n = pt->nprefs;
    }
    
    n++;
    
    trienode_t **res = calloc(1, n*sizeof(trienode_t*));
    
    int i = 0;
    while ((node = next)) {
        if (!ispnodeglue(node) && node->prefix.bitlen != 0) {
            res[i++] = &node->pub;
            if (sp != stack)
                next = *(--sp);
            else
                next = NULL;
            
            continue;
        }
        
        if (next->children[0]) {
            if (next->children[1])
                *sp++ = next->children[1];
            next = node->children[0];
        } else if (next->children[1]) {
            next = next->children[1];
        } else if (sp != stack) {
            next = *(--sp);
        } else {
            next = NULL;
        }
    }
    
    res[i] = NULL;
    
    return res;
}

trienode_t** patgetfirstsubnetsofc(const patricia_trie_t *pt, const char *cprefix)
{
    netaddr_t prefix;
    if (stonaddr(&prefix, cprefix) != 0)
        return NULL;
    
    return patgetfirstsubnetsofn(pt, &prefix);
}

/* Iterator */

static _Thread_local struct {
    pnode_t *stack[129];
    pnode_t **sp;
    pnode_t *curr;
} patiteratorstate;

void patiteratormovenext()
{
    pnode_t *l = patiteratorstate.curr->children[0];
    pnode_t *r = patiteratorstate.curr->children[1];
    
    if (l) {
        if (r) {
            *patiteratorstate.sp++ = r;
        }
        patiteratorstate.curr = l;
    } else if (r) {
        patiteratorstate.curr = r;
    } else if (patiteratorstate.sp != patiteratorstate.stack) {
        patiteratorstate.curr = *(--patiteratorstate.sp);
    } else {
        patiteratorstate.curr = NULL;
    }
}

void patiteratorskipglue()
{
    while (patiteratorstate.curr && ispnodeglue(patiteratorstate.curr))
        patiteratormovenext();
}


void patiteratorinit(patricia_trie_t *pt)
{
    patiteratorstate.sp = patiteratorstate.stack;
    patiteratorstate.curr = pt->head;
    patiteratorskipglue();
}

trienode_t* patiteratorget()
{
    return &patiteratorstate.curr->pub;
}

void patiteratornext()
{
    patiteratormovenext();
    patiteratorskipglue();
}

bool patiteratorend()
{
    return (patiteratorstate.curr == NULL);
}

/*void patblah(trie_node_t *n)
{
    pnode_t *actual = (pnode_t *) n;
}*/

