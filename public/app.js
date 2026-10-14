/**
 * SkyRoute — Autonomous Drone Delivery & Algorithmic Dispatch System
 * Core Frontend Application Logic & D3 Force Visualization
 *
 * Clean, modular architecture:
 * 1. Global state management with reactive SSE stream
 * 2. D3.js force-directed flight corridor radar
 * 3. Sorting (Merge Sort, Heap Sort, Quick Sort) & O(1) Hash Map
 * 4. Dual FIFO dispatch queues & Binary Min-Heap Priority Queue
 * 5. Graph algorithms (Dijkstra, BFS, Floyd-Warshall, Union-Find, Topo Sort)
 * 6. Customer lookup (26-way Trie & Binary Search Tree)
 */

// Application State
let currentState = {
    drones: [],
    customers: [],
    stations: [],
    nodes: [],
    edges: [],
    orders: [],
    queues: { emergency: [], normal: [], heap: [] },
    history_count: 0,
    no_fly_count: 0
};

// D3 Force Graph Globals
let simulation = null;
let svg = null;
let gRoot = null;
let linkElements = null;
let nodeElements = null;
let edgeLabels = null;
let zoomBehavior = null;

// Modal State
let pendingDecommissionDroneId = null;

// Node Names for Realistic Logistics Simulation
const NODE_NAMES = {
    0: "Central Sky Hub",
    1: "North Air Depot",
    2: "West Logistics Hub",
    3: "Solar Station Alpha",
    4: "Substation Beta",
    5: "Rooftop Charger",
    6: "Metro General Hospital",
    7: "Silicon Tech Park",
    8: "Downtown Plaza",
    9: "Skyline Tower",
    10: "Harbor Bay Point",
    11: "Green Hills Suburb",
    12: "Apex University",
    13: "Airport Zone"
};

/* ─── Initialization ─────────────────────────────────────────────── */
document.addEventListener('DOMContentLoaded', () => {
    initTabs();
    initD3Graph();
    initSSE();
    fetchState();
    logTelemetry('SkyRoute platform loaded. Connected to C engine.', 'system');
});

/* ─── Tabs Navigation ────────────────────────────────────────────── */
function initTabs() {
    const tabBtns = document.querySelectorAll('.tab-btn');
    tabBtns.forEach(btn => {
        btn.addEventListener('click', () => {
            tabBtns.forEach(b => b.classList.remove('active'));
            document.querySelectorAll('.tab-pane').forEach(p => p.classList.remove('active'));

            btn.classList.add('active');
            const target = btn.getAttribute('data-tab');
            const pane = document.getElementById(target);
            if (pane) pane.classList.add('active');

            // Tab-specific refreshes
            if (target === 'tab-algorithms') {
                computeFloydWarshall();
                checkComponents();
                runTopoSort();
            } else if (target === 'tab-radar') {
                if (simulation) simulation.alpha(0.2).restart();
            } else if (target === 'tab-dispatch') {
                renderHeapTree(currentState.queues?.heap || []);
            }
        });
    });
}

/* ─── SSE Stream for Real-time Updates ───────────────────────────── */
function initSSE() {
    try {
        const evtSource = new EventSource('/api/events');
        evtSource.addEventListener('state_change', () => {
            fetchState();
        });
        evtSource.onerror = () => {
            setTimeout(fetchState, 3000);
        };
    } catch (e) {
        setInterval(fetchState, 3000);
    }
}

async function fetchState() {
    try {
        const res = await fetch('/api/state');
        if (!res.ok) return;
        const data = await res.json();
        currentState = data;
        updateUI();
    } catch (err) {
        console.error("State fetch error:", err);
    }
}

function updateUI() {
    // 1. Navbar Metrics
    document.getElementById('stat-drones').textContent = currentState.drones?.length || 0;
    const pendingCount = (currentState.orders || []).filter(o => o.status === 0).length;
    document.getElementById('stat-orders').textContent = pendingCount;
    document.getElementById('stat-nofly').textContent = currentState.no_fly_count || 0;

    // 2. Tab 1: D3 Graph & Route Dropdowns
    updateD3Graph();
    populateRouteDropdowns();
    updateNoFlyList();

    // 3. Tab 2: Fleet & Stations
    renderFleetCards();
    renderStations();

    // 4. Tab 3: Dispatch Queues, Heap & Undo Stack
    renderDispatchQueues();
    renderHeapTree(currentState.queues?.heap || []);
    renderUndoStack();

    // 5. Tab 5: Customers BST
    renderCustomersTable();
}

/* ─── D3 Force-Directed Graph ────────────────────────────────────── */
function initD3Graph() {
    svg = d3.select('#network-svg');
    const container = document.getElementById('graph-container');
    const width = container.clientWidth || 800;
    const height = container.clientHeight || 540;

    svg.selectAll('*').remove();

    zoomBehavior = d3.zoom()
        .scaleExtent([0.3, 4])
        .on('zoom', (event) => {
            gRoot.attr('transform', event.transform);
        });
    svg.call(zoomBehavior);

    gRoot = svg.append('g').attr('class', 'graph-root');

    simulation = d3.forceSimulation()
        .force('link', d3.forceLink().id(d => d.id).distance(120))
        .force('charge', d3.forceManyBody().strength(-400))
        .force('center', d3.forceCenter(width / 2, height / 2))
        .force('collision', d3.forceCollide().radius(40));
}

