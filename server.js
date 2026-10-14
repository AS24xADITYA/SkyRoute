const express = require('express');
const { spawn } = require('child_process');
const readline = require('readline');
const path = require('path');

const app = express();
const port = 3000;

app.use(express.json());
app.use(express.static('public'));

/* ─── Persistent C Daemon Bridge ────────────────────────────────── */
const exePath = path.join(__dirname, 'skyroute.exe');
let daemon = null;
let rl = null;
const commandQueue = [];
let isProcessing = false;
let processingTimer = null;
const sseClients = new Set();

function broadcastEvent(type, data) {
    const payload = `event: ${type}\ndata: ${JSON.stringify(data)}\n\n`;
    for (const client of sseClients) {
        try {
            client.write(payload);
        } catch (e) {
            sseClients.delete(client);
        }
    }
}

function processNext() {
    if (commandQueue.length === 0 || isProcessing) return;
    if (!daemon || daemon.killed) {
        startDaemon();
        return;
    }

    isProcessing = true;
    const current = commandQueue[0];

    // Set safety watchdog timeout (5 seconds)
    clearTimeout(processingTimer);
    processingTimer = setTimeout(() => {
        if (isProcessing && commandQueue.length > 0) {
            console.warn('[DAEMON] Command timed out:', current.cmd);
            const timedOut = commandQueue.shift();
            isProcessing = false;
            timedOut.reject(new Error('C daemon response timeout'));
            processNext();
        }
    }, 5000);

    daemon.stdin.write(current.cmd.trim() + '\n');
}

function sendCommand(cmd) {
    return new Promise((resolve, reject) => {
        commandQueue.push({ cmd, resolve, reject });
        if (!isProcessing) {
            processNext();
        }
    });
}

function startDaemon() {
    console.log(`[DAEMON] Spawning persistent C process: ${exePath}`);
    daemon = spawn(exePath, ['--daemon']);
    rl = readline.createInterface({ input: daemon.stdout });

    rl.on('line', (line) => {
        line = line.trim();
        if (!line) return;

        // Ensure we only process JSON lines to prevent desync
        if (!line.startsWith('{') && !line.startsWith('[')) {
            console.log(`[C non-JSON stdout]: ${line}`);
            return;
        }

        if (commandQueue.length > 0) {
            clearTimeout(processingTimer);
            const current = commandQueue.shift();
            isProcessing = false;
            try {
                const parsed = JSON.parse(line);
                current.resolve(parsed);
            } catch (err) {
                current.resolve({ raw: line, error: 'JSON parse error' });
            }
            processNext();
        }
    });

    daemon.stderr.on('data', (d) => {
        console.error(`[C stderr]: ${d.toString().trim()}`);
    });

    daemon.on('close', (code) => {
        console.warn(`[DAEMON] Process closed with exit code ${code}. Restarting in 1s...`);
        isProcessing = false;
        clearTimeout(processingTimer);
        setTimeout(startDaemon, 1000);
    });

    // Auto-seed initial demo state
    setTimeout(() => {
        sendCommand('INIT_DEMO')
            .then(() => console.log('[DAEMON] Demo dataset initialized successfully'))
            .catch(console.error);
    }, 200);
}

startDaemon();

/* ─── SSE Stream for Real-time State Updates ─────────────────────── */
app.get('/api/events', (req, res) => {
    res.setHeader('Content-Type', 'text/event-stream');
    res.setHeader('Cache-Control', 'no-cache');
    res.setHeader('Connection', 'keep-alive');
    res.flushHeaders();

    sseClients.add(res);
    res.write(`event: connected\ndata: {"status":"connected"}\n\n`);

    req.on('close', () => {
        sseClients.delete(res);
    });
});

/* ─── REST Endpoints ────────────────────────────────────────────── */

