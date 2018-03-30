/**
 * @file isolario/patricia_trie.h
 *
 * @brief a (binary) Patricia Trie implementation
 */

#ifndef ISOLARIO_PATRICIA_TRIE_H_
#define ISOLARIO_PATRICIA_TRIE_H_

#include <isolario/netaddr.h>
#include <stdbool.h>

enum {
    PREFIX_INSERTED,
    PREFIX_ALREADY_PRESENT,
};

enum {
    SUPERNET_ITERATOR,
    SUBNET_ITERATOR,
};

typedef struct {
    netaddr_t prefix;
    void *payload;
} trienode_t;

typedef union pnode_s pnode_t;
typedef struct nodepage_s nodepage_t;

typedef struct {
    pnode_t* head;
    afi_t type;
    unsigned int nprefs;
    nodepage_t* pages;
    pnode_t* freenodes;
} patricia_trie_t;

void patinit(patricia_trie_t* pt, afi_t type);

void patdestroy(patricia_trie_t* pt);

trienode_t* patinsertn(patricia_trie_t *pt, const netaddr_t *prefix, int *inserted);

trienode_t* patinsertc(patricia_trie_t *pt, const char *cprefix, int *inserted);

trienode_t* patsearchexactn(const patricia_trie_t *pt, const netaddr_t *prefix);

trienode_t* patsearchexactc(const patricia_trie_t *pt, const char *cprefix);

trienode_t* patsearchbestn(const patricia_trie_t *pt, const netaddr_t *prefix);

trienode_t* patsearchbestc(const patricia_trie_t *pt, const char *cprefix);

void* patremoven(patricia_trie_t *pt, const netaddr_t *prefix);

void* patremovec(patricia_trie_t *pt, const char *prefix);

void patiteratorinit(patricia_trie_t *pt);

trienode_t* patiteratorget();

void patiteratornext();

bool patiteratorend();

/**
 * @brief Supernets of a prefix
 *
 * @return The supernets of the provided prefix
 *
 * @note If present, the provided prefix is returned into the result.
 * If not NULL, the returned value must bee free'd by the caller
 *
 */
trienode_t** patgetsupernetsofn(const patricia_trie_t *pt, const netaddr_t *prefix);

trienode_t** patgetsupernetsofc(const patricia_trie_t *pt, const char *cprefix);

#endif