function updateD3Graph() {
    if (!currentState.nodes || currentState.nodes.length === 0) return;

    const container = document.getElementById('graph-container');
    const width = container.clientWidth || 800;
    const height = container.clientHeight || 540;
    simulation.force('center', d3.forceCenter(width / 2, height / 2));

    const nodesData = currentState.nodes.map(n => ({
        id: n.id,
        kind: n.kind,
        name: NODE_NAMES[n.id] || `Zone ${n.id}`
    }));

    const linksData = (currentState.edges || []).map(e => ({
        source: e.from,
        target: e.to,
        distance: e.distance,
        is_open: e.is_open
    }));

    // Corridors (Lines)
    linkElements = gRoot.selectAll('.graph-link')
        .data(linksData, d => `${d.source.id || d.source}-${d.target.id || d.target}`)
        .join('line')
        .attr('class', d => `graph-link ${d.is_open ? 'open' : 'nofly'}`)
        .attr('stroke', d => d.is_open ? 'rgba(59, 130, 246, 0.45)' : '#ef4444')
        .attr('stroke-width', d => d.is_open ? 2.5 : 3.5)
        .attr('stroke-dasharray', d => d.is_open ? null : '6,4')
        .style('cursor', 'pointer')
        .on('click', (event, d) => {
            const u = typeof d.source === 'object' ? d.source.id : d.source;
            const v = typeof d.target === 'object' ? d.target.id : d.target;
            const nextState = d.is_open ? 0 : 1;
            toggleNoFlyEdge(u, v, nextState);
        });

    // Distance Labels
    edgeLabels = gRoot.selectAll('.edge-label')
        .data(linksData, d => `${d.source.id || d.source}-${d.target.id || d.target}`)
        .join('text')
        .attr('class', 'edge-label')
        .attr('fill', d => d.is_open ? '#64748b' : '#ef4444')
        .attr('font-size', '10px')
        .attr('font-family', 'JetBrains Mono, monospace')
        .attr('text-anchor', 'middle')
        .text(d => `${d.distance} km${d.is_open ? '' : ' [BLOCKED]'}`);

    // Nodes (G elements)
    nodeElements = gRoot.selectAll('.graph-node')
        .data(nodesData, d => d.id)
        .join('g')
        .attr('class', 'graph-node')
        .call(d3.drag()
            .on('start', dragStarted)
            .on('drag', dragged)
            .on('end', dragEnded));

    nodeElements.selectAll('*').remove();

    // Node Outer Ring
    nodeElements.append('circle')
        .attr('r', d => d.kind === 0 ? 20 : (d.kind === 1 ? 18 : 16))
        .attr('fill', d => {
            if (d.kind === 0) return 'rgba(59, 130, 246, 0.15)';
            if (d.kind === 1) return 'rgba(245, 158, 11, 0.15)';
            return 'rgba(16, 185, 129, 0.15)';
        })
        .attr('stroke', d => {
            if (d.kind === 0) return '#3b82f6';
            if (d.kind === 1) return '#f59e0b';
            return '#10b981';
        })
        .attr('stroke-width', 2);

    // Node Core Dot
    nodeElements.append('circle')
        .attr('r', 7)
        .attr('fill', d => {
            if (d.kind === 0) return '#3b82f6';
            if (d.kind === 1) return '#f59e0b';
            return '#10b981';
        });

    // Node ID
    nodeElements.append('text')
        .attr('dy', 4)
        .attr('text-anchor', 'middle')
        .attr('font-family', 'JetBrains Mono, monospace')
        .attr('font-size', '10px')
        .attr('font-weight', 'bold')
        .attr('fill', '#ffffff')
        .text(d => d.id);

    // Node Title Subtext
    nodeElements.append('text')
        .attr('dy', 30)
        .attr('text-anchor', 'middle')
        .attr('font-family', 'Inter, sans-serif')
        .attr('font-size', '10px')
        .attr('font-weight', '500')
        .attr('fill', '#94a3b8')
        .text(d => d.name);

    simulation.nodes(nodesData).on('tick', ticked);
    simulation.force('link').links(linksData);
    simulation.alpha(0.2).restart();
}

function ticked() {
    if (!linkElements || !nodeElements) return;

    linkElements
        .attr('x1', d => d.source.x)
        .attr('y1', d => d.source.y)
        .attr('x2', d => d.target.x)
        .attr('y2', d => d.target.y);

    if (edgeLabels) {
        edgeLabels
            .attr('x', d => (d.source.x + d.target.x) / 2)
            .attr('y', d => (d.source.y + d.target.y) / 2 - 5);
    }

    nodeElements.attr('transform', d => `translate(${d.x},${d.y})`);
}

function dragStarted(event, d) {
    if (!event.active) simulation.alphaTarget(0.3).restart();
    d.fx = d.x;
    d.fy = d.y;
}

function dragged(event, d) {
    d.fx = event.x;
    d.fy = event.y;
}

function dragEnded(event, d) {
    if (!event.active) simulation.alphaTarget(0);
    d.fx = null;
    d.fy = null;
}

function zoomInGraph() {
    if (svg && zoomBehavior) {
        svg.transition().duration(300).call(zoomBehavior.scaleBy, 1.3);
    }
}

function zoomOutGraph() {
    if (svg && zoomBehavior) {
        svg.transition().duration(300).call(zoomBehavior.scaleBy, 0.75);
    }
}

function resetGraphZoom() {
    if (svg && zoomBehavior) {
        svg.transition().duration(400).call(zoomBehavior.transform, d3.zoomIdentity);
    }
}