/* Get complete platform state */
app.get('/api/state', async (req, res) => {
    try {
        const state = await sendCommand('GET_STATE');
        res.json(state);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

/* Demo Controls */
app.post('/api/demo/init', async (req, res) => {
    try {
        const result = await sendCommand('INIT_DEMO');
        broadcastEvent('state_change', { action: 'INIT_DEMO' });
        res.json(result);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

app.post('/api/demo/clear', async (req, res) => {
    try {
        const result = await sendCommand('CLEAR_ALL');
        broadcastEvent('state_change', { action: 'CLEAR_ALL' });
        res.json(result);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

/* Fleet Management */
app.post('/api/drone', async (req, res) => {
    const { id, battery, location, weight } = req.body;
    try {
        const result = await sendCommand(`ADD_DRONE ${id} ${battery} ${location} ${weight}`);
        broadcastEvent('state_change', { action: 'ADD_DRONE', id });
        res.json(result);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

app.delete('/api/drone/:id', async (req, res) => {
    try {
        const result = await sendCommand(`REMOVE_DRONE ${req.params.id}`);
        broadcastEvent('state_change', { action: 'REMOVE_DRONE', id: req.params.id });
        res.json(result);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

app.post('/api/drones/sort', async (req, res) => {
    const algo = req.body.algo || 'heap';
    try {
        const result = await sendCommand(`SORT_DRONES ${algo}`);
        broadcastEvent('state_change', { action: 'SORT_DRONES', algo });
        res.json(result);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

app.get('/api/drone/search/:id', async (req, res) => {
    try {
        const result = await sendCommand(`SEARCH_DRONE ${req.params.id}`);
        res.json(result);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

/* Charging Stations */
app.post('/api/station', async (req, res) => {
    const { sid, nid, slots } = req.body;
    try {
        const result = await sendCommand(`ADD_STATION ${sid} ${nid} ${slots}`);
        broadcastEvent('state_change', { action: 'ADD_STATION' });
        res.json(result);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

app.post('/api/drone/charge', async (req, res) => {
    const { droneId, stationId } = req.body;
    try {
        const result = await sendCommand(`SEND_TO_CHARGE ${droneId} ${stationId}`);
        broadcastEvent('state_change', { action: 'SEND_TO_CHARGE' });
        res.json(result);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

app.post('/api/station/free', async (req, res) => {
    const { stationId } = req.body;
    try {
        const result = await sendCommand(`FREE_SLOT ${stationId}`);
        broadcastEvent('state_change', { action: 'FREE_SLOT' });
        res.json(result);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

/* Customer Directory (BST & Trie) */
app.post('/api/customer', async (req, res) => {
    const { id, name, nodeId } = req.body;
    try {
        const result = await sendCommand(`ADD_CUSTOMER ${id} ${name} ${nodeId}`);
        broadcastEvent('state_change', { action: 'ADD_CUSTOMER' });
        res.json(result);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

app.get('/api/customer/search/bst/:id', async (req, res) => {
    try {
        const result = await sendCommand(`SEARCH_CUSTOMER_BST ${req.params.id}`);
        res.json(result);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

app.get('/api/customer/search/trie/:prefix', async (req, res) => {
    try {
        const result = await sendCommand(`SEARCH_CUSTOMER_TRIE ${req.params.prefix}`);
        res.json(result);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

/* Air Corridor Network & No-Fly Zones */
app.post('/api/node', async (req, res) => {
    const { kind } = req.body;
    try {
        const result = await sendCommand(`ADD_NODE ${kind}`);
        broadcastEvent('state_change', { action: 'ADD_NODE' });
        res.json(result);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

app.post('/api/edge', async (req, res) => {
    const { u, v, distance } = req.body;
    try {
        const result = await sendCommand(`ADD_EDGE ${u} ${v} ${distance}`);
        broadcastEvent('state_change', { action: 'ADD_EDGE' });
        res.json(result);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

app.post('/api/nofly', async (req, res) => {
    const { u, v, open } = req.body;
    try {
        const result = await sendCommand(`SET_NOFLY ${u} ${v} ${open ? 1 : 0}`);
        broadcastEvent('state_change', { action: 'SET_NOFLY' });
        res.json(result);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

/* Orders, Dispatch & LIFO Undo */
app.post('/api/order', async (req, res) => {
    const { id, customerId, weight, priority } = req.body;
    try {
        const result = await sendCommand(`ADD_ORDER ${id} ${customerId} ${weight} ${priority}`);
        broadcastEvent('state_change', { action: 'ADD_ORDER' });
        res.json(result);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

app.post('/api/order/assign', async (req, res) => {
    try {
        const result = await sendCommand('ASSIGN_ORDER');
        broadcastEvent('state_change', { action: 'ASSIGN_ORDER', result });
        res.json(result);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

app.post('/api/order/complete', async (req, res) => {
    const { orderId, droneId, finalNode } = req.body;
    try {
        const result = await sendCommand(`COMPLETE_DELIVERY ${orderId} ${droneId} ${finalNode}`);
        broadcastEvent('state_change', { action: 'COMPLETE_DELIVERY' });
        res.json(result);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

app.post('/api/undo', async (req, res) => {
    try {
        const result = await sendCommand('UNDO');
        broadcastEvent('state_change', { action: 'UNDO' });
        res.json(result);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

/* Graph Algorithms Explorer */
app.get('/api/algo/bfs', async (req, res) => {
    const { src, dst } = req.query;
    try {
        const result = await sendCommand(`RUN_BFS ${src} ${dst}`);
        res.json(result);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

app.get('/api/algo/dijkstra', async (req, res) => {
    const { src, dst } = req.query;
    try {
        const result = await sendCommand(`RUN_DIJKSTRA ${src} ${dst}`);
        res.json(result);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

app.get('/api/algo/floyd-warshall', async (req, res) => {
    try {
        const result = await sendCommand('RUN_FLOYD_WARSHALL');
        res.json(result);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

app.get('/api/algo/components', async (req, res) => {
    try {
        const result = await sendCommand('RUN_COMPONENTS');
        res.json(result);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

app.get('/api/algo/topo-sort', async (req, res) => {
    try {
        const result = await sendCommand('RUN_TOPO_SORT');
        res.json(result);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

app.listen(port, () => {
    console.log(`SkyRoute Dispatch Platform active at http://localhost:${port}`);
});
