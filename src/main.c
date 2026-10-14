#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "common.h"
#include "drone.h"
#include "drone_hash.h"
#include "request.h"
#include "station.h"
#include "graph.h"
#include "customer.h"
#include "history.h"
#include "algorithms.h"
#include "heap.h"
#include "union_find.h"
#include "trie.h"

/*
 * ============================================================================
 * SKYROUTE — Autonomous Drone Delivery & Algorithmic Dispatch Platform
 *
 * Core Server Daemon Orchestrator & CLI Interface.
 * Implements persistent line-buffered JSON protocol communicating over stdio.
 * ============================================================================
 */

DroneManager dm;

void initSystem(void) {
    initDroneManager(&dm);
    initHashTable();
    initRequests();
    initStations();
    initGraph(&network_graph);
    initNoFlyZones();
    initCustomerDirectory();
    initUndoStack();
    initHeap(&priority_heap);
    trieInit();
}

void teardownSystem(void) {
    freeHashTable();
    freeOrderList(&active_orders);
    freeGraph(&network_graph);
    freeCustomerBST(customer_root);
    trieFreeAll(customer_trie);
    customer_trie = NULL;
}

static void ensureGraphNodes(int count) {
    while (network_graph.node_count < count && network_graph.node_count < MAX_NODES) {
        addNode(&network_graph, WAREHOUSE);
    }
}

/*
 * Pre-seeds a rich, realistic delivery network with:
 * - 14 Nodes (Warehouses, Charging Bays, Customer Zones)
 * - 17 Air Corridors with accurate flight distances
 * - 3 Solar Charging Stations with FIFO waiting queues
 * - 8 Registered Customers indexed in both BST and Trie
 * - 6 Autonomous Drones with various battery & payload profiles
 * - 5 Initial Delivery Requests partitioned across Emergency & Standard queues
 */
void initDemoData(void) {
    teardownSystem();
    initSystem();

    /* 1. Add 14 Airspace Nodes */
    addNode(&network_graph, WAREHOUSE); /* 0: Central Sky Hub */
    addNode(&network_graph, WAREHOUSE); /* 1: North Air Depot */
    addNode(&network_graph, WAREHOUSE); /* 2: West Logistics Hub */
    addNode(&network_graph, STATION);   /* 3: Solar Station Alpha */
    addNode(&network_graph, STATION);   /* 4: Substation Beta */
    addNode(&network_graph, STATION);   /* 5: Rooftop Charger */
    addNode(&network_graph, CUSTOMER);  /* 6: Metro General Hospital */
    addNode(&network_graph, CUSTOMER);  /* 7: Silicon Tech Park */
    addNode(&network_graph, CUSTOMER);  /* 8: Downtown Plaza */
    addNode(&network_graph, CUSTOMER);  /* 9: Skyline Tower */
    addNode(&network_graph, CUSTOMER);  /* 10: Harbor Bay Point */
    addNode(&network_graph, CUSTOMER);  /* 11: Green Hills Suburb */
    addNode(&network_graph, CUSTOMER);  /* 12: Apex University */
    addNode(&network_graph, CUSTOMER);  /* 13: Airport Zone */

    /* 2. Add 17 Flight Corridors (Undirected Edges) */
    addEdge(&network_graph, 0, 1, 4.2);
    addEdge(&network_graph, 0, 2, 5.5);
    addEdge(&network_graph, 0, 3, 3.0);
    addEdge(&network_graph, 1, 3, 3.8);
    addEdge(&network_graph, 1, 6, 2.5);
    addEdge(&network_graph, 2, 4, 3.2);
    addEdge(&network_graph, 2, 7, 4.0);
    addEdge(&network_graph, 3, 8, 3.5);
    addEdge(&network_graph, 4, 9, 4.8);
    addEdge(&network_graph, 5, 10, 5.0);
    addEdge(&network_graph, 6, 8, 3.1);
    addEdge(&network_graph, 7, 11, 4.4);
    addEdge(&network_graph, 8, 9, 2.8);
    addEdge(&network_graph, 9, 12, 3.6);
    addEdge(&network_graph, 10, 13, 4.1);
    addEdge(&network_graph, 11, 12, 3.0);
    addEdge(&network_graph, 3, 5, 4.5);

    /* 3. Add 3 Solar Charging Stations */
    addStation(101, 3, 3);
    addStation(102, 4, 2);
    addStation(103, 5, 2);

    /* 4. Add Customers into BST (Key: ID) and Trie (Prefix: Name) */
    struct { int id; const char *name; int node; } demoCusts[] = {
        {1, "Alice Smith", 6},
        {2, "Bob Jones", 7},
        {3, "Charlie Brown", 8},
        {4, "Diana Prince", 9},
        {5, "Evan Wright", 10},
        {6, "Fiona Gallagher", 11},
        {7, "George Clark", 12},
        {8, "Hannah Abbott", 13}
    };
    for (int i = 0; i < 8; i++) {
        customer_root = insertCustomer(customer_root, demoCusts[i].id, demoCusts[i].name, demoCusts[i].node);
        trieInsert(demoCusts[i].name, demoCusts[i].id);
    }

    /* 5. Add Fleet Drones (addDrone automatically indexes each in hash table) */
    Drone d1 = {101, 95, 0, IDLE, 5.0f};
    Drone d2 = {102, 80, 0, IDLE, 4.0f};
    Drone d3 = {103, 65, 1, IDLE, 6.0f};
    Drone d4 = {104, 45, 2, IDLE, 4.5f};
    Drone d5 = {105, 18, 3, IDLE, 5.0f}; /* Low battery reserve */
    Drone d6 = {106, 90, 2, IDLE, 8.0f};

    addDrone(&dm, d1);
    addDrone(&dm, d2);
    addDrone(&dm, d3);
    addDrone(&dm, d4);
    addDrone(&dm, d5);
    addDrone(&dm, d6);

    /* 6. Add Initial Delivery Requests */
    DeliveryRequest r1 = {501, 6, 2.0f, EMERGENCY, PENDING}; /* Hospital (Critical) */
    DeliveryRequest r2 = {502, 8, 1.5f, NORMAL, PENDING};    /* Plaza */
    DeliveryRequest r3 = {503, 9, 3.2f, EMERGENCY, PENDING}; /* Skyline (Critical) */
    DeliveryRequest r4 = {504, 7, 4.0f, NORMAL, PENDING};    /* Tech Park */
    DeliveryRequest r5 = {505, 12, 2.5f, NORMAL, PENDING};   /* University */

    addRequest(r1);
    addRequest(r2);
    addRequest(r3);
    addRequest(r4);
    addRequest(r5);
}