/* ─── Route Highlighting & Drone Flight Animation ────────────────── */
function highlightPath(pathNodes, isDelivery = false) {
    if (!gRoot || !pathNodes || pathNodes.length < 2) return;

    // Reset corridors
    gRoot.selectAll('.graph-link')
        .attr('stroke', d => d.is_open ? 'rgba(59, 130, 246, 0.45)' : '#ef4444')
        .attr('stroke-width', d => d.is_open ? 2.5 : 3.5);

    // Highlight path segments
    for (let i = 0; i < pathNodes.length - 1; i++) {
        const u = pathNodes[i];
        const v = pathNodes[i + 1];
        gRoot.selectAll('.graph-link')
            .filter(d => {
                const su = typeof d.source === 'object' ? d.source.id : d.source;
                const sv = typeof d.target === 'object' ? d.target.id : d.target;
                return (su === u && sv === v) || (su === v && sv === u);
            })
            .attr('stroke', isDelivery ? '#38bdf8' : '#60a5fa')
            .attr('stroke-width', 4);
    }
}

function animateDroneFlight(droneId, orderId, path, distance, targetNode) {
    if (!path || path.length < 2) return;

    const hud = document.getElementById('flight-hud');
    const hudTitle = document.getElementById('hud-title');
    const hudPath = document.getElementById('hud-path');
    const hudFill = document.getElementById('hud-fill');

    hud.classList.remove('hidden');
    hudTitle.textContent = `Drone #${droneId} Dispatched for Order #${orderId}`;
    hudPath.textContent = `Flight Corridor: ${path.join(' ➔ ')} | Total Route: ${distance.toFixed(1)} km`;
    hudFill.style.width = '0%';

    highlightPath(path, true);

    const nodesMap = {};
    simulation.nodes().forEach(n => { nodesMap[n.id] = n; });

    // Minimalist SVG Drone Marker
    const sprite = gRoot.append('g').attr('class', 'active-drone-sprite');
    sprite.append('circle')
        .attr('r', 12)
        .attr('fill', 'rgba(59, 130, 246, 0.35)')
        .attr('stroke', '#60a5fa')
        .attr('stroke-width', 2);
    // Sleek aerodynamic arrow silhouette (no emojis)
    sprite.append('polygon')
        .attr('points', '0,-6 6,5 0,2 -6,5')
        .attr('fill', '#ffffff');

    const startNode = nodesMap[path[0]];
    if (!startNode) return;
    sprite.attr('transform', `translate(${startNode.x},${startNode.y})`);

    let currentHop = 0;
    const totalHops = path.length - 1;
    const hopDuration = 1000;

    function flyHop() {
        if (currentHop >= totalHops) {
            hudFill.style.width = '100%';
            setTimeout(async () => {
                sprite.remove();
                hud.classList.add('hidden');

                await fetch('/api/order/complete', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ orderId, droneId, finalNode: targetNode })
                });

                logTelemetry(`[DELIVERY COMPLETE] Drone #${droneId} arrived at Zone ${targetNode}. Order #${orderId} marked DELIVERED.`, 'success');
                fetchState();
            }, 500);
            return;
        }

        const dstNode = nodesMap[path[currentHop + 1]];
        currentHop++;
        hudFill.style.width = `${(currentHop / totalHops) * 100}%`;

        sprite.transition()
            .duration(hopDuration)
            .ease(d3.easeQuadInOut)
            .attr('transform', `translate(${dstNode.x},${dstNode.y})`)
            .on('end', flyHop);
    }

    flyHop();
}

/* ─── Global Actions ──────────────────────────────────────────────── */
async function initDemoData() {
    try {
        const res = await fetch('/api/demo/init', { method: 'POST' });
        await res.json();
        logTelemetry(`[INIT] Loaded 14 Nodes, 17 Corridors, 6 Drones, 8 Customers, 5 Orders.`, 'success');
        fetchState();
    } catch (e) {
        console.error(e);
    }
}

async function assignNextOrder() {
    try {
        logTelemetry(`[HEAP] Extracting highest-priority order from Min-Heap (O(log n))...`, 'dsa');
        const res = await fetch('/api/order/assign', { method: 'POST' });
        const data = await res.json();

        if (data.status === 'success') {
            logTelemetry(`[DIJKSTRA] Shortest trajectory calculated in O(V^2). Assigned Order #${data.order_id} to Drone #${data.drone_id}.`, 'action');
            logTelemetry(`[LIFO STACK] Pushed assignment snapshot to Undo Stack (O(1)).`, 'dsa');
            animateDroneFlight(data.drone_id, data.order_id, data.path, data.distance, data.target_node);
            fetchState();
        } else {
            logTelemetry(`[DISPATCH FAILED] ${data.message}`, 'warning');
            alert(data.message);
        }
    } catch (e) {
        console.error(e);
    }
}

async function triggerUndo() {
    try {
        const res = await fetch('/api/undo', { method: 'POST' });
        const data = await res.json();
        if (data.status === 'success') {
            logTelemetry(`[UNDO STACK] Popped assignment snapshot from LIFO Stack (O(1)). State reverted.`, 'dsa');
            fetchState();
        } else {
            logTelemetry(`[UNDO] ${data.message}`, 'warning');
            alert(data.message);
        }
    } catch (e) {
        console.error(e);
    }
}

async function toggleNoFlyEdge(u, v, open) {
    try {
        await fetch('/api/nofly', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ u, v, open })
        });
        logTelemetry(`[AIRSPACE] Corridor (${u}, ${v}) ${open ? 'RESTORED' : 'RESTRICTED as No-Fly Zone'}.`, open ? 'success' : 'warning');
        fetchState();
    } catch (e) {
        console.error(e);
    }
}

function toggleNoFly(open) {
    const u = parseInt(document.getElementById('nofly-u').value);
    const v = parseInt(document.getElementById('nofly-v').value);
    toggleNoFlyEdge(u, v, open);
}

function updateNoFlyList() {
    const list = document.getElementById('nofly-active-list');
    const blocked = (currentState.edges || []).filter(e => e.is_open === 0);
    if (blocked.length === 0) {
        list.innerHTML = '<span class="muted-text">No active airspace restrictions.</span>';
        return;
    }
    list.innerHTML = blocked.map(b =>
        `<span class="nofly-tag">Corridor ${b.from} ➔ ${b.to} (${b.distance}km)</span>`
    ).join(' ');
}

