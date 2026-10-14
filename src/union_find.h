#ifndef UNION_FIND_H
#define UNION_FIND_H

#include "common.h"

/*
 * Union-Find (Disjoint Set Union) with path compression + union-by-rank.
 * Used to detect connected components in the air route network.
 */
typedef struct {
    int parent[MAX_NODES];
    int rnk[MAX_NODES];   /* rank array for union-by-rank */
    int n;
} UnionFind;

void ufInit(UnionFind *uf, int n);
int  ufFind(UnionFind *uf, int x);
void ufUnion(UnionFind *uf, int x, int y);
int  ufConnected(UnionFind *uf, int x, int y);
int  ufCountComponents(UnionFind *uf);

#endif /* UNION_FIND_H */
