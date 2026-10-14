#ifndef GRAPH_H
#define GRAPH_H

#include "common.h"

typedef struct EdgeNode {
    int to;
    double distance;
    int is_open;             /* 1 = open, 0 = disabled (no-fly zone) */
    struct EdgeNode *next;   /* adjacency list */
} EdgeNode;

typedef struct {
    NodeKind kind;
    EdgeNode *adj_head;      /* head of this node's adjacency list */
} GraphNode;

typedef struct {
    GraphNode nodes[MAX_NODES];
    int node_count;
} Graph;

/* Global Graph instance for simplicity */
extern Graph network_graph;

/* Initialize graph */
void initGraph(Graph *g);

/* Add a node and return its ID */
int addNode(Graph *g, NodeKind kind);

/* Add undirected edge between u and v */
void addEdge(Graph *g, int u, int v, double distance);

/* Toggle no-fly zone */
void setEdgeOpen(Graph *g, int u, int v, int open);

/* Free all edge memory */
void freeGraph(Graph *g);

/* No-Fly Zone Management */
typedef struct { 
    int u, v; 
} DisabledEdge;

extern DisabledEdge active_zones[MAX_ZONES];
extern int zone_count;

void initNoFlyZones();
void addNoFlyZone(Graph *g, int u, int v);
void removeNoFlyZone(Graph *g, int u, int v);
void printNoFlyZones();

#endif // GRAPH_H