function populateRouteDropdowns() {
    const src = document.getElementById('route-src');
    const dst = document.getElementById('route-dst');
    if (!src || !dst || !currentState.nodes) return;

    const curSrc = src.value;
    const curDst = dst.value;

    const options = currentState.nodes.map(n =>
        `<option value="${n.id}">Node ${n.id} (${NODE_NAMES[n.id] || 'Zone'})</option>`
    ).join('');

    src.innerHTML = options;
    dst.innerHTML = options;

    if (curSrc) src.value = curSrc;
    if (curDst) dst.value = curDst;
    else if (currentState.nodes.length > 6) dst.value = "6";
}

async function calculateDijkstra() {
    const s = document.getElementById('route-src').value;
    const d = document.getElementById('route-dst').value;
    try {
        logTelemetry(`[DIJKSTRA] Running single-source shortest path from Node ${s} to Node ${d}...`, 'dsa');
        const res = await fetch(`/api/algo/dijkstra?src=${s}&dst=${d}`);
        const data = await res.json();
        const out = document.getElementById('route-output');
        if (data.found) {
            out.innerHTML = `<strong>Dijkstra Route:</strong> ${data.path.join(' ➔ ')}<br>
                             <strong>Total Flight Distance:</strong> ${data.distance.toFixed(1)} km<br>
                             <strong>Time Complexity:</strong> O(V²)`;
            highlightPath(data.path);
        } else {
            out.innerHTML = `<span style="color: var(--rose);">No path exists. Corridors blocked by No-Fly restrictions.</span>`;
        }
    } catch (e) {
        console.error(e);
    }
}

async function calculateBFS() {
    const s = document.getElementById('route-src').value;
    const d = document.getElementById('route-dst').value;
    try {
        logTelemetry(`[BFS] Running unweighted breadth-first traversal from Node ${s} to Node ${d}...`, 'dsa');
        const res = await fetch(`/api/algo/bfs?src=${s}&dst=${d}`);
        const data = await res.json();
        const out = document.getElementById('route-output');
        if (data.found) {
            out.innerHTML = `<strong>BFS Route:</strong> ${data.path.join(' ➔ ')}<br>
                             <strong>Hops Required:</strong> ${data.hops} hops<br>
                             <strong>Time Complexity:</strong> O(V + E)`;
            highlightPath(data.path);
        } else {
            out.innerHTML = `<span style="color: var(--rose);">No path exists between Node ${s} and Node ${d}.</span>`;
        }
    } catch (e) {
        console.error(e);
    }
}

/* ─── TAB 2: FLEET COMMANDER ─────────────────────────────────────── */
function renderFleetCards() {
    const container = document.getElementById('drone-cards-container');
    if (!container) return;

    if (!currentState.drones || currentState.drones.length === 0) {
        container.innerHTML = '<div class="empty-state">No registered drones in active fleet.</div>';
        return;
    }

    const statusMap = {
        0: { text: 'IDLE', class: 'idle' },
        1: { text: 'IN FLIGHT', class: 'delivering' },
        2: { text: 'CHARGING', class: 'charging' },
        3: { text: 'OFFLINE', class: 'offline' }
    };

    container.innerHTML = currentState.drones.map(d => {
        const st = statusMap[d.status] || statusMap[0];
        const batteryColor = d.battery > 50 ? 'var(--emerald)' : (d.battery > 20 ? 'var(--amber)' : 'var(--rose)');
        const canDecommission = d.status === 0;

        return `
            <div class="drone-card">
                <div class="drone-card-top">
                    <span class="drone-id-tag">Drone #${d.id}</span>
                    <span class="drone-status-badge ${st.class}">${st.text}</span>
                </div>
                <div class="battery-meter">
                    <div class="battery-label-row">
                        <span>Battery Reserve</span>
                        <strong style="color: ${batteryColor}; font-family: var(--font-mono);">${d.battery}%</strong>
                    </div>
                    <div class="battery-bar-track">
                        <div class="battery-bar-level" style="width: ${d.battery}%; background: ${batteryColor};"></div>
                    </div>
                </div>
                <div class="drone-meta-row">
                    <div class="meta-item">
                        <span class="meta-label">Current Node</span>
                        <span class="meta-val">Node ${d.location}</span>
                    </div>
                    <div class="meta-item">
                        <span class="meta-label">Payload Capacity</span>
                        <span class="meta-val">${d.max_carry} kg</span>
                    </div>
                </div>
                <div class="drone-card-actions">
                    ${d.status === 0 && d.battery < 90 ? `
                        <button class="btn btn-sm btn-primary" onclick="openDockModal(${d.id})">Dock at Station</button>
                    ` : ''}
                    <button class="btn btn-sm ${canDecommission ? 'btn-danger' : 'btn-outline'}"
                            onclick="${canDecommission ? `openDecommissionModal(${d.id})` : ''}"
                            style="${!canDecommission ? 'opacity: 0.45; cursor: not-allowed;' : ''}"
                            title="${canDecommission ? 'Decommission idle drone' : 'Only IDLE drones can be decommissioned'}">
                        Decommission
                    </button>
                </div>
            </div>
        `;
    }).join('');
}

