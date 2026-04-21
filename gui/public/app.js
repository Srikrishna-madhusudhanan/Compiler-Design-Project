// DOM Elements
const editor = document.getElementById('editor');
const irOutput = document.getElementById('ir-output');
const terminalOutput = document.getElementById('terminal-output');
const compileBtn = document.getElementById('compile-btn');
const runBtn = document.getElementById('run-btn');
const optLevel = document.getElementById('opt-level');
const exampleSelect = document.getElementById('example-select');
const animAstBtn = document.getElementById('anim-ast');
const animRaBtn = document.getElementById('anim-ra');
const animSchedBtn = document.getElementById('anim-sched');
const funcSelect = document.getElementById('func-select');
const asmOutput = document.getElementById('asm-output');


// Metrics Elements
const mPreIr = document.getElementById('m-pre-ir');
const mPostIr = document.getElementById('m-post-ir');
const mTime = document.getElementById('m-time');
const mMem = document.getElementById('m-mem');

// Visualization Data
let astNetwork = null;
let raNetwork = null;
let currentAstData = null;
let currentRaDataArray = [];
let currentCfgDataArray = [];
let currentRaData = null;
let currentCfgData = null;
let metricsChart = null;

// Initialize Chart
function initChart() {
    const ctx = document.getElementById('metrics-chart').getContext('2d');
    metricsChart = new Chart(ctx, {
        type: 'bar',
        data: {
            labels: ['Instructions'],
            datasets: [
                {
                    label: 'Pre-Optimization',
                    data: [0],
                    backgroundColor: 'rgba(56, 189, 248, 0.2)',
                    borderColor: 'rgba(56, 189, 248, 1)',
                    borderWidth: 1
                },
                {
                    label: 'Post-Optimization',
                    data: [0],
                    backgroundColor: 'rgba(34, 197, 94, 0.2)',
                    borderColor: 'rgba(34, 197, 94, 1)',
                    borderWidth: 1
                }
            ]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                y: { beginAtZero: true, grid: { color: 'rgba(255,255,255,0.1)' } },
                x: { grid: { display: false } }
            },
            plugins: {
                legend: { labels: { color: '#94a3b8' } }
            }
        }
    });
}

// Logging
function log(msg, type = 'info') {
    const div = document.createElement('div');
    div.className = type;
    div.textContent = `[${new Date().toLocaleTimeString()}] ${msg}`;
    terminalOutput.appendChild(div);
    terminalOutput.scrollTop = terminalOutput.scrollHeight;
}

function clearTerminal() {
    terminalOutput.innerHTML = '';
}

