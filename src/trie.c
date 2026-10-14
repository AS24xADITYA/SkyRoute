#include "trie.h"
#include <ctype.h>

TrieNode *customer_trie = NULL;

void trieInit(void) { customer_trie = NULL; }

static TrieNode *newNode(void) {
    TrieNode *n = (TrieNode *)calloc(1, sizeof(TrieNode));
    if (!n) fprintf(stderr, "[TRIE] malloc failed\n");
    return n;
}

static int cidx(char c) {
    c = (char)tolower((unsigned char)c);
    return (c >= 'a' && c <= 'z') ? c - 'a' : -1;
}

void trieInsert(const char *name, int customer_id) {
    if (!customer_trie) customer_trie = newNode();
    TrieNode *cur = customer_trie;
    for (int i = 0; name[i]; i++) {
        int idx = cidx(name[i]);
        if (idx < 0) continue;
        if (!cur->children[idx]) cur->children[idx] = newNode();
        cur = cur->children[idx];
    }
    cur->is_end = 1;
    cur->customer_id = customer_id;
}

/* --- internal DFS collector --- */
static int  g_cnt;
static int  g_ids[MAX_TRIE_RESULTS];
static char g_names[MAX_TRIE_RESULTS][TRIE_MAX_WORD];

static void collect(TrieNode *node, char *word, int depth) {
    if (!node || g_cnt >= MAX_TRIE_RESULTS) return;
    if (node->is_end) {
        word[depth] = '\0';
        g_ids[g_cnt] = node->customer_id;
        strncpy(g_names[g_cnt], word, TRIE_MAX_WORD - 1);
        g_names[g_cnt][TRIE_MAX_WORD - 1] = '\0';
        g_cnt++;
    }
    for (int i = 0; i < TRIE_ALPHA; i++) {
        if (node->children[i]) {
            word[depth] = (char)('a' + i);
            collect(node->children[i], word, depth + 1);
        }
    }
}

int trieSearch(const char *prefix,
               int result_ids[],
               char result_names[][TRIE_MAX_WORD]) {
    g_cnt = 0;
    if (!customer_trie) return 0;
    TrieNode *cur = customer_trie;
    char word[TRIE_MAX_WORD];
    int depth = 0;
    for (int i = 0; prefix[i]; i++) {
        int idx = cidx(prefix[i]);
        if (idx < 0) continue;
        if (!cur->children[idx]) return 0;
        word[depth++] = (char)('a' + idx);
        cur = cur->children[idx];
    }
    collect(cur, word, depth);
    for (int i = 0; i < g_cnt; i++) {
        result_ids[i] = g_ids[i];
        strncpy(result_names[i], g_names[i], TRIE_MAX_WORD - 1);
        result_names[i][TRIE_MAX_WORD - 1] = '\0';
    }
    return g_cnt;
}

void trieFreeAll(TrieNode *root) {
    if (!root) return;
    for (int i = 0; i < TRIE_ALPHA; i++) trieFreeAll(root->children[i]);
    free(root);
}