function renderStations() {
    const container = document.getElementById('stations-container');
    if (!container) return;

    if (!currentState.stations || currentState.stations.length === 0) {
        container.innerHTML = '<div class="empty-state">No charging bays configured.</div>';
        return;
    }

    container.innerHTML = currentState.stations.map(s => {
        return `
            <div class="station-bay-card">
                <div class="card-header-compact">
                    <h4>Bay #${s.id} (Node ${s.node_id})</h4>
                    <span class="badge-tech">FIFO Queue</span>
                </div>
                <div class="drone-meta-row">
                    <div class="meta-item">
                        <span class="meta-label">Free Slots</span>
                        <span class="meta-val">${s.free_slots} / ${s.total_slots}</span>
                    </div>
                    <div class="meta-item">
                        <span class="meta-label">Occupied</span>
                        <span class="meta-val">${s.occupied}</span>
                    </div>
                </div>
                <div style="font-size: 0.75rem; color: var(--text-secondary); margin-bottom: 0.75rem;">
                    <strong>Waiting Queue:</strong> [ ${s.waiting.length ? s.waiting.map(id => `#${id}`).join(', ') : 'Empty'} ]
                </div>
                <div class="button-group">
                    <button class="btn btn-sm btn-secondary" onclick="freeStationSlot(${s.id})">Release Slot & Next</button>
                </div>
            </div>
        `;
    }).join('');
}

/* ─── Sorting Algorithms ─────────────────────────────────────────── */
async function sortFleet(algo) {
    try {
        let desc = "Sorting fleet...";
        if (algo === 'merge_desc' || algo === 'merge') {
            desc = "Merge Sort O(n log n) — Battery (Descending: readiest drone first)";
        } else if (algo === 'heap_asc' || algo === 'heap') {
            desc = "Heap Sort O(n log n) — Battery (Ascending: lowest reserve first)";
        } else if (algo === 'id_asc' || algo === 'id') {
            desc = "Quick Sort O(n log n) — Drone ID (Ascending: enables Binary Search)";
        }

        logTelemetry(`[SORT] Executing ${desc}...`, 'dsa');
        const res = await fetch('/api/drones/sort', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ algo })
        });
        const data = await res.json();

        // Update active sort indicator
        const badge = document.getElementById('active-sort-badge');
        if (badge) badge.textContent = desc;

        logTelemetry(`[HASH TABLE] Re-indexed drone hash mappings in O(n).`, 'dsa');
        fetchState();
    } catch (e) {
        console.error("Sort error:", e);
    }
}

/* ─── Search Drone (Hash Table O(1) & Binary Search O(log n)) ───── */
async function searchDrone() {
    const id = document.getElementById('search-drone-id').value;
    if (!id) return;

    try {
        logTelemetry(`[LOOKUP] Querying Drone #${id} using O(1) Hash Map & O(log n) Binary Search...`, 'dsa');
        const res = await fetch(`/api/drone/search/${id}`);
        const data = await res.json();
        const banner = document.getElementById('drone-search-banner');
        banner.classList.remove('hidden');

        if (data.found) {
            banner.innerHTML = `<strong>Found Drone #${data.drone.id}:</strong> Battery: ${data.drone.battery}% | Location: Node ${data.drone.location} | Method: <code>${data.method}</code>`;
        } else {
            banner.innerHTML = `<span style="color: var(--rose);">Drone #${id} not found in fleet.</span>`;
        }
    } catch (e) {
        console.error(e);
    }
}

/* ─── Decommission Modal & Workflow ──────────────────────────────── */
function openDecommissionModal(id) {
    pendingDecommissionDroneId = id;
    const msg = document.getElementById('decommission-modal-msg');
    if (msg) msg.textContent = `Are you sure you want to decommission Drone #${id} from active fleet duty?`;
    document.getElementById('modal-decommission').classList.add('active');
}

async function confirmDecommission() {
    if (!pendingDecommissionDroneId) return;
    const id = pendingDecommissionDroneId;
    closeModal('modal-decommission');

    try {
        const res = await fetch(`/api/drone/${id}`, { method: 'DELETE' });
        const data = await res.json();
        if (data.status === 'success') {
            logTelemetry(`Drone #${id} successfully decommissioned from fleet.`, 'action');
        } else {
            logTelemetry(`Failed to decommission Drone #${id}: ${data.message || 'Error'}`, 'warning');
            alert(data.message || 'Cannot decommission drone');
        }
        fetchState();
    } catch (e) {
        console.error(e);
    }
}

/* ─── Dock Drone Modal (Replaces browser prompt) ─────────────────── */
function openDockModal(droneId) {
    document.getElementById('dock-drone-id').value = droneId;
    document.getElementById('dock-modal-title').textContent = `Dock Drone #${droneId} for Recharging`;
    const sel = document.getElementById('dock-station-select');

    if (currentState.stations && currentState.stations.length > 0) {
        sel.innerHTML = currentState.stations.map(s => `
            <option value="${s.id}">Station #${s.id} at Node ${s.node_id} (${s.free_slots}/${s.total_slots} slots available)</option>
        `).join('');
    } else {
        sel.innerHTML = '<option value="">No charging stations available</option>';
    }

    document.getElementById('modal-dock').classList.add('active');
}

async function submitDockDrone(e) {
    e.preventDefault();
    const droneId = parseInt(document.getElementById('dock-drone-id').value);
    const stationId = parseInt(document.getElementById('dock-station-select').value);
    if (!stationId) return;

    try {
        await fetch('/api/drone/charge', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ droneId, stationId })
        });
        closeModal('modal-dock');
        logTelemetry(`Drone #${droneId} routed to Charging Station #${stationId}.`, 'action');
        fetchState();
    } catch (err) {
        console.error(err);
    }
}

async function freeStationSlot(stationId) {
    try {
        const res = await fetch('/api/station/free', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ stationId })
        });
        const data = await res.json();
        if (data.freed_drone && data.freed_drone !== -1) {
            logTelemetry(`[QUEUE] Slot freed at Station #${stationId}. Dequeued Drone #${data.freed_drone} into charging bay.`, 'action');
        } else {
            logTelemetry(`[QUEUE] Released vacant pad at Station #${stationId}.`, 'action');
        }
        fetchState();
    } catch (e) {
        console.error(e);
    }
}