function getBenchmarkInput(code) {
    // Heuristic for menu-driven interactive programs (like BST demo): choose Exit immediately.
    if (/while\s*\(\s*1\s*\)/.test(code) && /scanf\s*\(/.test(code) && /case\s+4\s*:/.test(code)) {
        return '4\n';
    }
    return '';
}

function summarizeOutput(text, maxLen = 1500) {
    if (!text) return '';
    if (text.length <= maxLen) return text;
    return `${text.slice(0, maxLen)}\n... [truncated ${text.length - maxLen} chars]`;
}

function compactRunOutput(text) {
    if (!text) return '';
    const marker = '--> Executing in QEMU...';
    const idx = text.indexOf(marker);
    const runtimeSlice = idx >= 0 ? text.slice(idx + marker.length).trim() : text;
    return summarizeOutput(runtimeSlice);
}

// Load Examples
async function loadExamples() {
    try {
        const response = await fetch('/api/examples');
        const examples = await response.json();
        examples.forEach(ex => {
            const opt = document.createElement('option');
            opt.value = ex.content;
            opt.textContent = ex.name;
            exampleSelect.appendChild(opt);
        });
    } catch (e) {
        log('Failed to load examples', 'error');
    }
}

exampleSelect.addEventListener('change', () => {
    if (exampleSelect.value) {
        editor.value = exampleSelect.value;
        log(`Loaded example: ${exampleSelect.options[exampleSelect.selectedIndex].text}`);
    }
});

// AST Animation (Incremental Build)
async function animateAst(ast) {
    if (!ast) return;
    log('Starting AST Animation...', 'info');
    
    const nodes = new vis.DataSet([]);
    const edges = new vis.DataSet([]);
    const container = document.getElementById('ast-viz');
    const options = {
        layout: { hierarchical: { direction: 'UD', sortMethod: 'directed' } },
        physics: false,
        nodes: { shape: 'box', color: '#38bdf8', font: { color: '#0f172a' } }
    };
    
    astNetwork = new vis.Network(container, { nodes, edges }, options);
    
    let nodeId = 0;
    const process = async (node, parentId = null) => {
        const id = nodeId++;
        nodes.add({ id, label: `${node.type}${node.value ? '\n' + node.value : ''}` });
        if (parentId !== null) edges.add({ from: parentId, to: id });
        
        await new Promise(r => setTimeout(r, 100));
        
        if (node.children) {
            for (let child of node.children) {
                await process(child, id);
            }
        }
        if (node.next) {
            await process(node.next, parentId); // Sibling at same level as parent? No, wait.
            // Simplified next handling for visualization
        }
    };
    
    await process(ast);
    log('AST Animation complete.', 'success');
}

// RA Animation (Detailed Simplify/Select Trace)
async function animateRa(data) {
    if (!data || !data.nodes || data.nodes.length === 0) {
        log('No register allocation data available.', 'warning');
        return;
    }
    log('🎬 Replaying Register Allocation animation...', 'info');
    
    const stackEl = document.getElementById('ra-stack');
    stackEl.innerHTML = ''; // Reset stack

    // Start fresh: reset all nodes to grey (uncolored) state, keeping same layout
    const nodes = new vis.DataSet(data.nodes.map(n => ({ 
        id: n.id, 
        label: n.name,
        color: { background: '#1e293b', border: '#334155' },
        font: { color: '#64748b', face: 'Fira Code', size: 13 },
        shape: 'dot',
        size: 22,
        shadow: false
    })));
    
    const edges = new vis.DataSet(data.edges.map(e => ({
        ...e,
        color: { color: '#1e3a5f', opacity: 0.8 },
        width: 1.5,
        smooth: { type: 'continuous' }
    })));

    const container = document.getElementById('ra-viz');
    const options = {
        physics: {
            enabled: true,
            barnesHut: {
                gravitationalConstant: -6000,
                centralGravity: 0.25,
                springLength: 110,
                springConstant: 0.04,
                damping: 0.15,
                avoidOverlap: 0.5
            },
            stabilization: { enabled: true, iterations: 250, fit: true }
        },
        interaction: { hover: true },
        nodes: { borderWidth: 2 },
        edges: { width: 1.5 }
    };

    if (raNetwork) raNetwork.destroy();
    raNetwork = new vis.Network(container, { nodes, edges }, options);

    await new Promise(resolve => {
        raNetwork.once('stabilizationIterationsDone', () => {
            raNetwork.fit({ animation: { duration: 500, easingFunction: 'easeInOutQuad' } });
            raNetwork.setOptions({ physics: { enabled: false } });
            resolve();
        });
    });

    // Keep layout locked on window resize too
    const fitNetwork = () => { if (raNetwork) raNetwork.fit(); };
    window.addEventListener('resize', fitNetwork);
    raNetwork.on('destroy', () => window.removeEventListener('resize', fitNetwork));

    // 1. Simplify Phase (Pushing to stack)
    for (let step of data.simplify_history) {
        const nodeData = data.nodes[step.node_idx];
        await new Promise(r => setTimeout(r, 450));
        
        // Highlight in graph - change to a ghost-like state
        nodes.update({ 
            id: step.node_idx, 
            color: { background: '#0f172a', border: '#1e293b' },
            font: { color: '#64748b' },
            size: 15
        });
        
        // Add to visual stack with animation
        const item = document.createElement('div');
        item.className = `ra-stack-item ${step.potential_spill ? 'spill' : ''}`;
        item.style.borderLeft = `4px solid ${step.potential_spill ? '#ef4444' : '#38bdf8'}`;
        item.innerHTML = `<div style="display:flex; justify-content:space-between; width:100%;">
            <span style="font-weight:700;">${nodeData.name}</span>
            <span style="opacity:0.6; font-size:0.7rem;">deg:${step.degree}</span>
        </div>`;
        stackEl.appendChild(item);
    }

    await new Promise(r => setTimeout(r, 800));

    // 2. Select Phase (Popping and coloring)
    const stackItems = Array.from(stackEl.children);
    for (let i = stackItems.length - 1; i >= 0; i--) {
        const step = data.simplify_history[i];
        const nodeData = data.nodes[step.node_idx];
        const stackItem = stackItems[i];
        
        await new Promise(r => setTimeout(r, 450));
        
        stackItem.style.boxShadow = '0 0 15px var(--accent-color)';
        stackItem.style.background = 'rgba(56, 189, 248, 0.2)';
        
        if (nodeData.spilled) {
            nodes.update({ 
                id: step.node_idx, 
                color: { background: '#7f1d1d', border: '#ef4444' },
                font: { color: '#fca5a5', size: 14, face: 'Fira Code', bold: true },
                size: 30,
                shape: 'box',
                label: `🔴 ${nodeData.name}\n(SPILL)`,
                shadow: { enabled: true, color: 'rgba(239, 68, 68, 0.8)', size: 15 }
            });
        } else if (nodeData.color >= 0) {
            const color = RA_PALETTE[nodeData.color % RA_PALETTE.length];
            const regName = data.reg_names ? data.reg_names[nodeData.color] : nodeData.color;
            nodes.update({ 
                id: step.node_idx, 
                color: { background: color, border: '#ffffff' },
                font: { color: '#0f172a', size: 14, face: 'Fira Code', bold: true },
                size: 30,
                label: `${nodeData.name}\n(${regName})`,
                shadow: { enabled: true, color: color + 'cc', size: 16 }
            });
        }
        
        await new Promise(r => setTimeout(r, 200));
        stackItem.style.transform = 'translateX(100%)';
        stackItem.style.opacity = '0';
        setTimeout(() => stackItem.remove(), 300);
    }
    
    log('Register Allocation coloring complete.', 'success');
}

// RA Color Palette (15 registers = 15 colors)
const RA_PALETTE = [
    "#38bdf8", "#818cf8", "#c084fc", "#f472b6", "#fb7185",
    "#fb923c", "#fbbf24", "#a3e635", "#4ade80", "#2dd4bf",
    "#22d3ee", "#60a5fa", "#a78bfa", "#e879f9", "#f43f5e"
];

// Static render of final colored RA graph (runs immediately after compile)
function renderRaGraph(data) {
    if (!data || !data.nodes || data.nodes.length === 0) return;

    const container = document.getElementById('ra-viz');

    const visNodes = new vis.DataSet(data.nodes.map(n => {
        if (n.spilled) {
            return {
                id: n.id,
                label: `🔴 ${n.name}\n(SPILL @${n.spill_offset})`,
                color: { background: '#7f1d1d', border: '#ef4444' },
                font: { color: '#fca5a5', face: 'Fira Code', size: 13, bold: true },
                shape: 'box',
                shadow: { enabled: true, color: 'rgba(239,68,68,0.6)', size: 12 },
                size: 30
            };
        } else {
            const c = RA_PALETTE[n.color % RA_PALETTE.length] || '#38bdf8';
            const regName = data.reg_names ? data.reg_names[n.color] : `r${n.color}`;
            return {
                id: n.id,
                label: `${n.name}\n(${regName})`,
                color: { background: c, border: '#ffffff' },
                font: { color: '#0f172a', face: 'Fira Code', size: 12, bold: true },
                shape: 'dot',
                shadow: { enabled: true, color: c + 'aa', size: 10 },
                size: 25
            };
        }
    }));

    const visEdges = new vis.DataSet(data.edges.map((e, i) => ({
        id: i,
        from: e.from,
        to: e.to,
        color: { color: '#334155', opacity: 0.7 },
        width: 1.5,
        smooth: { type: 'continuous' }
    })));

    const options = {
        physics: {
            enabled: true,
            barnesHut: {
                gravitationalConstant: -6000,
                centralGravity: 0.25,
                springLength: 110,
                springConstant: 0.04,
                damping: 0.15,
                avoidOverlap: 0.5
            },
            stabilization: { enabled: true, iterations: 250, fit: true }
        },
        interaction: { hover: true },
        nodes: { borderWidth: 2 },
        edges: { width: 1.5 }
    };

    if (raNetwork) raNetwork.destroy();
    raNetwork = new vis.Network(container, { nodes: visNodes, edges: visEdges }, options);
    raNetwork.once('stabilizationIterationsDone', () => {
        raNetwork.setOptions({ physics: { enabled: false } });
        raNetwork.fit({ animation: { duration: 500 } });
    });

    log(`RA graph: ${data.nodes.length} vars, ${data.edges.length} interferences, ${new Set(data.nodes.filter(n=>!n.spilled).map(n=>n.color)).size} colors used.`, 'success');
    const spills = data.nodes.filter(n => n.spilled);
    if (spills.length > 0) log(`⚠️ ${spills.length} variable(s) spilled to stack: ${spills.map(n=>n.name).join(', ')}`, 'warning');
}

// Register Usage Map Rendering
function renderRegisterUsage(data) {
    // Draw interference graph with final colors
    renderRaGraph(data);

    const body = document.getElementById('usage-body');
    if (!data || !data.liveness_history) {
        body.innerHTML = '<tr><td colspan="3" style="text-align: center; color: #444; padding: 20px;">No liveness data.</td></tr>';
        return;
    }

    body.innerHTML = '';
    data.liveness_history.forEach(step => {
        const tr = document.createElement('tr');
        
        const tdIdx = document.createElement('td');
        tdIdx.textContent = step.idx;
        
        const tdInstr = document.createElement('td');
        tdInstr.className = 'instr-line';
        tdInstr.textContent = step.instr;
        
        const tdLive = document.createElement('td');
        const tagsDiv = document.createElement('div');
        tagsDiv.className = 'live-tags';
        
        step.live.forEach(varName => {
            const span = document.createElement('span');
            const node = data.nodes.find(n => n.name === varName);
            if (node && node.spilled) {
                span.className = 'live-tag spill-tag';
                span.textContent = `${varName} (SPILL)`;
            } else {
                const regName = (node && node.color >= 0 && data.reg_names) ? ` → ${data.reg_names[node.color]}` : '';
                span.className = 'live-tag';
                if (node && node.color >= 0) {
                    span.style.borderColor = RA_PALETTE[node.color % RA_PALETTE.length];
                    span.style.color = RA_PALETTE[node.color % RA_PALETTE.length];
                }
                span.textContent = varName + regName;
            }
            tagsDiv.appendChild(span);
        });
        
        tdLive.appendChild(tagsDiv);
        tr.appendChild(tdIdx);
        tr.appendChild(tdInstr);
        tr.appendChild(tdLive);
        body.appendChild(tr);
    });
}

// CFG Rendering
async function renderCFG(data) {
    if (!data || !data.blocks || data.blocks.length === 0) return;
    log('Rendering Control Flow Graph...', 'info');

    const nodes = new vis.DataSet(data.blocks.map(b => {
        // Clean up common IR prefixes for display
        const cleanInstrs = b.instrs.map(i => i.trim());
        return {
            id: b.id,
            label: `BLOCK ${b.id}\n${'—'.repeat(20)}\n${cleanInstrs.join('\n')}`,
            shape: 'box',
            font: { face: 'Fira Code', align: 'left', size: 11, color: '#334155' },
            color: { background: '#f8fafc', border: '#334155' },
            margin: 10
        };
    }));

    const edges = [];
    data.blocks.forEach(b => {
        b.successors.forEach(s => {
            edges.push({ from: b.id, to: s, arrows: 'to', color: '#64748b', width: 2 });
        });
    });

    const container = document.getElementById('cfg-viz');
    const options = {
        layout: { hierarchical: { direction: 'UD', sortMethod: 'directed', nodeSpacing: 150, levelSeparation: 150 } },
        physics: false,
        edges: { smooth: { type: 'cubicBezier', forceDirection: 'vertical', roundness: 0.5 } }
    };

    new vis.Network(container, { nodes, edges: new vis.DataSet(edges) }, options);
    log('CFG Rendering complete.', 'success');
}

// Compile Handling
compileBtn.addEventListener('click', async () => {
    log('Compiling...', 'info');
    compileBtn.disabled = true;
    
    try {
        const response = await fetch('/api/compile', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                code: editor.value,
                optimizationLevel: optLevel.value,
                useMetrics: true
            })
        });
        
        const data = await response.json();
        if (data.error) {
            log(data.error, 'error');
        } else {
            log('Compilation successful.', 'success');
            irOutput.textContent = data.ir_opt || data.ir;
            
            // Parse Metrics (compile mode: only IR counts, no exec time/mem)
            const metrics = parseMetrics(data.metrics);
            updateDashboard(metrics);
            // Explicitly clear runtime metrics — they only belong to benchmark mode
            mTime.textContent = '-';
            mMem.textContent = '-';
            mTime.parentElement.querySelector('.metric-label').textContent = 'Exec Time (ms)';
            
            currentAstData = data.ast_json;
            currentRaDataArray = data.ra_json || [];
            currentCfgDataArray = data.cfg_json || [];
            
            // Populate function selector
            funcSelect.innerHTML = '';
            currentRaDataArray.forEach((ra, idx) => {
                const opt = document.createElement('option');
                opt.value = idx;
                opt.textContent = ra.func_name || `Function ${idx}`;
                funcSelect.appendChild(opt);
            });

            if (currentRaDataArray.length > 0) {
                currentRaData = currentRaDataArray[0];
                // Match CFG by function name if possible
                currentCfgData = currentCfgDataArray.find(c => c.func_name === currentRaData.func_name) || currentCfgDataArray[0];
            } else {
                currentRaData = null;
                currentCfgData = null;
            }
            
            // Static viz
            if (data.asm) asmOutput.textContent = data.asm;
            else asmOutput.textContent = 'Assembly not generated.';

            if (currentCfgData) renderCFG(currentCfgData);
            if (currentRaData) renderRegisterUsage(currentRaData);

        }
    } catch (e) {
        log('Connection error.', 'error');
    } finally {
        compileBtn.disabled = false;
    }
});

