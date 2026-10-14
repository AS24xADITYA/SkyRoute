#include "graph.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * ============================================================================
 * AIR CORRIDOR TOPOLOGY GRAPH — Adjacency List Representation
 *
 * Vertices (Nodes): Represent warehouses, charging hubs, or customer zones.
 * Edges (Corridors): Undirected links with flight distance in kilometers.
 * No-Fly Zones: Dynamic boolean flag (is_open) on each directed EdgeNode.
 * Allows instant O(1) blocking/restoring of air corridors during dispatch.
 * ============================================================================
 */

Graph network_graph;
DisabledEdge active_zones[MAX_ZONES];
int zone_count = 0;

void initGraph(Graph *g) {
    g->node_count = 0;
    for (int i = 0; i < MAX_NODES; i++) {
        g->nodes[i].adj_head = NULL;
    }
}

int addNode(Graph *g, NodeKind kind) {
    if (g->node_count >= MAX_NODES) {
        return -1;
    }
    int id = g->node_count++;
    g->nodes[id].kind = kind;
    g->nodes[id].adj_head = NULL;
    return id;
}

static void addDirectedEdge(Graph *g, int u, int v, double distance) {
    EdgeNode *edge = (EdgeNode *)malloc(sizeof(EdgeNode));
    if (!edge) {
        return;
    }
    edge->to = v;
    edge->distance = distance;
    edge->is_open = 1;
    edge->next = g->nodes[u].adj_head;
    g->nodes[u].adj_head = edge;
}

void addEdge(Graph *g, int u, int v, double distance) {
    addDirectedEdge(g, u, v, distance);
    addDirectedEdge(g, v, u, distance);
}

void setEdgeOpen(Graph *g, int u, int v, int open) {
    EdgeNode *curr = g->nodes[u].adj_head;
    while (curr) {
        if (curr->to == v) curr->is_open = open;
        curr = curr->next;
    }
    curr = g->nodes[v].adj_head;
    while (curr) {
        if (curr->to == u) curr->is_open = open;
        curr = curr->next;
    }
}

void freeGraph(Graph *g) {
    for (int i = 0; i < g->node_count; i++) {
        EdgeNode *curr = g->nodes[i].adj_head;
        while (curr) {
            EdgeNode *temp = curr;
            curr = curr->next;
            free(temp);
        }
        g->nodes[i].adj_head = NULL;
    }
    g->node_count = 0;
}

void initNoFlyZones(void) {
    zone_count = 0;
}

void addNoFlyZone(Graph *g, int u, int v) {
    if (zone_count >= MAX_ZONES) {
        return;
    }
    setEdgeOpen(g, u, v, 0);
    active_zones[zone_count].u = u;
    active_zones[zone_count].v = v;
    zone_count++;
}

void removeNoFlyZone(Graph *g, int u, int v) {
    for (int i = 0; i < zone_count; i++) {
        if ((active_zones[i].u == u && active_zones[i].v == v) ||
            (active_zones[i].u == v && active_zones[i].v == u)) {
            setEdgeOpen(g, u, v, 1);
            for (int j = i; j < zone_count - 1; j++) {
                active_zones[j] = active_zones[j + 1];
            }
            zone_count--;
            return;
        }
    }
}

void printNoFlyZones(void) {
    fprintf(stderr, "Active No-Fly Zones:\n");
    if (zone_count == 0) fprintf(stderr, "None.\n");
    for (int i = 0; i < zone_count; i++) {
        fprintf(stderr, "- Between Node %d and Node %d\n", active_zones[i].u, active_zones[i].v);
    }
}