/*
 * Serializes the complete platform state into a single-line JSON payload.
 * Emitted to stdout for frontend state hydration.
 */
static void printStateJSON(void) {
    printf("{\"status\":\"ok\",\"drones\":[");
    for (int i = 0; i < dm.count; i++) {
        if (i) printf(",");
        printf("{\"id\":%d,\"battery\":%d,\"location\":%d,\"status\":%d,\"max_carry\":%.1f}",
               dm.drones[i].drone_id, dm.drones[i].battery_pct, dm.drones[i].location_id,
               dm.drones[i].status, dm.drones[i].max_carry_weight);
    }
    printf("],\"customers\":[");
    int c_ids[MAX_CUSTOMERS], c_nodes[MAX_CUSTOMERS];
    char c_names[MAX_CUSTOMERS][50];
    int c_cnt = collectCustomers(customer_root, c_ids, c_names, c_nodes, MAX_CUSTOMERS);
    for (int i = 0; i < c_cnt; i++) {
        if (i) printf(",");
        printf("{\"id\":%d,\"name\":\"%s\",\"node_id\":%d}", c_ids[i], c_names[i], c_nodes[i]);
    }
    printf("],\"stations\":[");
    for (int i = 0; i < stationCount; i++) {
        if (i) printf(",");
        int occupied = stations[i].total_slots - stations[i].free_slots;
        printf("{\"id\":%d,\"node_id\":%d,\"total_slots\":%d,\"free_slots\":%d,\"occupied\":%d,\"waiting\":[",
               stations[i].station_id, stations[i].node_id, stations[i].total_slots,
               stations[i].free_slots, occupied);
        for (int k = 0; k < stations[i].wq_count; k++) {
            int idx = (stations[i].wq_front + k) % MAX_WAITING;
            if (k) printf(",");
            printf("%d", stations[i].waiting_queue[idx]);
        }
        printf("]}");
    }
    printf("],\"nodes\":[");
    for (int i = 0; i < network_graph.node_count; i++) {
        if (i) printf(",");
        printf("{\"id\":%d,\"kind\":%d}", i, network_graph.nodes[i].kind);
    }
    printf("],\"edges\":[");
    int edgePrinted = 0;
    for (int u = 0; u < network_graph.node_count; u++) {
        for (EdgeNode *e = network_graph.nodes[u].adj_head; e; e = e->next) {
            if (u < e->to) {
                if (edgePrinted++) printf(",");
                printf("{\"from\":%d,\"to\":%d,\"distance\":%.1f,\"is_open\":%d}",
                       u, e->to, e->distance, e->is_open);
            }
        }
    }
    printf("],\"orders\":[");
    OrderNode *on = active_orders.head;
    int oprinted = 0;
    while (on) {
        if (oprinted++) printf(",");
        printf("{\"id\":%d,\"customer_node_id\":%d,\"weight\":%.1f,\"priority\":%d,\"status\":%d}",
               on->data.order_id, on->data.customer_node_id, on->data.package_weight,
               on->data.priority, on->data.status);
        on = on->next;
    }
    printf("],\"queues\":{\"emergency\":[");
    for (int i = 0; i < emergencyQueue.count; i++) {
        if (i) printf(",");
        int idx = (emergencyQueue.front + i) % MAX_QUEUE;
        printf("%d", emergencyQueue.items[idx]);
    }
    printf("],\"normal\":[");
    for (int i = 0; i < normalQueue.count; i++) {
        if (i) printf(",");
        int idx = (normalQueue.front + i) % MAX_QUEUE;
        printf("%d", normalQueue.items[idx]);
    }
    printf("],\"heap\":");
    heapPrintJSON(&priority_heap);
    printf("},\"history_count\":%d,\"no_fly_count\":%d}\n", undo_stack.top + 1, zone_count);
    fflush(stdout);
}

