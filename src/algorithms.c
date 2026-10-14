#include "algorithms.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * 1. LINEAR SEARCH — Charging Station Lookup
 * Time Complexity: O(n) | Space Complexity: O(1)
 * Used because the number of charging stations is small (<= 10).
 * ============================================================================ */
int linearSearchStation(int id) {
    for (int i = 0; i < stationCount; i++) {
        if (stations[i].station_id == id) return i;
    }
    return -1;
}

/* ============================================================================
 * 2. BINARY SEARCH — Drone Lookup by ID
 * Time Complexity: O(log n) | Space Complexity: O(1)
 * Requires the drone array to be sorted by drone_id (see quickSortDronesById).
 * ============================================================================ */
int binarySearchDrone(Drone d[], int n, int target_id) {
    int lo = 0, hi = n - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (d[mid].drone_id == target_id) return mid;
        else if (d[mid].drone_id < target_id) lo = mid + 1;
        else hi = mid - 1;
    }
    return -1;
}

/* ============================================================================
 * 3. HEAP SORT — Sort Drones by Battery Reserve (Ascending: Low to High)
 * Time Complexity: O(n log n) | Space Complexity: O(1) in-place
 * Useful for finding drones that critically need charging.
 * ============================================================================ */
static void heapifyDroneAsc(Drone d[], int n, int i) {
    int largest = i;
    int left = 2 * i + 1;
    int right = 2 * i + 2;

    if (left < n && d[left].battery_pct > d[largest].battery_pct)
        largest = left;
    if (right < n && d[right].battery_pct > d[largest].battery_pct)
        largest = right;

    if (largest != i) {
        Drone temp = d[i];
        d[i] = d[largest];
        d[largest] = temp;
        heapifyDroneAsc(d, n, largest);
    }
}

void heapSortDronesByBatteryAsc(Drone d[], int n) {
    /* Build max-heap */
    for (int i = n / 2 - 1; i >= 0; i--) {
        heapifyDroneAsc(d, n, i);
    }
    /* Extract elements one by one */
    for (int i = n - 1; i > 0; i--) {
        Drone temp = d[0];
        d[0] = d[i];
        d[i] = temp;
        heapifyDroneAsc(d, i, 0);
    }
}

/* ============================================================================
 * 4. MERGE SORT — Sort Drones by Battery Reserve (Descending: High to Low)
 * Time Complexity: O(n log n) | Space Complexity: O(n)
 * Stable divide-and-conquer algorithm to prioritize ready drones for dispatch.
 * ============================================================================ */
static void mergeBatteryDesc(Drone d[], int l, int m, int r) {
    int n1 = m - l + 1;
    int n2 = r - m;

    Drone *L = (Drone *)malloc(n1 * sizeof(Drone));
    Drone *R = (Drone *)malloc(n2 * sizeof(Drone));
    if (!L || !R) {
        if (L) free(L);
        if (R) free(R);
        return;
    }

    for (int i = 0; i < n1; i++) L[i] = d[l + i];
    for (int j = 0; j < n2; j++) R[j] = d[m + 1 + j];

    int i = 0, j = 0, k = l;
    while (i < n1 && j < n2) {
        /* Descending: larger battery first */
        if (L[i].battery_pct >= R[j].battery_pct) {
            d[k++] = L[i++];
        } else {
            d[k++] = R[j++];
        }
    }

    while (i < n1) d[k++] = L[i++];
    while (j < n2) d[k++] = R[j++];

    free(L);
    free(R);
}

void mergeSortDronesByBatteryDesc(Drone d[], int l, int r) {
    if (l >= r) return;
    int m = l + (r - l) / 2;
    mergeSortDronesByBatteryDesc(d, l, m);
    mergeSortDronesByBatteryDesc(d, m + 1, r);
    mergeBatteryDesc(d, l, m, r);
}

/* ============================================================================
 * 5. QUICK SORT — Sort Drones by Drone ID (Ascending)
 * Time Complexity: O(n log n) avg | Space Complexity: O(log n)
 * Sorts fleet sequentially so binarySearchDrone() works reliably.
 * ============================================================================ */
static int partitionById(Drone d[], int low, int high) {
    int pivot = d[high].drone_id;
    int i = low - 1;

    for (int j = low; j < high; j++) {
        if (d[j].drone_id <= pivot) {
            i++;
            Drone temp = d[i];
            d[i] = d[j];
            d[j] = temp;
        }
    }
    Drone temp = d[i + 1];
    d[i + 1] = d[high];
    d[high] = temp;
    return i + 1;
}

void quickSortDronesById(Drone d[], int low, int high) {
    if (low < high) {
        int pi = partitionById(d, low, high);
        quickSortDronesById(d, low, pi - 1);
        quickSortDronesById(d, pi + 1, high);
    }
}

/* ============================================================================
 * 6. BREADTH-FIRST SEARCH (BFS) — Minimum Hop Air Path
 * Time Complexity: O(V + E) | Space Complexity: O(V)
 * Uses a FIFO queue to find route with minimum flight handoffs.
 * ============================================================================ */
int bfs(Graph *g, int source, int target, int parent[]) {
    int visited[MAX_NODES] = {0};
    int queue[MAX_NODES], front = 0, rear = 0;

    for (int i = 0; i < g->node_count; i++) parent[i] = -1;

    if (source < 0 || source >= g->node_count || target < 0 || target >= g->node_count)
        return 0;

    visited[source] = 1;
    queue[rear++] = source;

    while (front < rear) {
        int u = queue[front++];
        if (u == target) return 1; /* Target reached */

        for (EdgeNode *e = g->nodes[u].adj_head; e; e = e->next) {
            if (e->is_open && !visited[e->to]) {
                visited[e->to] = 1;
                parent[e->to] = u;
                queue[rear++] = e->to;
            }
        }
    }
    return 0; /* Target unreachable */
}

