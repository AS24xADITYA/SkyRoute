#include "union_find.h"

void ufInit(UnionFind *uf, int n) {
    uf->n = n;
    for (int i = 0; i < n; i++) { uf->parent[i] = i; uf->rnk[i] = 0; }
}

int ufFind(UnionFind *uf, int x) {
    if (uf->parent[x] != x)
        uf->parent[x] = ufFind(uf, uf->parent[x]);
    return uf->parent[x];
}

void ufUnion(UnionFind *uf, int x, int y) {
    int rx = ufFind(uf, x), ry = ufFind(uf, y);
    if (rx == ry) return;
    if (uf->rnk[rx] < uf->rnk[ry]) { int t = rx; rx = ry; ry = t; }
    uf->parent[ry] = rx;
    if (uf->rnk[rx] == uf->rnk[ry]) uf->rnk[rx]++;
}

int ufConnected(UnionFind *uf, int x, int y) {
    return ufFind(uf, x) == ufFind(uf, y);
}

int ufCountComponents(UnionFind *uf) {
    int c = 0;
    for (int i = 0; i < uf->n; i++)
        if (ufFind(uf, i) == i) c++;
    return c;
}