/* ─── TAB 3: Priority Dispatch & Min-Heap ────────────────────────── */
function renderDispatchQueues() {
    const eList = document.getElementById('emergency-orders-list');
    const nList = document.getElementById('normal-orders-list');
    const eCount = document.getElementById('emergency-count');
    const nCount = document.getElementById('normal-count');

    const orders = currentState.orders || [];
    const emergencyOrders = orders.filter(o => o.priority === 1 && o.status === 0);
    const normalOrders = orders.filter(o => o.priority === 0 && o.status === 0);

    eCount.textContent = emergencyOrders.length;
    nCount.textContent = normalOrders.length;

    eList.innerHTML = emergencyOrders.length ? emergencyOrders.map(o => `
        <div class="order-card" style="border-left: 3px solid var(--rose);">
            <div>
                <span class="order-card-id">Order #${o.id}</span>
                <div style="font-size: 0.7rem; color: var(--text-muted);">Node ${o.customer_node_id} • ${o.weight}kg</div>
            </div>
            <span class="badge-emergency">EMERGENCY</span>
        </div>
    `).join('') : '<div class="empty-state">No emergency requests queued</div>';

    nList.innerHTML = normalOrders.length ? normalOrders.map(o => `
        <div class="order-card" style="border-left: 3px solid var(--blue);">
            <div>
                <span class="order-card-id">Order #${o.id}</span>
                <div style="font-size: 0.7rem; color: var(--text-muted);">Node ${o.customer_node_id} • ${o.weight}kg</div>
            </div>
            <span class="badge-normal">STANDARD</span>
        </div>
    `).join('') : '<div class="empty-state">No standard requests queued</div>';
}

function renderHeapTree(heapData) {
    const slotsRow = document.getElementById('heap-array-slots');
    const svgTree = d3.select('#heap-tree-svg');
    svgTree.selectAll('*').remove();

    if (!heapData || heapData.length === 0) {
        slotsRow.innerHTML = '<span class="muted-text">Heap is empty</span>';
        return;
    }

    // Array Buffer slots
    slotsRow.innerHTML = heapData.map((it, idx) => `
        <div class="array-slot ${it.priority === 1 ? 'emergency' : ''}">
            <span class="slot-index">[${idx}]</span>
            <span class="slot-key">#${it.order_id}</span>
            <span class="slot-index">k:${it.key}</span>
        </div>
    `).join('');

    // D3 Complete Binary Tree Coordinates
    const container = document.getElementById('heap-tree-container');
    const width = container.clientWidth || 500;
    const height = 320;
    svgTree.attr('height', height);

    const treeNodes = heapData.map((d, i) => {
        const level = Math.floor(Math.log2(i + 1));
        const indexInLevel = i - (Math.pow(2, level) - 1);
        const totalInLevel = Math.pow(2, level);
        const x = (width / (totalInLevel + 1)) * (indexInLevel + 1);
        const y = 40 + level * 70;
        return { ...d, index: i, x, y };
    });

    // Connecting Edges
    treeNodes.forEach(n => {
        if (n.index > 0) {
            const parentIdx = Math.floor((n.index - 1) / 2);
            const parent = treeNodes[parentIdx];
            svgTree.append('line')
                .attr('x1', parent.x)
                .attr('y1', parent.y)
                .attr('x2', n.x)
                .attr('y2', n.y)
                .attr('stroke', 'rgba(255, 255, 255, 0.15)')
                .attr('stroke-width', 1.5);
        }
    });

    // Tree Nodes
    const nodeG = svgTree.selectAll('.heap-node')
        .data(treeNodes)
        .enter()
        .append('g')
        .attr('transform', d => `translate(${d.x},${d.y})`);

    nodeG.append('circle')
        .attr('r', 16)
        .attr('fill', d => d.priority === 1 ? 'rgba(239, 68, 68, 0.2)' : 'rgba(59, 130, 246, 0.2)')
        .attr('stroke', d => d.priority === 1 ? '#ef4444' : '#3b82f6')
        .attr('stroke-width', 1.5);

    nodeG.append('text')
        .attr('dy', 4)
        .attr('text-anchor', 'middle')
        .attr('font-size', '10px')
        .attr('font-family', 'JetBrains Mono, monospace')
        .attr('fill', '#ffffff')
        .text(d => `#${d.order_id}`);
}

function renderUndoStack() {
    const view = document.getElementById('undo-stack-view');
    if (!view) return;

    if (!currentState.history_count || currentState.history_count === 0) {
        view.innerHTML = '<div class="empty-state">Undo stack is currently empty</div>';
        return;
    }

    view.innerHTML = `
        <div class="stack-item">
            <span>Stack Depth: ${currentState.history_count} snapshots</span>
            <span class="badge-tech">TOP</span>
        </div>
    `;
}

/* ─── TAB 4: FLOYD-WARSHALL & CONNECTIVITY ────────────────────────── */
async function computeFloydWarshall() {
    try {
        const res = await fetch('/api/algo/floyd-warshall');
        const data = await res.json();
        const container = document.getElementById('floyd-table-container');

        if (!data.matrix) return;

        let html = '<table class="matrix-table"><thead><tr><th>Node</th>';
        for (let j = 0; j < data.node_count; j++) {
            html += `<th>N${j}</th>`;
        }
        html += '</tr></thead><tbody>';

        for (let i = 0; i < data.node_count; i++) {
            html += `<tr><th>N${i}</th>`;
            for (let j = 0; j < data.node_count; j++) {
                const val = data.matrix[i][j];
                if (i === j) {
                    html += `<td class="diag">0</td>`;
                } else if (val < 0) {
                    html += `<td class="inf">∞</td>`;
                } else {
                    html += `<td class="dist" title="From Node ${i} to Node ${j}" onclick="setRouteAndRun(${i}, ${j})">${val}</td>`;
                }
            }
            html += '</tr>';
        }
        html += '</tbody></table>';
        container.innerHTML = html;
        logTelemetry(`[FLOYD-WARSHALL] Computed ${data.node_count}x${data.node_count} all-pairs distance matrix.`, 'success');
    } catch (e) {
        console.error(e);
    }
}