function parseMetrics(text) {
    if (!text) return {};
    const metrics = {};
    const lines = text.split('\n');
    
    lines.forEach(line => {
        // Handle pre/post IR counts
        const preMatch = line.match(/IR instructions, pre-opt.*:\s+(\d+)/i);
        if (preMatch) metrics.preIr = parseInt(preMatch[1]);
        
        const postMatch = line.match(/IR instructions, post-opt.*:\s+(\d+)/i);
        if (postMatch) metrics.postIr = parseInt(postMatch[1]);

        // Handle execution time (wall)
        const timeMatch = line.match(/Execution time(?:\s*\(wall\))?:\s*([\d\.,]+)/i);
        if (timeMatch) {
            metrics.time = parseFloat(timeMatch[1].replace(/,/g, ''));
            if (line.toLowerCase().includes('ns')) metrics.unit = 'ns';
            else if (line.toLowerCase().includes('ms')) metrics.unit = 'ms';
            else if (line.toLowerCase().includes(' us')) metrics.unit = 'us';
            else metrics.unit = 's';
        }

        // Handle peak memory
        const memMatch = line.match(/Peak memory(?:\s+usage)?.*:\s+(.*)/i);
        if (memMatch) {
            const rawVal = memMatch[1].trim();
            const val = rawVal.split(/\s+/)[0].replace(/,/g, '');
            const parsedMem = parseInt(val);
            if (!isNaN(parsedMem)) {
                metrics.mem = parsedMem;
            } else if (rawVal.toLowerCase().includes('unknown')) {
                metrics.mem = 'Unknown';
            }
        }
    });

    return metrics;
}

