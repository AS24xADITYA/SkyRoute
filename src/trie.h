#ifndef TRIE_H
#define TRIE_H

#include "common.h"

/*
 * Trie (Prefix Tree) for customer name prefix-search.
 * Supports lowercase a-z (TRIE_ALPHA = 26).
 * Names are normalised to lowercase before insertion.
 */
typedef struct TrieNode {
    struct TrieNode *children[TRIE_ALPHA];
    int is_end;
    int customer_id;
} TrieNode;

extern TrieNode *customer_trie;

void trieInit(void);
void trieInsert(const char *name, int customer_id);
int  trieSearch(const char *prefix,
                int result_ids[],
                char result_names[][TRIE_MAX_WORD]);
void trieFreeAll(TrieNode *root);

#endif /* TRIE_H */