function setRouteAndRun(src, dst) {
    document.getElementById('route-src').value = src;
    document.getElementById('route-dst').value = dst;
    document.querySelector('[data-tab="tab-radar"]').click();
    calculateDijkstra();
}

async function checkComponents() {
    try {
        logTelemetry(`[UNION-FIND] Auditing connected components with path compression (O(α(n)))...`, 'dsa');
        const res = await fetch('/api/algo/components');
        const data = await res.json();
        const out = document.getElementById('components-output');
        document.getElementById('stat-components').textContent = data.dfs_count;

        out.innerHTML = `
            <div style="margin-bottom: 0.75rem;">
                <strong>Connected Sub-graphs:</strong>
                <span style="color: ${data.dfs_count === 1 ? 'var(--emerald)' : 'var(--rose)'}; font-size: 1.1rem; font-weight: bold; margin-left: 6px;">
                    ${data.dfs_count}
                </span>
                <span class="badge-tech" style="margin-left: 8px;">DFS & Union-Find verified</span>
            </div>
            <p style="font-size: 0.8rem; color: var(--text-secondary); margin-bottom: 0.5rem;">
                ${data.dfs_count === 1 ? 'Network is fully connected. All zones accessible.' : 'Airspace is partitioned into isolated clusters by active No-Fly zones.'}
            </p>
            <div style="display: flex; flex-wrap: wrap; gap: 4px;">
                ${data.components.map(c => `
                    <span class="badge-tech" style="background: rgba(255, 255, 255, 0.05); color: var(--text-secondary); font-size: 0.7rem;">
                        Node ${c.node} ➔ Comp ${c.comp}
                    </span>
                `).join('')}
            </div>
        `;
    } catch (e) {
        console.error(e);
    }
}

async function runTopoSort() {
    try {
        logTelemetry(`[TOPO SORT] Linearizing pre-flight dependencies with Kahn's Algorithm (O(V + E))...`, 'dsa');
        const res = await fetch('/api/algo/topo-sort');
        const data = await res.json();
        const container = document.getElementById('topo-pipeline-container');

        container.innerHTML = `
            <div style="font-size: 0.8rem; color: var(--text-secondary); margin-bottom: 0.75rem;">
                <strong>Valid DAG Workflow:</strong> ${data.is_dag ? 'Linearizable (Cycle-free)' : 'Cycle detected'}
            </div>
            <div style="display: flex; flex-direction: column; gap: 0.45rem;">
                ${data.stages.map((stage, i) => `
                    <div class="topo-stage-node">
                        <span style="font-family: var(--font-mono); color: var(--blue); font-weight: 600;">Stage ${i + 1}:</span>
                        <span>${stage}</span>
                    </div>
                `).join('')}
            </div>
        `;
    } catch (e) {
        console.error(e);
    }
}

/* ─── TAB 5: TRIE & BST DIRECTORY ────────────────────────────────── */
let trieDebounce = null;
function onTrieInput(query) {
    clearTimeout(trieDebounce);
    trieDebounce = setTimeout(async () => {
        const grid = document.getElementById('trie-results');
        if (!query.trim()) {
            grid.innerHTML = '<div class="muted-text">Type prefix to query Trie nodes...</div>';
            return;
        }

        try {
            logTelemetry(`[TRIE] Prefix query '${query}' in O(L) time complexity...`, 'dsa');
            const res = await fetch(`/api/customer/search/trie/${encodeURIComponent(query)}`);
            const data = await res.json();

            if (data.matches && data.matches.length) {
                grid.innerHTML = data.matches.map(m => `
                    <div class="trie-result-pill">
                        <span class="trie-match-name">${m.name}</span>
                        <span class="trie-match-id">Customer #${m.id}</span>
                    </div>
                `).join('');
            } else {
                grid.innerHTML = `<span class="muted-text">No names matching prefix '${query}' in Trie</span>`;
            }
        } catch (e) {
            console.error(e);
        }
    }, 200);
}

async function searchCustomerBST() {
    const id = document.getElementById('bst-search-id').value;
    if (!id) return;

    try {
        logTelemetry(`[BST] Searching Customer #${id} in Binary Search Tree...`, 'dsa');
        const res = await fetch(`/api/customer/search/bst/${id}`);
        const data = await res.json();
        const banner = document.getElementById('bst-trace-banner');
        banner.classList.remove('hidden');

        if (data.found) {
            banner.innerHTML = `<strong>Found Customer ${data.customer.name}:</strong> Assigned to Zone ${data.customer.node_id}<br>
                                <strong>BST Traversal Path:</strong> [ ${data.steps.join(' ➔ ')} ] (Depth: ${data.steps.length})`;
        } else {
            banner.innerHTML = `<span style="color: var(--rose);">Customer #${id} not found. Path traversed: [ ${data.steps.join(' ➔ ')} ]</span>`;
        }
    } catch (e) {
        console.error(e);
    }
}