function updateDashboard(metrics) {
    if (metrics.preIr !== undefined) mPreIr.textContent = metrics.preIr;
    if (metrics.postIr !== undefined) mPostIr.textContent = metrics.postIr;
    
    if (metrics.time !== undefined && !isNaN(metrics.time)) {
        if (metrics.unit === 'ns') {
            mTime.textContent = Math.round(metrics.time);
            mTime.parentElement.querySelector('.metric-label').textContent = 'Exec Time (ns)';
        } else if (metrics.unit === 'ms') {
            mTime.textContent = metrics.time.toFixed(2);
            mTime.parentElement.querySelector('.metric-label').textContent = 'Exec Time (ms)';
        } else {
            mTime.textContent = metrics.time.toFixed(6);
        }
    } else {
        mTime.textContent = '-';
    }
    
    if (metrics.mem !== undefined) {
        if (!isNaN(metrics.mem)) {
            mMem.textContent = metrics.mem;
        } else if (typeof metrics.mem === 'string' && metrics.mem === 'Unknown') {
            mMem.textContent = 'Unknown';
        } else {
            mMem.textContent = '-';
        }
    } else {
        mMem.textContent = '-';
    }
    
    metricsChart.data.datasets[0].data = [metrics.preIr || 0];
    metricsChart.data.datasets[1].data = [metrics.postIr || 0];
    metricsChart.update();
}

