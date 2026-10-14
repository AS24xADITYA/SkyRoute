# SkyRoute — Autonomous Air Corridor Dispatch & Algorithmic Platform

[![C11](https://img.shields.io/badge/C-C11%2FC99-blue.svg)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![Node.js](https://img.shields.io/badge/Node.js-v18%2B-green.svg)](https://nodejs.org/)
[![D3.js](https://img.shields.io/badge/Visualization-D3.js%20v7-orange.svg)](https://d3js.org/)
[![License](https://img.shields.io/badge/License-MIT-purple.svg)](LICENSE)
[![Academic Project](https://img.shields.io/badge/Course-Data%20Structures%20%26%20Algorithms-informational.svg)]()

> A mission-control drone logistics and air-corridor dispatch platform built from first principles in **pure C (C11)** with **no external C libraries**, paired with a persistent **Node.js daemon bridge** and a high-performance **D3.js interactive dashboard**.

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                 WEB DASHBOARD (HTML5 / D3 / CSS3)           │
│  - Force-Directed Airspace Radar   - Real-time Telemetry    │
│  - Min-Heap & Queue Visualizers    - Floyd-Warshall Heatmap │
└──────────────────────────────▲──────────────────────────────┘
                               │ HTTP / SSE Event Stream
┌──────────────────────────────▼──────────────────────────────┐
│                  NODE.JS EXPRESS BRIDGE                     │
│  - RESTful API Endpoints           - Stdout Watchdog & JSON │
│  - Persistent Process Daemon       - Real-time Broadcast    │
└──────────────────────────────▲──────────────────────────────┘
                               │ stdin / stdout Pipe (Line-Buffered JSON)
┌──────────────────────────────▼──────────────────────────────┐
│                    C11 CORE ENGINE                          │
│  - Graph Adjacency Lists           - Binary Min-Heap        │
│  - Hash Table (Chaining)           - 26-way Trie            │
│  - Binary Search Tree (BST)        - Disjoint Set (DSU)     │
│  - Dijkstra / BFS / Floyd-Warshall - LIFO Undo Stack        │
│  - Merge Sort / Heap Sort / Quick Sort                      │
└─────────────────────────────────────────────────────────────┘
```

---

## Data Structures & Algorithms Implementation Matrix

| Module | Data Structure | Algorithm | Source File | Time Complexity | Space Complexity |
|---|---|---|---|---|---|
| **Fleet Manager** | Sequential Array | Array Shifting & Boundary Clamping | `src/drone.c` | $O(n)$ decommission | $O(MAX\_DRONES)$ |
| **Drone Indexing** | Hash Table (Chaining) | Modulo Division Hash | `src/drone_hash.c` | $O(1)$ avg lookup | $O(TABLE\_SIZE + n)$ |
| **Fleet Sorting** | Binary Heap (In-place) | **Heap Sort** (Ascending Battery) | `src/algorithms.c` | $O(n \log n)$ | $O(1)$ |
| **Fleet Sorting** | Auxiliary Array | **Merge Sort** (Descending Battery) | `src/algorithms.c` | $O(n \log n)$ | $O(n)$ |
| **Fleet Sorting** | In-place Partition | **Quick Sort** (Drone ID Ascending) | `src/algorithms.c` | $O(n \log n)$ avg | $O(\log n)$ |
| **Drone Search** | Sorted Array | **Binary Search** | `src/algorithms.c` | $O(\log n)$ | $O(1)$ |
| **Air Corridor Network** | Adjacency List | Dynamic Edge Masking (No-Fly) | `src/graph.c` | $O(1)$ toggle | $O(V + E)$ |
| **Single-Source Route** | Priority Vertex Scan | **Dijkstra's Algorithm** | `src/algorithms.c` | $O(V^2)$ | $O(V)$ |
| **Minimum Hop Route** | FIFO Circular Queue | **Breadth-First Search (BFS)** | `src/algorithms.c` | $O(V + E)$ | $O(V)$ |
| **All-Pairs Shortest Path** | Distance Matrix | **Floyd-Warshall** | `src/algorithms.c` | $O(V^3)$ | $O(V^2)$ |
| **Airspace Connectivity** | Disjoint-Set Forest | **Union-Find (Path Compression & Rank)** | `src/union_find.c` | $O(\alpha(V))$ | $O(V)$ |
| **Cluster Discovery** | Recursive Call Stack | **Depth-First Search (DFS)** | `src/algorithms.c` | $O(V + E)$ | $O(V)$ |
| **Pre-flight Pipeline** | In-degree Array + Queue | **Kahn's Topological Sort (DAG)** | `src/algorithms.c` | $O(V + E)$ | $O(V)$ |
| **Dispatch Queuing** | Dual Circular Queues | Strict FIFO Partitioning | `src/request.c` | $O(1)$ push/pop | $O(MAX\_QUEUE)$ |
| **Priority Scheduling** | Complete Binary Tree | **Binary Min-Heap** (Sift-Up / Down) | `src/heap.c` | $O(\log n)$ insert/extract | $O(MAX\_HEAP)$ |
| **Assignment Rollback** | LIFO Array Stack | **Undo Stack** | `src/history.c` | $O(1)$ push/pop | $O(MAX\_HISTORY)$ |
| **Charging Bays** | Circular Array Queue | FIFO Turnaround Model | `src/station.c` | $O(1)$ turnaround | $O(MAX\_WAITING)$ |
| **Customer Ledger** | Node Pointer Tree | **Binary Search Tree (BST)** | `src/customer.c` | $O(\log n)$ avg | $O(n)$ |
| **Customer Autocomplete** | 26-way Branching Tree | **Prefix Trie** | `src/trie.c` | $O(L)$ query length | $O(\Sigma \cdot L)$ |

---

## Key Platform Features

1. **Airspace Radar & Dynamic Corridors**:
   - D3 force-directed visualizer representing warehouses, solar charging bays, and customer delivery zones.
   - Click any corridor to immediately restrict or restore it as a **No-Fly Zone**.
   - Live route calculation comparing weighted physical distance (**Dijkstra**) vs minimum hops (**BFS**).
   - Animated drone flight along calculated shortest trajectories.

2. **Fleet Commander**:
   - Instant $O(1)$ drone lookup via separate chaining hash table with fallback to $O(\log n)$ binary search.
   - Comprehensive sorting suite:
     - **Merge Sort**: Sorts battery in descending order (locates the readiest drone for long routes).
     - **Heap Sort**: Sorts battery in ascending order (locates low-reserve units needing charging).
     - **Quick Sort**: Sorts drones by ID ascending (prepares the fleet array for binary search).
   - Safe drone decommissioning with automated hash table re-indexing.
   - Automated solar charging stations with FIFO queueing turnaround.

3. **Priority Dispatch & Min-Heap Engine**:
   - Dual FIFO queues separating critical emergency medical orders from standard packages.
   - Array-backed Binary Min-Heap priority queue with live tree diagram visualizer.
   - One-click LIFO undo stack to revert accidental assignments.

4. **Dynamic Programming & Connectivity Matrix**:
   - $14 \times 14$ all-pairs distance heatmap computed via **Floyd-Warshall**. Click any cell to load the trajectory into the flight radar.
   - Partition analysis using **Disjoint Set Union (Union-Find)** and **DFS**.
   - Workflow dependency simulation with **Kahn's Topological Sort**.

5. **Customer Directory**:
   - Real-time prefix autocomplete powered by an alphabet-branching **Trie**.
   - Sorted customer ledger maintained in a **Binary Search Tree (BST)** with in-order traversal and path tracing.

---

## Quickstart & Setup Guide

### 1. Prerequisites
Before running SkyRoute, ensure the following are installed on your machine:
- **C Compiler**: GCC with C11 support
  - *Windows*: [MinGW-w64](https://www.mingw-w64.org/) or [w64devkit](https://github.com/skeeto/w64devkit/releases) (ensure `gcc` is in your `PATH`)
  - *Linux*: `sudo apt install build-essential`
  - *macOS*: `xcode-select --install`
- **Node.js**: v18.0 or newer ([nodejs.org](https://nodejs.org/))
- **Git**: For version control

---

### 2. Step-by-Step Installation

#### Step 1: Clone the Repository
```bash
git clone https://github.com/AS24xADITYA/SkyRoute.git
cd SkyRoute
```

#### Step 2: Install Node.js Dependencies
```bash
npm install
```

#### Step 3: Build the C Core Engine
You can compile using `npm run build` or directly with GCC:

- **Via NPM:**
  ```bash
  npm run build
  ```
- **Windows (MinGW / PowerShell / CMD):**
  ```bash
  build.bat
  ```
  *or directly:*
  ```bash
  gcc -std=c11 -Wall -Wextra src/*.c -o skyroute.exe
  ```
- **Linux / macOS:**
  ```bash
  make
  ```

#### Step 4: Launch the Platform
```bash
npm start
```

Open your browser and navigate to:
```text
http://localhost:3000
```

---

### 3. Troubleshooting & Verification

- **C Daemon not compiling?**
  - Verify GCC is available in your terminal: `gcc --version`
  - Ensure all source files in `src/` are compiled together: `gcc -std=c11 src/*.c -o skyroute.exe`
- **Port 3000 already in use?**
  - Change the port in `server.js` or set the `PORT` environment variable: `PORT=3001 npm start`
- **Standalone C Engine Testing:**
  - You can test the C engine independently without Node.js by running `./skyroute.exe` or `skyroute.exe --daemon`.


---

## Project Structure

```
SkyRoute/
├── src/                          # Pure C11 Algorithmic Engine
│   ├── common.h                  # Global limits, types, and enumerations
│   ├── drone.h / drone.c         # Fleet array management & battery updates
│   ├── drone_hash.h / .c         # Chaining hash table for O(1) ID lookups
│   ├── customer.h / customer.c   # Binary Search Tree (BST) customer ledger
│   ├── trie.h / trie.c           # 26-way prefix tree for name autocomplete
│   ├── graph.h / graph.c         # Adjacency list airspace graph & No-Fly zones
│   ├── request.h / request.c     # Dual FIFO queues & order linked list
│   ├── heap.h / heap.c           # Binary Min-Heap priority queue
│   ├── history.h / history.c     # LIFO Undo Stack for rollback
│   ├── station.h / station.c     # Solar charging bay FIFO queue
│   ├── union_find.h / .c         # Disjoint Set Union (path compression + rank)
│   ├── algorithms.h / .c         # Sorting (Heap, Merge, Quick) & Graph Algos
│   └── main.c                    # Persistent line-buffered JSON daemon & CLI
├── public/                       # Glassmorphic Web Dashboard
│   ├── index.html                # Responsive semantic HTML5 dashboard
│   ├── style.css                 # Slate & glassmorphism theme (no AI neon)
│   └── app.js                    # D3.js force graph, SSE stream, & API client
├── server.js                     # Express daemon bridge & child_process manager
├── package.json                  # Node metadata, start and build scripts
├── Makefile                      # Cross-platform GNU Makefile
├── build.bat                     # Windows MinGW build batch script
├── .gitignore                    # Git exclusions
└── README.md                     # Comprehensive documentation
```

---

## Standalone C CLI Mode

The C engine can also run as an independent, interactive command-line simulator without Node.js:
```bash
./skyroute.exe
```
Or in persistent pipe mode:
```bash
./skyroute.exe --daemon
```

### Supported Daemon Protocol Commands
```text
INIT_DEMO                             # Seeds 14 nodes, 17 corridors, 6 drones, 8 customers, 5 orders
GET_STATE                             # Dumps complete platform state in single-line JSON
ADD_DRONE <id> <bat> <loc> <weight>   # Registers new drone into array & hash table
REMOVE_DRONE <id>                     # Decommissions IDLE drone and re-indexes table
SORT_DRONES <merge|heap|id>           # Executes Merge Sort, Heap Sort, or Quick Sort
SEARCH_DRONE <id>                     # O(1) Hash Map lookup with O(log n) Binary Search fallback
ADD_CUSTOMER <id> <name> <node>       # Inserts customer into BST and Trie
SEARCH_CUSTOMER_BST <id>              # Traces path through BST
SEARCH_CUSTOMER_TRIE <prefix>         # Prefix search in O(L)
ADD_STATION <sid> <nid> <slots>       # Adds solar charging bay
SEND_TO_CHARGE <did> <sid>            # Docks drone or places into station FIFO queue
FREE_SLOT <sid>                       # Vacates charging slot & pulls next waiting drone
SET_NOFLY <u> <v> <0|1>               # Restricts (0) or restores (1) corridor
ADD_ORDER <id> <cid> <weight> <prio>  # Enqueues request into FIFO and Min-Heap
ASSIGN_ORDER                          # Runs Dijkstra dispatch, updates drone & marks in-transit
COMPLETE_DELIVERY <oid> <did> <node>  # Finalizes delivery & updates drone location/battery
UNDO                                  # Pops last assignment from LIFO Stack
RUN_BFS <src> <dst>                   # Unweighted minimum-hop traversal
RUN_DIJKSTRA <src> <dst>              # Single-source shortest distance path
RUN_FLOYD_WARSHALL                    # Generates all-pairs distance matrix
RUN_COMPONENTS                        # Partitions graph via DFS & Union-Find
RUN_TOPO_SORT                         # Validates delivery DAG with Kahn's Algorithm
EXIT                                  # Shuts down daemon
```

---

## License

This project is licensed under the MIT License — see the [LICENSE](LICENSE) file for details.
