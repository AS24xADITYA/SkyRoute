#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include "common.h"
#include "drone.h"
#include "station.h"
#include "graph.h"

/*
 * ============================================================================
 * SKYROUTE CORE ALGORITHMS SPECIFICATION
 * ============================================================================
 * Each algorithm is chosen for a specific mission-critical logistics requirement:
 *
 * 1.  Linear Search:        O(n)        Lookup station in small array
 * 2.  Binary Search:        O(log n)    Locate drone by ID (requires sorted array)
 * 3.  Heap Sort:            O(n log n)  In-place sort by battery (Ascending)
 * 4.  Merge Sort:           O(n log n)  Stable divide-and-conquer (Descending)
 * 5.  Quick Sort:           O(n log n)  Sort by Drone ID to prepare Binary Search
 * 6.  Breadth-First Search: O(V + E)    Shortest unweighted hop corridor path
 * 7.  Depth-First Search:   O(V + E)    Graph reachability & cluster discovery
 * 8.  Dijkstra's Algorithm: O(V^2)      Weighted shortest flight trajectory
 * 9.  Floyd-Warshall:       O(V^3)      All-pairs dynamic programming matrix
 * 10. Connected Components: O(V + E)    Partition check across active No-Fly zones
 * 11. Kahn's Topo Sort:     O(V + E)    DAG stage pipeline ordering & cycle check
 * ============================================================================
 */

/* 1. Linear Search for Charging Station by ID - O(n) */
int linearSearchStation(int id);

/* 2. Binary Search for Drone by ID - O(log n) */
int binarySearchDrone(Drone d[], int n, int target_id);

/* 3. Heap Sort: Sorts drones by battery ascending (lowest reserve first) - O(n log n) */
void heapSortDronesByBatteryAsc(Drone d[], int n);

/* 4. Merge Sort: Sorts drones by battery descending (readiest drone first) - O(n log n) */
void mergeSortDronesByBatteryDesc(Drone d[], int l, int r);

/* 5. Quick Sort: Sorts drones by ID ascending (enables Binary Search) - O(n log n) */
void quickSortDronesById(Drone d[], int low, int high);

/* 6. Breadth-First Search (BFS) for minimum-hop path - O(V + E) */
int bfs(Graph *g, int source, int target, int parent[]);

/* 7. Depth-First Search (DFS) for reachability - O(V + E) */
void dfs(Graph *g, int u, int visited[]);

/* 8. Dijkstra's Algorithm for minimum weighted distance path - O(V^2) */
void dijkstra(Graph *g, int source, double dist[], int parent[]);

/* 9. Floyd-Warshall All-Pairs Shortest Paths - O(V^3) */
void floydWarshall(Graph *g, double distMatrix[MAX_NODES][MAX_NODES], int nextHop[MAX_NODES][MAX_NODES]);

/* 10. DFS Connected Components analysis - O(V + E) */
int dfsConnectedComponents(Graph *g, int componentId[]);

/* 11. Kahn's Topological Sort for DAG delivery workflow validation - O(V + E) */
int topologicalSort(int n, int adj[MAX_NODES][MAX_NODES], int in_degree[MAX_NODES], int result[]);

#endif // ALGORITHMS_H