function renderStaticViz(ast, ra) {
    // Basic static render
    if (ast) {
        // ... simple vis-network render without animation ...
    }
}

animAstBtn.addEventListener('click', () => animateAst(currentAstData));
animRaBtn.addEventListener('click', () => animateRa(currentRaData));
animSchedBtn.addEventListener('click', () => animateSched());

funcSelect.addEventListener('change', () => {
    const idx = parseInt(funcSelect.value);
    if (!isNaN(idx) && currentRaDataArray[idx]) {
        currentRaData = currentRaDataArray[idx];
        currentCfgData = currentCfgDataArray.find(c => c.func_name === currentRaData.func_name) || currentCfgDataArray[idx] || currentCfgDataArray[0];
        
        log(`Switched visualization to function: ${currentRaData.func_name}`, 'info');
        if (currentCfgData) renderCFG(currentCfgData);
        if (currentRaData) renderRegisterUsage(currentRaData);
    }
});

runBtn.addEventListener('click', async () => {
    if (!editor.value.trim()) { log('No code to run. Write or load a program first.', 'warning'); return; }
    clearTerminal();
    log('Running Benchmarks with QEMU...', 'info');
    runBtn.disabled = true;
    const origLabel = runBtn.textContent;
    runBtn.textContent = '⏳ Running...';

    // Countdown shown on button
    let seconds = 0;
    const timer = setInterval(() => { seconds++; runBtn.textContent = `⏳ Running... ${seconds}s`; }, 1000);
    const inferredInput = getBenchmarkInput(editor.value);
    if (inferredInput) {
        log('Auto input applied for menu-driven program: "4" (Exit)', 'info');
    }
    const controller = new AbortController();
    const fetchTimeout = setTimeout(() => controller.abort(), 30000);

    try {
        const response = await fetch('/api/run', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ code: editor.value, input: inferredInput }),
            signal: controller.signal
        });
        clearInterval(timer);
        clearTimeout(fetchTimeout);
        const data = await response.json();

        if (data.stdout) log('Program output: ' + compactRunOutput(data.stdout), 'success');
        if (data.stderr) log('Stderr: ' + summarizeOutput(data.stderr), 'warning');
        if (data.error) {
            if (data.error.includes('killed') || data.error.includes('SIGTERM')) {
                log('⚠️ Program killed — exceeded 60s timeout (infinite loop?)', 'error');
            } else {
                log('Error: ' + data.error, 'error');
            }
        }

        const metrics = parseMetrics(data.metrics);
        if (metrics.time !== undefined) {
            updateDashboard(metrics);
            log(`✅ Exec time: ${metrics.time} ms | Peak memory: ${metrics.mem} KB`, 'success');
        } else {
            log('⚠️ Metrics not available — did you compile first?', 'warning');
        }
    } catch (e) {
        clearInterval(timer);
        clearTimeout(fetchTimeout);
        if (e.name === 'AbortError') {
            log('Run cancelled after 30s client timeout.', 'error');
        } else {
            log('Run failed or timed out: ' + e.message, 'error');
        }
    } finally {
        clearInterval(timer);
        runBtn.disabled = false;
        runBtn.textContent = origLabel;
    }
});