/*
 * Line-buffered API command processor.
 * Each input line corresponds to exactly ONE JSON response line emitted to stdout.
 */
void apiLoop(void) {
    setvbuf(stdout, NULL, _IONBF, 0); /* Direct flush without OS buffering */
    char line[512];

    while (fgets(line, sizeof(line), stdin)) {
        /* Strip trailing whitespace and newlines */
        int len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r' || line[len - 1] == ' ')) {
            line[--len] = '\0';
        }
        if (len == 0) continue;

        char cmd[64] = {0};
        if (sscanf(line, "%63s", cmd) != 1) continue;

        if (strcmp(cmd, "INIT_DEMO") == 0) {
            initDemoData();
            printf("{\"status\":\"success\",\"message\":\"Demo environment loaded with 14 nodes, 17 corridors, 6 drones, 8 customers, 5 orders\"}\n");
        } else if (strcmp(cmd, "CLEAR_ALL") == 0) {
            teardownSystem();
            initSystem();
            printf("{\"status\":\"success\",\"message\":\"All platform state cleared\"}\n");
        } else if (strcmp(cmd, "GET_STATE") == 0) {
            printStateJSON();
        } else if (strcmp(cmd, "ADD_DRONE") == 0) {
            int id, battery, location;
            float weight;
            if (sscanf(line, "%*s %d %d %d %f", &id, &battery, &location, &weight) == 4) {
                Drone d = {id, battery, location, IDLE, weight};
                int idx = addDrone(&dm, d);
                if (idx != -1) {
                    printf("{\"status\":\"success\",\"message\":\"Drone %d registered\",\"index\":%d}\n", id, idx);
                } else {
                    printf("{\"status\":\"error\",\"message\":\"Failed to add drone: fleet full or duplicate ID %d\"}\n", id);
                }
            } else {
                printf("{\"status\":\"error\",\"message\":\"Invalid ADD_DRONE arguments. Expected: id battery location weight\"}\n");
            }
        } else if (strcmp(cmd, "REMOVE_DRONE") == 0) {
            int id;
            if (sscanf(line, "%*s %d", &id) == 1) {
                if (removeDrone(&dm, id)) {
                    printf("{\"status\":\"success\",\"message\":\"Drone %d decommissioned from fleet\"}\n", id);
                } else {
                    printf("{\"status\":\"error\",\"message\":\"Cannot decommission drone %d: not found or currently active (not IDLE)\"}\n", id);
                }
            } else {
                printf("{\"status\":\"error\",\"message\":\"Invalid REMOVE_DRONE arguments. Expected: id\"}\n");
            }
        } else if (strcmp(cmd, "SORT_DRONES") == 0) {
            char algo[32] = "heap";
            sscanf(line, "%*s %31s", algo);

            if (strcmp(algo, "heap") == 0 || strcmp(algo, "battery_asc") == 0) {
                heapSortDronesByBatteryAsc(dm.drones, dm.count);
                rehashAllDrones(&dm);
                printf("{\"status\":\"success\",\"algorithm\":\"heap_asc\",\"message\":\"Drones sorted by battery (Ascending: lowest first via Heap Sort O(n log n))\"}\n");
            } else if (strcmp(algo, "merge") == 0 || strcmp(algo, "battery_desc") == 0) {
                mergeSortDronesByBatteryDesc(dm.drones, 0, dm.count - 1);
                rehashAllDrones(&dm);
                printf("{\"status\":\"success\",\"algorithm\":\"merge_desc\",\"message\":\"Drones sorted by battery (Descending: readiest first via Merge Sort O(n log n))\"}\n");
            } else if (strcmp(algo, "id") == 0 || strcmp(algo, "id_asc") == 0) {
                quickSortDronesById(dm.drones, 0, dm.count - 1);
                rehashAllDrones(&dm);
                printf("{\"status\":\"success\",\"algorithm\":\"id_asc\",\"message\":\"Drones sorted by ID (Ascending via Quick Sort O(n log n) - enables Binary Search)\"}\n");
            } else {
                heapSortDronesByBatteryAsc(dm.drones, dm.count);
                rehashAllDrones(&dm);
                printf("{\"status\":\"success\",\"algorithm\":\"heap_asc\",\"message\":\"Drones sorted by battery reserve (Heap Sort)\"}\n");
            }
        } else if (strcmp(cmd, "SEARCH_DRONE") == 0) {
            int id;
            if (sscanf(line, "%*s %d", &id) == 1) {
                int idx = hashFind(id);
                if (idx != -1 && idx < dm.count && dm.drones[idx].drone_id == id) {
                    printf("{\"status\":\"success\",\"found\":1,\"method\":\"O(1) Hash Table Lookup\",\"drone\":{\"id\":%d,\"battery\":%d,\"location\":%d,\"status\":%d,\"max_carry\":%.1f}}\n",
                           dm.drones[idx].drone_id, dm.drones[idx].battery_pct, dm.drones[idx].location_id,
                           dm.drones[idx].status, dm.drones[idx].max_carry_weight);
                } else {
                    int bidx = binarySearchDrone(dm.drones, dm.count, id);
                    if (bidx != -1) {
                        printf("{\"status\":\"success\",\"found\":1,\"method\":\"O(log n) Binary Search\",\"drone\":{\"id\":%d,\"battery\":%d,\"location\":%d,\"status\":%d,\"max_carry\":%.1f}}\n",
                               dm.drones[bidx].drone_id, dm.drones[bidx].battery_pct, dm.drones[bidx].location_id,
                               dm.drones[bidx].status, dm.drones[bidx].max_carry_weight);
                    } else {
                        printf("{\"status\":\"success\",\"found\":0,\"message\":\"Drone %d not found in fleet\"}\n", id);
                    }
                }
            } else {
                printf("{\"status\":\"error\",\"message\":\"Invalid SEARCH_DRONE arguments. Expected: id\"}\n");
            }
        } else if (strcmp(cmd, "ADD_CUSTOMER") == 0) {
            int id, node_id;
            char name[64];
            if (sscanf(line, "%*s %d %63s %d", &id, name, &node_id) == 3) {
                customer_root = insertCustomer(customer_root, id, name, node_id);
                trieInsert(name, id);
                printf("{\"status\":\"success\",\"message\":\"Customer '%s' registered into BST and Trie\"}\n", name);
            } else {
                printf("{\"status\":\"error\",\"message\":\"Invalid ADD_CUSTOMER arguments. Expected: id name node_id\"}\n");
            }
        } else if (strcmp(cmd, "SEARCH_CUSTOMER_BST") == 0) {
            int id;
            if (sscanf(line, "%*s %d", &id) == 1) {
                CustomerNode *cur = customer_root;
                printf("{\"status\":\"success\",\"steps\":[");
                int stepCount = 0;
                CustomerNode *found = NULL;
                while (cur) {
                    if (stepCount++) printf(",");
                    printf("%d", cur->customer_id);
                    if (cur->customer_id == id) {
                        found = cur;
                        break;
                    } else if (id < cur->customer_id) {
                        cur = cur->left;
                    } else {
                        cur = cur->right;
                    }
                }
                printf("],\"found\":%d", found != NULL ? 1 : 0);
                if (found) {
                    printf(",\"customer\":{\"id\":%d,\"name\":\"%s\",\"node_id\":%d}",
                           found->customer_id, found->name, found->node_id);
                }
                printf("}\n");
            } else {
                printf("{\"status\":\"error\",\"message\":\"Invalid SEARCH_CUSTOMER_BST arguments\"}\n");
            }
        } else if (strcmp(cmd, "SEARCH_CUSTOMER_TRIE") == 0) {
            char prefix[64] = {0};
            if (sscanf(line, "%*s %63s", prefix) == 1) {
                int r_ids[MAX_TRIE_RESULTS];
                char r_names[MAX_TRIE_RESULTS][TRIE_MAX_WORD];
                int cnt = trieSearch(prefix, r_ids, r_names);
                printf("{\"status\":\"success\",\"prefix\":\"%s\",\"count\":%d,\"matches\":[", prefix, cnt);
                for (int i = 0; i < cnt; i++) {
                    if (i) printf(",");
                    printf("{\"id\":%d,\"name\":\"%s\"}", r_ids[i], r_names[i]);
                }
                printf("]}\n");
            } else {
                printf("{\"status\":\"error\",\"message\":\"Invalid SEARCH_CUSTOMER_TRIE arguments\"}\n");
            }
        } else if (strcmp(cmd, "ADD_STATION") == 0) {
            int sid, nid, slots;
            if (sscanf(line, "%*s %d %d %d", &sid, &nid, &slots) == 3) {
                if (addStation(sid, nid, slots) != -1) {
                    printf("{\"status\":\"success\",\"message\":\"Charging station %d deployed\"}\n", sid);
                } else {
                    printf("{\"status\":\"error\",\"message\":\"Failed to add station: limit reached or duplicate ID\"}\n");
                }
            } else {
                printf("{\"status\":\"error\",\"message\":\"Invalid ADD_STATION arguments\"}\n");
            }
        } else if (strcmp(cmd, "SEND_TO_CHARGE") == 0) {
            int did, sid;
            if (sscanf(line, "%*s %d %d", &did, &sid) == 2) {
                ChargingStation *cs = findStation(sid);
                int didx = searchDroneIndex(&dm, did);
                if (cs && didx != -1) {
                    dm.drones[didx].location_id = cs->node_id;
                    dm.drones[didx].status = CHARGING;
                    assignDrone(cs, did);
                    printf("{\"status\":\"success\",\"message\":\"Drone %d docked/queued at station %d\"}\n", did, sid);
                } else {
                    printf("{\"status\":\"error\",\"message\":\"Station or drone not found\"}\n");
                }
            } else {
                printf("{\"status\":\"error\",\"message\":\"Invalid SEND_TO_CHARGE arguments\"}\n");
            }
        } else if (strcmp(cmd, "FREE_SLOT") == 0) {
            int sid;
            if (sscanf(line, "%*s %d", &sid) == 1) {
                ChargingStation *cs = findStation(sid);
                if (cs) {
                    int pulled_drone = freeSlot(cs);
                    if (pulled_drone != -1) {
                        int didx = searchDroneIndex(&dm, pulled_drone);
                        if (didx != -1) {
                            dm.drones[didx].battery_pct = 100;
                            dm.drones[didx].status = CHARGING;
                        }
                    }
                    printf("{\"status\":\"success\",\"freed_drone\":%d,\"free_slots\":%d}\n", pulled_drone, cs->free_slots);
                } else {
                    printf("{\"status\":\"error\",\"message\":\"Station not found\"}\n");
                }
            } else {
                printf("{\"status\":\"error\",\"message\":\"Invalid FREE_SLOT arguments\"}\n");
            }
        } else if (strcmp(cmd, "ADD_NODE") == 0) {
            int kind;
            if (sscanf(line, "%*s %d", &kind) == 1) {
                int nid = addNode(&network_graph, (NodeKind)kind);
                printf("{\"status\":\"success\",\"node_id\":%d}\n", nid);
            } else {
                printf("{\"status\":\"error\",\"message\":\"Invalid ADD_NODE arguments\"}\n");
            }
        } else if (strcmp(cmd, "ADD_EDGE") == 0) {
            int u, v; double dist;
            if (sscanf(line, "%*s %d %d %lf", &u, &v, &dist) == 3) {
                ensureGraphNodes((u > v ? u : v) + 1);
                addEdge(&network_graph, u, v, dist);
                printf("{\"status\":\"success\",\"message\":\"Air corridor (%d,%d) established\"}\n", u, v);
            } else {
                printf("{\"status\":\"error\",\"message\":\"Invalid ADD_EDGE arguments\"}\n");
            }
        } else if (strcmp(cmd, "SET_NOFLY") == 0) {
            int u, v, open;
            if (sscanf(line, "%*s %d %d %d", &u, &v, &open) == 3) {
                if (open == 0) addNoFlyZone(&network_graph, u, v);
                else removeNoFlyZone(&network_graph, u, v);
                printf("{\"status\":\"success\",\"u\":%d,\"v\":%d,\"is_open\":%d}\n", u, v, open);
            } else {
                printf("{\"status\":\"error\",\"message\":\"Invalid SET_NOFLY arguments\"}\n");
            }
        } else if (strcmp(cmd, "ADD_ORDER") == 0) {
            int oid, cid, p; float weight;
            if (sscanf(line, "%*s %d %d %f %d", &oid, &cid, &weight, &p) == 4) {
                CustomerNode *cust = searchCustomer(customer_root, cid);
                if (!cust) {
                    printf("{\"status\":\"error\",\"message\":\"Customer ID %d not registered\"}\n", cid);
                } else {
                    DeliveryRequest r;
                    r.order_id = oid;
                    r.customer_node_id = cust->node_id;
                    r.package_weight = weight;
                    r.priority = (p == 1) ? EMERGENCY : NORMAL;
                    r.status = PENDING;
                    addRequest(r);
                    printf("{\"status\":\"success\",\"message\":\"Order %d queued (priority: %d)\"}\n", oid, p);
                }
            } else {
                printf("{\"status\":\"error\",\"message\":\"Invalid ADD_ORDER arguments\"}\n");
            }
        } else if (strcmp(cmd, "ASSIGN_ORDER") == 0) {
            int oid = getNextOrder();
            if (oid == -1) {
                printf("{\"status\":\"error\",\"message\":\"No pending delivery requests in queue\"}\n");
                continue;
            }
            DeliveryRequest *r = getOrder(&active_orders, oid);
            if (!r) {
                printf("{\"status\":\"error\",\"message\":\"Order %d not found\"}\n", oid);
                continue;
            }

            /* Find optimal drone via Dijkstra shortest path */
            int best_drone_idx = -1;
            double min_dist = 1e9;
            int best_path[MAX_NODES];
            int best_path_len = 0;

            for (int i = 0; i < dm.count; i++) {
                if (dm.drones[i].status == IDLE &&
                    dm.drones[i].battery_pct >= MIN_FLIGHT_THRESHOLD &&
                    dm.drones[i].max_carry_weight >= r->package_weight) {

                    int src = dm.drones[i].location_id;
                    int dst = r->customer_node_id;
                    double dist[MAX_NODES];
                    int parent[MAX_NODES];
                    dijkstra(&network_graph, src, dist, parent);

                    if (dist[dst] < min_dist) {
                        min_dist = dist[dst];
                        best_drone_idx = i;

                        /* Trace path backward from dst to src */
                        int cur = dst;
                        int temp_path[MAX_NODES];
                        int tlen = 0;
                        while (cur != -1) {
                            temp_path[tlen++] = cur;
                            cur = parent[cur];
                        }
                        best_path_len = 0;
                        for (int k = tlen - 1; k >= 0; k--) {
                            best_path[best_path_len++] = temp_path[k];
                        }
                    }
                }
            }

            if (best_drone_idx == -1 || min_dist >= 1e8) {
                /* Return order to queue if no path or no drone available */
                if (r->priority == EMERGENCY) enqueue(&emergencyQueue, oid);
                else enqueue(&normalQueue, oid);
                printf("{\"status\":\"error\",\"message\":\"No available drone with sufficient battery or route blocked by No-Fly zones\"}\n");
            } else {
                Drone *d = &dm.drones[best_drone_idx];
                AssignmentRecord rec = {oid, d->drone_id, IDLE, PENDING};
                pushAssignment(rec);

                r->status = IN_TRANSIT;
                d->status = DELIVERING;

                printf("{\"status\":\"success\",\"order_id\":%d,\"drone_id\":%d,\"distance\":%.1f,\"path\":[",
                       oid, d->drone_id, min_dist);
                for (int p = 0; p < best_path_len; p++) {
                    if (p) printf(",");
                    printf("%d", best_path[p]);
                }
                printf("],\"target_node\":%d,\"battery\":%d}\n", r->customer_node_id, d->battery_pct);
            }
        } else if (strcmp(cmd, "COMPLETE_DELIVERY") == 0) {
            int oid, did, final_node;
            if (sscanf(line, "%*s %d %d %d", &oid, &did, &final_node) == 3) {
                completeRequest(oid);
                int didx = searchDroneIndex(&dm, did);
                if (didx != -1) {
                    dm.drones[didx].status = IDLE;
                    dm.drones[didx].location_id = final_node;
                    int drain = 10;
                    dm.drones[didx].battery_pct = (dm.drones[didx].battery_pct > drain) ? (dm.drones[didx].battery_pct - drain) : 5;
                    printf("{\"status\":\"success\",\"order_id\":%d,\"drone_id\":%d,\"new_location\":%d,\"battery\":%d}\n",
                           oid, did, final_node, dm.drones[didx].battery_pct);
                } else {
                    printf("{\"status\":\"error\",\"message\":\"Drone %d not found\"}\n", did);
                }
            } else {
                printf("{\"status\":\"error\",\"message\":\"Invalid COMPLETE_DELIVERY arguments\"}\n");
            }
        } else if (strcmp(cmd, "UNDO") == 0) {
            if (undo_stack.top < 0) {
                printf("{\"status\":\"error\",\"message\":\"Undo stack is currently empty\"}\n");
            } else {
                AssignmentRecord top = undo_stack.records[undo_stack.top];
                int ok = undoLast(&dm);
                if (ok) {
                    DeliveryRequest *r = getOrder(&active_orders, top.order_id);
                    if (r) {
                        r->status = PENDING;
                        if (r->priority == EMERGENCY) enqueue(&emergencyQueue, top.order_id);
                        else enqueue(&normalQueue, top.order_id);
                    }
                    printf("{\"status\":\"success\",\"message\":\"Assignment undone for Order %d and Drone %d\"}\n",
                           top.order_id, top.drone_id);
                } else {
                    printf("{\"status\":\"error\",\"message\":\"Failed to undo dispatch assignment\"}\n");
                }
            }
        } else if (strcmp(cmd, "RUN_BFS") == 0) {
            int src, dst;
            if (sscanf(line, "%*s %d %d", &src, &dst) == 2) {
                int parent[MAX_NODES];
                int ok = bfs(&network_graph, src, dst, parent);
                printf("{\"status\":\"success\",\"found\":%d", ok);
                if (ok) {
                    int cur = dst;
                    int temp[MAX_NODES], tlen = 0;
                    while (cur != -1) {
                        temp[tlen++] = cur;
                        cur = parent[cur];
                    }
                    printf(",\"hops\":%d,\"path\":[", tlen - 1);
                    for (int k = tlen - 1; k >= 0; k--) {
                        if (k != tlen - 1) printf(",");
                        printf("%d", temp[k]);
                    }
                    printf("]");
                }
                printf("}\n");
            } else {
                printf("{\"status\":\"error\",\"message\":\"Invalid RUN_BFS arguments\"}\n");
            }
        } else if (strcmp(cmd, "RUN_DIJKSTRA") == 0) {
            int src, dst;
            if (sscanf(line, "%*s %d %d", &src, &dst) == 2) {
                double dist[MAX_NODES];
                int parent[MAX_NODES];
                dijkstra(&network_graph, src, dist, parent);
                int ok = (dst >= 0 && dst < network_graph.node_count && dist[dst] < 1e8);
                printf("{\"status\":\"success\",\"found\":%d", ok ? 1 : 0);
                if (ok) {
                    int cur = dst;
                    int temp[MAX_NODES], tlen = 0;
                    while (cur != -1) {
                        temp[tlen++] = cur;
                        cur = parent[cur];
                    }
                    printf(",\"distance\":%.2f,\"path\":[", dist[dst]);
                    for (int k = tlen - 1; k >= 0; k--) {
                        if (k != tlen - 1) printf(",");
                        printf("%d", temp[k]);
                    }
                    printf("]");
                }
                printf("}\n");
            } else {
                printf("{\"status\":\"error\",\"message\":\"Invalid RUN_DIJKSTRA arguments\"}\n");
            }
        } else if (strcmp(cmd, "RUN_FLOYD_WARSHALL") == 0) {
            double matrix[MAX_NODES][MAX_NODES];
            int nextHop[MAX_NODES][MAX_NODES];
            floydWarshall(&network_graph, matrix, nextHop);
            int n = network_graph.node_count;
            printf("{\"status\":\"success\",\"node_count\":%d,\"matrix\":[", n);
            for (int i = 0; i < n; i++) {
                if (i) printf(",");
                printf("[");
                for (int j = 0; j < n; j++) {
                    if (j) printf(",");
                    if (matrix[i][j] > 1e8) printf("-1");
                    else printf("%.1f", matrix[i][j]);
                }
                printf("]");
            }
            printf("]}\n");
        } else if (strcmp(cmd, "RUN_COMPONENTS") == 0) {
            int compId[MAX_NODES];
            int cCount = dfsConnectedComponents(&network_graph, compId);

            UnionFind uf;
            ufInit(&uf, network_graph.node_count);
            for (int u = 0; u < network_graph.node_count; u++) {
                for (EdgeNode *e = network_graph.nodes[u].adj_head; e; e = e->next) {
                    if (e->is_open && u < e->to) {
                        ufUnion(&uf, u, e->to);
                    }
                }
            }
            int ufCount = ufCountComponents(&uf);

            printf("{\"status\":\"success\",\"dfs_count\":%d,\"uf_count\":%d,\"components\":[", cCount, ufCount);
            for (int i = 0; i < network_graph.node_count; i++) {
                if (i) printf(",");
                printf("{\"node\":%d,\"comp\":%d}", i, compId[i]);
            }
            printf("]}\n");
        } else if (strcmp(cmd, "RUN_TOPO_SORT") == 0) {
            int n = 5;
            int adj[MAX_NODES][MAX_NODES] = {0};
            int in_deg[MAX_NODES] = {0};

            /* Realistic delivery DAG: Inspection -> Preflight -> Trajectory -> Corridor Clearance -> Dropoff */
            adj[0][1] = 1; in_deg[1]++;
            adj[0][2] = 1; in_deg[2]++;
            adj[1][3] = 1; in_deg[3]++;
            adj[2][3] = 1; in_deg[3]++;
            adj[3][4] = 1; in_deg[4]++;

            int order[MAX_NODES];
            int res = topologicalSort(n, adj, in_deg, order);

            printf("{\"status\":\"success\",\"is_dag\":%d,\"sequence\":[", res != -1 ? 1 : 0);
            if (res != -1) {
                for (int i = 0; i < res; i++) {
                    if (i) printf(",");
                    printf("%d", order[i]);
                }
            }
            printf("],\"stages\":[\"Payload Verification\",\"Pre-flight Avionics\",\"Flight Trajectory Calc\",\"Corridor Clearance\",\"Final Descent & Drop-off\"]}\n");
        } else if (strcmp(cmd, "EXIT") == 0) {
            printf("{\"status\":\"success\",\"message\":\"Daemon exiting\"}\n");
            break;
        } else {
            printf("{\"status\":\"error\",\"message\":\"Unknown command: %s\"}\n", cmd);
        }
    }
}

int main(int argc, char *argv[]) {
    initSystem();

    if (argc > 1 && (strcmp(argv[1], "--daemon") == 0 || strcmp(argv[1], "--api") == 0)) {
        apiLoop();
        teardownSystem();
        return 0;
    }

    /* Standalone CLI mode */
    printf("SkyRoute Autonomous Drone Dispatch Engine\n");
    printf("Initializing demo dataset...\n");
    initDemoData();
    printf("Ready. Run with '--daemon' flag to connect with Web UI bridge.\n");
    teardownSystem();
    return 0;
}