/* ============================================================================
 * 7. DEPTH-FIRST SEARCH (DFS) — Reachability
 * Time Complexity: O(V + E) | Space Complexity: O(V) call stack
 * Recursively visits all open corridors in an airspace cluster.
 * ============================================================================ */
void dfs(Graph *g, int u, int visited[]) {
    if (u < 0 || u >= g->node_count) return;
    visited[u] = 1;
    for (EdgeNode *e = g->nodes[u].adj_head; e; e = e->next) {
        if (e->is_open && !visited[e->to]) {
            dfs(g, e->to, visited);
        }
    }
}

/* ============================================================================
 * 8. DIJKSTRA'S ALGORITHM — Shortest Weighted Flight Path
 * Time Complexity: O(V^2) | Space Complexity: O(V)
 * Finds lowest physical km route avoiding restricted No-Fly corridors.
 * ============================================================================ */
void dijkstra(Graph *g, int source, double dist[], int parent[]) {
    int visited[MAX_NODES] = {0};
    for (int i = 0; i < g->node_count; i++) {
        dist[i] = 1e9;
        parent[i] = -1;
    }

    if (source < 0 || source >= g->node_count) return;
    dist[source] = 0.0;

    for (int count = 0; count < g->node_count; count++) {
        int u = -1;
        double best = 1e9;
        for (int i = 0; i < g->node_count; i++) {
            if (!visited[i] && dist[i] < best) {
                best = dist[i];
                u = i;
            }
        }

        if (u == -1) break; /* All reachable nodes finalized */
        visited[u] = 1;

        for (EdgeNode *e = g->nodes[u].adj_head; e; e = e->next) {
            if (e->is_open && dist[u] + e->distance < dist[e->to]) {
                dist[e->to] = dist[u] + e->distance;
                parent[e->to] = u;
            }
        }
    }
}

/* ============================================================================
 * 9. FLOYD-WARSHALL ALGORITHM — All-Pairs Shortest Distances
 * Time Complexity: O(V^3) | Space Complexity: O(V^2)
 * Computes complete distance matrix via dynamic programming.
 * ============================================================================ */
void floydWarshall(Graph *g, double distMatrix[MAX_NODES][MAX_NODES], int nextHop[MAX_NODES][MAX_NODES]) {
    int n = g->node_count;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j) {
                distMatrix[i][j] = 0.0;
                nextHop[i][j] = j;
            } else {
                distMatrix[i][j] = 1e9;
                nextHop[i][j] = -1;
            }
        }
    }

    /* Initialize with direct corridor weights */
    for (int u = 0; u < n; u++) {
        for (EdgeNode *e = g->nodes[u].adj_head; e; e = e->next) {
            if (e->is_open) {
                int v = e->to;
                if (e->distance < distMatrix[u][v]) {
                    distMatrix[u][v] = e->distance;
                    nextHop[u][v] = v;
                }
            }
        }
    }

    /* Dynamic programming relaxation over intermediate node k */
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (distMatrix[i][k] < 1e8 && distMatrix[k][j] < 1e8) {
                    if (distMatrix[i][k] + distMatrix[k][j] < distMatrix[i][j]) {
                        distMatrix[i][j] = distMatrix[i][k] + distMatrix[k][j];
                        nextHop[i][j] = nextHop[i][k];
                    }
                }
            }
        }
    }
}

/* ============================================================================
 * 10. DFS CONNECTED COMPONENTS — Partition Analysis
 * Time Complexity: O(V + E) | Space Complexity: O(V)
 * Identifies disconnected subgraphs caused by closed No-Fly zones.
 * ============================================================================ */
static void dfsComp(Graph *g, int u, int compId, int componentId[], int visited[]) {
    visited[u] = 1;
    componentId[u] = compId;
    for (EdgeNode *e = g->nodes[u].adj_head; e; e = e->next) {
        if (e->is_open && !visited[e->to]) {
            dfsComp(g, e->to, compId, componentId, visited);
        }
    }
}

int dfsConnectedComponents(Graph *g, int componentId[]) {
    int visited[MAX_NODES] = {0};
    int compCount = 0;
    for (int i = 0; i < g->node_count; i++) componentId[i] = -1;

    for (int i = 0; i < g->node_count; i++) {
        if (!visited[i]) {
            dfsComp(g, i, compCount, componentId, visited);
            compCount++;
        }
    }
    return compCount;
}

/* ============================================================================
 * 11. KAHN'S ALGORITHM — Topological Sort for Delivery Pipeline
 * Time Complexity: O(V + E) | Space Complexity: O(V)
 * Linearizes workflow dependencies (e.g. payload check -> clearance -> flight).
 * Detects cycles to prevent delivery pipeline deadlocks.
 * ============================================================================ */
int topologicalSort(int n, int adj[MAX_NODES][MAX_NODES], int in_degree[MAX_NODES], int result[]) {
    int queue[MAX_NODES], front = 0, rear = 0;
    int cur_deg[MAX_NODES];
    for (int i = 0; i < n; i++) {
        cur_deg[i] = in_degree[i];
        if (cur_deg[i] == 0) queue[rear++] = i;
    }

    int count = 0;
    while (front < rear) {
        int u = queue[front++];
        result[count++] = u;

        for (int v = 0; v < n; v++) {
            if (adj[u][v]) {
                cur_deg[v]--;
                if (cur_deg[v] == 0) queue[rear++] = v;
            }
        }
    }
    return (count == n) ? count : -1;
}