// Init
initChart();
loadExamples();
log('System Ready.');

// Panel Controls (Min/Max)
document.querySelectorAll('.panel-controls').forEach(controls => {
    const panel = controls.closest('.panel');
    const minBtn = controls.querySelector('.min-btn');
    const maxBtn = controls.querySelector('.max-btn');

    minBtn.onclick = (e) => {
        e.stopPropagation();
        panel.classList.toggle('minimized');
        panel.classList.remove('maximized');
        document.body.classList.remove('has-maximized');
    };

    maxBtn.onclick = (e) => {
        e.stopPropagation();
        const wasMaximized = panel.classList.contains('maximized');
        
        // Remove maximized from all other panels first
        document.querySelectorAll('.panel').forEach(p => p.classList.remove('maximized'));
        
        if (!wasMaximized) {
            panel.classList.add('maximized');
            panel.classList.remove('minimized');
            document.body.classList.add('has-maximized');
        } else {
            document.body.classList.remove('has-maximized');
        }
    };
});

// ==========================================
// Resizer Logic
// ==========================================

function setupResizer(resizerId, direction) {
    const resizer = document.getElementById(resizerId);
    let isDragging = false;

    resizer.addEventListener('mousedown', (e) => {
        isDragging = true;
        resizer.classList.add('dragging');
        document.body.style.cursor = direction === 'h' ? 'col-resize' : 'row-resize';
        e.preventDefault();
    });

    document.addEventListener('mousemove', (e) => {
        if (!isDragging) return;

        if (direction === 'h') {
            if (resizerId === 'resizer-h1') {
                const pane = document.getElementById('left-pane');
                pane.style.width = `${e.clientX - 5}px`;
            } else if (resizerId === 'resizer-h2') {
                const pane = document.getElementById('right-pane');
                const width = window.innerWidth - e.clientX - 5;
                pane.style.width = `${width}px`;
            }
        } else if (direction === 'v') {
            if (resizerId === 'resizer-v1') {
                const pane = document.getElementById('metrics-panel');
                const parent = document.getElementById('center-pane');
                const parentRect = parent.getBoundingClientRect();
                const height = parentRect.bottom - e.clientY - 3;
                pane.style.height = `${height}px`;
            } else if (resizerId === 'resizer-v2') {
                const pane = document.getElementById('bottom-section');
                const height = window.innerHeight - e.clientY - 5;
                pane.style.height = `${height}px`;
            }
        }
        
        // Trigger resize event for vis-network
        if (astNetwork) astNetwork.redraw();
        if (raNetwork) raNetwork.redraw();
    });

    document.addEventListener('mouseup', () => {
        if (isDragging) {
            isDragging = false;
            resizer.classList.remove('dragging');
            document.body.style.cursor = 'default';
        }
    });
}

