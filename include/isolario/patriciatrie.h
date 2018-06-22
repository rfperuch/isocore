/**
 * @file isolario/patricia_trie.h
 *
 * @brief a (binary) Patricia Trie implementation
 */

#ifndef ISOLARIO_PATRICIA_TRIE_H_
#define ISOLARIO_PATRICIA_TRIE_H_

#include <isolario/netaddr.h>
#include <isolario/uint128_t.h>

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

/**
 * The patricia memory is allocated in pages.
 * A list of pages is maintained.
 * Each page is a block of 256 nodes.
 * Each node can be free or not. Each node has the possibility to be in a list
 * of free nodes. When no more free nodes are available, a new page is allocated.
*/
typedef struct {
    pnode_t* head;
    sa_family_t family;
    unsigned int nprefs;
    nodepage_t* pages;
    pnode_t* freenodes;
} patricia_trie_t;

void patinit(patricia_trie_t* pt, sa_family_t family);

/**
 * @brief Clear a Patricia Trie *without* freeing its memory.
 *
 * This is essentially a faster way to remove every node in the trie, while
 * preserving the allocated memory for further use.
 *
 * @param [in] pt Patricia Trie to be cleared, must not be \a NULL.
 *
 * @note Calling this function invalidates any node in the Patricia Trie,
 *       the user is responsible to perform any memory management to free()
 *       node resources stored in the \a payload field of each node, whenever
 *       necessary.
 */
void patclear(patricia_trie_t *pt);

/**
 * @brief Destroy a Patricia Trie, free()ing any allocated memory.
 *
 * @param [in] pt Patricia Trie to be free()d, must not be \a NULL.
 *
 * @note This function *does not* free() any memory contained in the
 *       \a payload field of each node, the user of the Patricia Trie is
 *       responsible for any memory management of such field, more importantly
 *       such memory management *must* be done before calling this function 
 */
void patdestroy(patricia_trie_t* pt);

trienode_t* patinsertn(patricia_trie_t *pt, const netaddr_t *prefix, int *inserted);

trienode_t* patinsertc(patricia_trie_t *pt, const char *cprefix, int *inserted);

trienode_t* patsearchexactn(const patricia_trie_t *pt, const netaddr_t *prefix);

trienode_t* patsearchexactc(const patricia_trie_t *pt, const char *cprefix);

trienode_t* patsearchbestn(const patricia_trie_t *pt, const netaddr_t *prefix);

trienode_t* patsearchbestc(const patricia_trie_t *pt, const char *cprefix);

void* patremoven(patricia_trie_t *pt, const netaddr_t *prefix);

void* patremovec(patricia_trie_t *pt, const char *prefix);

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

/**
 * @brief Check if supernets of a prefix
 *
 * @return 1 if the provided prefix is supernet of any patricia prefix,
 * 0 otherwise
 *
 */
int patissupernetofn(const patricia_trie_t *pt, const netaddr_t *prefix);

int patissupernetofc(const patricia_trie_t *pt, const char *cprefix);

/**
 * @brief Subnets of a prefix
 *
 * @return The subnets of the provided prefix
 *
 * @note If present, the provided prefix is returned into the result.
 * If not NULL, the returned value must bee free'd by the caller
 *
 */
trienode_t** patgetsubnetsofn(const patricia_trie_t *pt, const netaddr_t *prefix);

trienode_t** patgetsubnetsofc(const patricia_trie_t *pt, const char *cprefix);

/**
 * @brief Check subnet of a prefix
 *
 * @return 1 if the provided prefix is subnet of any patricia prefix,
 * 0 otherwise
 *
 */
int patissubnetofn(const patricia_trie_t *pt, const netaddr_t *prefix);

int patissubnetofc(const patricia_trie_t *pt, const char *cprefix);

/**
 * @brief Related of a prefix
 *
 * @return The related prefixes of the provided prefix
 *
 * @note If present, the provided prefix is returned into the result. FIXME?
 * If not NULL, the returned value must bee free'd by the caller
 *
 */
trienode_t** patgetrelatedofn(const patricia_trie_t *pt, const netaddr_t *prefix);

trienode_t** patgetrelatedofc(const patricia_trie_t *pt, const char *cprefix);

/**
 * @brief Check if related of a prefix
 *
 * @return 1 if the provided prefix is related to any of the patricia prefixes,
 * 0 otherwise
 */
int patisrelatedofn(const patricia_trie_t *pt, const netaddr_t *prefix);

int patisrelatedofc(const patricia_trie_t *pt, const char *cprefix);

/**
 * @brief Coverage of prefixes
 *
 * @return The amount of address space covered by the prefixes insrted into the patricia
 *
 * @note the default route is ignored
 *
 */
uint128_t patcoverage(const patricia_trie_t *pt);

/**
 * @brief Get the first subnets of a given prefix
 *
 * @return the subnets of first-level of a given prefix
 *
 */
trienode_t** patgetfirstsubnetsofn(const patricia_trie_t *pt, const netaddr_t *prefix);

trienode_t** patgetfirstsubnetsofc(const patricia_trie_t *pt, const char *cprefix);

/* Iterator */

typedef struct {
    pnode_t *stack[129];
    pnode_t **sp;
    pnode_t *curr;
} patiterator_t;

void patiteratorinit(patiterator_t *state, const patricia_trie_t *pt);

trienode_t* patiteratorget(patiterator_t *state);

void patiteratornext(patiterator_t *state);

int patiteratorend(const patiterator_t *state);

#endif