function renderCustomersTable() {
    const tbody = document.getElementById('customers-bst-body');
    const selectCust = document.getElementById('m-order-customer');
    if (!tbody || !currentState.customers) return;

    tbody.innerHTML = currentState.customers.map(c => `
        <tr>
            <td style="font-family: var(--font-mono); font-weight: 600; color: #60a5fa;">#${c.id}</td>
            <td>${c.name}</td>
            <td>Node ${c.node_id} (${NODE_NAMES[c.node_id] || 'Zone'})</td>
            <td><span class="badge-tech" style="background: rgba(16, 185, 129, 0.1); color: #34d399; border-color: rgba(16, 185, 129, 0.25);">Registered</span></td>
        </tr>
    `).join('');

    if (selectCust) {
        selectCust.innerHTML = currentState.customers.map(c => `
            <option value="${c.id}">${c.name} (ID: ${c.id}, Node: ${c.node_id})</option>
        `).join('');
    }
}

/* ─── Modal Form Submissions ─────────────────────────────────────── */
function showAddDroneModal()    { document.getElementById('modal-drone').classList.add('active'); }
function showAddOrderModal()    { document.getElementById('modal-order').classList.add('active'); }
function showAddCustomerModal() { document.getElementById('modal-customer').classList.add('active'); }
function showAddEdgeModal()     { document.getElementById('modal-edge').classList.add('active'); }
function showAddStationModal()  { document.getElementById('modal-station').classList.add('active'); }
function closeModal(id)         { document.getElementById(id).classList.remove('active'); }

async function submitAddDrone(e) {
    e.preventDefault();
    const id = parseInt(document.getElementById('m-drone-id').value);
    const battery = parseInt(document.getElementById('m-drone-battery').value);
    const location = parseInt(document.getElementById('m-drone-location').value);
    const weight = parseFloat(document.getElementById('m-drone-weight').value);

    try {
        await fetch('/api/drone', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ id, battery, location, weight })
        });
        closeModal('modal-drone');
        logTelemetry(`[HASH] Registered Drone #${id} into Fleet & Hash Table.`, 'success');
        fetchState();
    } catch (err) {
        console.error(err);
    }
}

async function submitAddOrder(e) {
    e.preventDefault();
    const id = parseInt(document.getElementById('m-order-id').value);
    const customerId = parseInt(document.getElementById('m-order-customer').value);
    const weight = parseFloat(document.getElementById('m-order-weight').value);
    const priority = parseInt(document.getElementById('m-order-priority').value);

    try {
        await fetch('/api/order', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ id, customerId, weight, priority })
        });
        closeModal('modal-order');
        logTelemetry(`[HEAP] Enqueued Order #${id} with Min-Heap sift-up (O(log n)). Priority: ${priority ? 'EMERGENCY' : 'STANDARD'}.`, 'success');
        fetchState();
    } catch (err) {
        console.error(err);
    }
}

async function submitAddCustomer(e) {
    e.preventDefault();
    const id = parseInt(document.getElementById('m-cust-id').value);
    const name = document.getElementById('m-cust-name').value;
    const nodeId = parseInt(document.getElementById('m-cust-node').value);

    try {
        await fetch('/api/customer', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ id, name, nodeId })
        });
        closeModal('modal-customer');
        logTelemetry(`[BST & TRIE] Customer '${name}' (#${id}) inserted into BST and Trie prefix index.`, 'success');
        fetchState();
    } catch (err) {
        console.error(err);
    }
}

async function submitAddEdge(e) {
    e.preventDefault();
    const u = parseInt(document.getElementById('m-edge-u').value);
    const v = parseInt(document.getElementById('m-edge-v').value);
    const distance = parseFloat(document.getElementById('m-edge-dist').value);

    try {
        await fetch('/api/edge', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ u, v, distance })
        });
        closeModal('modal-edge');
        logTelemetry(`Air corridor added between Node ${u} and Node ${v} (${distance}km).`, 'success');
        fetchState();
    } catch (err) {
        console.error(err);
    }
}

async function submitAddStation(e) {
    e.preventDefault();
    const sid = parseInt(document.getElementById('m-station-id').value);
    const nid = parseInt(document.getElementById('m-station-node').value);
    const slots = parseInt(document.getElementById('m-station-slots').value);

    try {
        await fetch('/api/station', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ sid, nid, slots })
        });
        closeModal('modal-station');
        logTelemetry(`Deployed Solar Charging Bay #${sid} at Node ${nid} with ${slots} slots.`, 'success');
        fetchState();
    } catch (err) {
        console.error(err);
    }
}

/* ─── Telemetry Console ──────────────────────────────────────────── */
function logTelemetry(msg, type = 'system') {
    const stream = document.getElementById('telemetry-log');
    if (!stream) return;

    const time = new Date().toLocaleTimeString();
    const entry = document.createElement('div');
    entry.className = `log-entry ${type}`;
    entry.innerHTML = `<span class="log-time">[${time}]</span> ${msg}`;

    stream.appendChild(entry);
    stream.scrollTop = stream.scrollHeight;
}

function clearLogs() {
    const stream = document.getElementById('telemetry-log');
    if (stream) stream.innerHTML = '';
}

function toggleTelemetry() {
    const bar = document.getElementById('telemetry-bar');
    const btn = document.getElementById('btn-toggle-telemetry');
    if (!bar) return;
    bar.classList.toggle('collapsed');
    if (btn) {
        btn.textContent = bar.classList.contains('collapsed') ? 'Expand' : 'Collapse';
    }
}

// Window resize listener to re-center D3 graph
window.addEventListener('resize', () => {
    const container = document.getElementById('graph-container');
    if (container && simulation) {
        const w = container.clientWidth;
        const h = container.clientHeight;
        simulation.force('center', d3.forceCenter(w / 2, h / 2));
        simulation.alpha(0.15).restart();
    }
});