setupResizer('resizer-h1', 'h');
setupResizer('resizer-h2', 'h');
setupResizer('resizer-v1', 'v');
setupResizer('resizer-v2', 'v');

// Instruction Scheduling Visualization
let schedNetwork = null;
const SCHED_PALETTES = {
    critical: { bg: '#f97316', border: '#fb923c', glow: 'rgba(249,115,22,0.7)', label: 'Critical Path' },
    high:     { bg: '#a855f7', border: '#c084fc', glow: 'rgba(168,85,247,0.7)', label: 'High Priority' },
    mid:      { bg: '#38bdf8', border: '#7dd3fc', glow: 'rgba(56,189,248,0.6)', label: 'Mid Priority' },
    low:      { bg: '#4ade80', border: '#86efac', glow: 'rgba(74,222,128,0.5)', label: 'Low Priority' },
    idle:     { bg: '#1e293b', border: '#334155', glow: null,                   label: null }
};

function getSchedTier(priority, maxPriority) {
    const ratio = priority / maxPriority;
    if (ratio >= 0.9) return 'critical';
    if (ratio >= 0.6) return 'high';
    if (ratio >= 0.3) return 'mid';
    return 'low';
}

async function animateSched() {
    const currentFunc = currentRaData ? currentRaData.func_name : 'main';
    log(`Loading scheduling DAG data for ${currentFunc}...`, 'info');
    try {
        const response = await fetch(`/api/sched/${currentFunc}`);
        if (!response.ok) throw new Error('Scheduling data not found. Compile first.');
        const data = await response.json();
        if (!data.blocks || data.blocks.length === 0) {
            log('No scheduling data. Please compile first.', 'warning');
            return;
        }

        // Combine ALL blocks into one DAG with globally-unique IDs
        const allNodes = [];
        const allEdges = [];
        let globalOffset = 0;

        for (const block of data.blocks) {
            if (!block.nodes || block.nodes.length === 0) { globalOffset += 0; continue; }

            log(`⚡ Block ${block.id}: ${block.nodes.length} instructions`, 'info');

            for (const n of block.nodes) {
                allNodes.push({ ...n, id: globalOffset + n.id, blockId: block.id });
            }
            for (const e of (block.edges || [])) {
                allEdges.push({ from: globalOffset + e.from, to: globalOffset + e.to });
            }
            globalOffset += block.nodes.length;
        }

        const maxPriority = Math.max(...allNodes.map(n => n.priority), 1);

        const nodesDataset = new vis.DataSet(allNodes.map(n => ({
            id: n.id,
            label: n.label.trim(),
            title: `Block ${n.blockId} | Priority: ${n.priority}`,
            color: { background: SCHED_PALETTES.idle.bg, border: SCHED_PALETTES.idle.border },
            font: { color: '#64748b', face: 'Fira Code', size: 12, bold: false },
            shape: 'box',
            margin: { top: 8, bottom: 8, left: 12, right: 12 },
            borderWidth: 1,
            shadow: false
        })));

        const edgesDataset = new vis.DataSet(allEdges.map((e, i) => ({
            id: i,
            from: e.from,
            to: e.to,
            arrows: { to: { enabled: true, scaleFactor: 0.8 } },
            color: { color: '#1e3a5f', highlight: '#38bdf8', opacity: 0.7 },
            width: 1.5,
            smooth: { type: 'curvedCW', roundness: 0.2 },
        })));

        const container = document.getElementById('sched-viz');
        const options = {
            layout: {
                hierarchical: {
                    direction: 'UD',
                    sortMethod: 'directed',
                    nodeSpacing: 140,
                    levelSeparation: 100
                }
            },
            physics: false,
            interaction: { hover: true, tooltipDelay: 100 },
            nodes: { borderWidth: 2 }
        };

        if (schedNetwork) schedNetwork.destroy();
        schedNetwork = new vis.Network(container, { nodes: nodesDataset, edges: edgesDataset }, options);
        schedNetwork.fit({ animation: { duration: 500 } });

        // Animate: schedule nodes in priority order across all blocks
        await new Promise(r => setTimeout(r, 600));
        log('🎬 Animating scheduling order (critical path first)...', 'info');

        const sortedNodes = [...allNodes].sort((a, b) => b.priority - a.priority);

        for (let node of sortedNodes) {
            await new Promise(r => setTimeout(r, 500));

            const tier = getSchedTier(node.priority, maxPriority);
            const pal = SCHED_PALETTES[tier];

            nodesDataset.update({
                id: node.id,
                color: { background: pal.bg, border: pal.border },
                font: { color: '#ffffff', face: 'Fira Code', size: 13, bold: true },
                shadow: { enabled: true, color: pal.glow, size: 18, x: 0, y: 0 },
                borderWidth: 2
            });

            // Highlight outgoing edges
            const outEdges = allEdges
                .map((e, i) => ({ ...e, id: i }))
                .filter(e => e.from === node.id);
            outEdges.forEach(eu => edgesDataset.update({ id: eu.id, color: { color: pal.border, opacity: 1 }, width: 2.5 }));

            log(`→ Scheduled: "${node.label.trim()}" [Block ${node.blockId}] [${tier.toUpperCase()}, priority: ${node.priority}]`,
                tier === 'critical' ? 'warning' : 'success');
        }

        schedNetwork.fit({ animation: { duration: 800, easingFunction: 'easeInOutQuad' } });
        log('✅ Instruction scheduling visualization complete.', 'success');

    } catch (err) {
        log(`Error: ${err.message}`, 'error');
    }
}


document.getElementById('anim-sched').addEventListener('click', animateSched);
