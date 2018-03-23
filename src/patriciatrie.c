#include <isolario/patriciatrie.h>
#include <isolario/util.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

enum {
    PATRICIA_GLUE_NODE = 1,
};


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
    if (memcmp(&(addr->bytes[0]), &(dest->bytes[0]), mask / 8) == 0) {
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
        int bit = (n->prefix.bitlen < maxbits) && (prefix->bytes[n->prefix.bitlen >> 3] & 0x80 >> n->prefix.bitlen & 0x07);
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
        
        int bit = (n->prefix.bitlen < maxbits) && (prefix->bytes[n->prefix.bitlen >> 3] & 0x80 >> n->prefix.bitlen & 0x07);
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
    
    *inserted = true;
    
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

/*void patblah(trie_node_t *n)
{
    pnode_t *actual = (pnode_t *) n;
}*/
