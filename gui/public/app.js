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

// Metrics Elements
const mPreIr = document.getElementById('m-pre-ir');
const mPostIr = document.getElementById('m-post-ir');
const mTime = document.getElementById('m-time');
const mMem = document.getElementById('m-mem');

// Visualization Data
let astNetwork = null;
let raNetwork = null;
let currentAstData = null;
let currentRaData = null;
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

// RA Animation (Step-by-step coloring)
async function animateRa(data) {
    if (!data || !data.nodes || data.nodes.length === 0) {
        log('No allocatable variables found. Try O0 or more complex code!', 'warning');
        return;
    }
    log('Starting Register Allocation Animation...', 'info');
    
    const nodes = new vis.DataSet(data.nodes.map(n => ({ 
        id: n.id, 
        label: n.name, 
        color: '#334155', // Uncolored initially
        font: { color: '#fff' }
    })));
    const edges = new vis.DataSet(data.edges);
    const container = document.getElementById('ra-viz');
    const options = {
        physics: { enabled: true, stabilization: true },
        nodes: { shape: 'dot', size: 20 }
    };
    
    raNetwork = new vis.Network(container, { nodes, edges }, options);
    
    const palette = ["#FAD7A0", "#D2B4DE", "#F1948A", "#A3E4D7", "#85C1E9", "#82E0AA", "#F8C471", "#F0B27A", "#C39BD3", "#7FB3D3"];
    
    // Simulate Pop and Color
    const stack = [...data.stack].reverse(); // Popping Order
    for (let nodeId of stack) {
        const nodeData = data.nodes.find(n => n.id === nodeId);
        await new Promise(r => setTimeout(r, 300));
        
        if (nodeData.spilled) {
            nodes.update({ id: nodeId, color: '#ef4444', label: `${nodeData.name}\n(SPILL)` });
        } else if (nodeData.color >= 0) {
            const color = palette[nodeData.color % palette.length];
            nodes.update({ id: nodeId, color: color, font: { color: '#000' } });
        }
    }
    log('Register Allocation coloring complete.', 'success');
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
            
            // Parse Metrics
            const metrics = parseMetrics(data.metrics);
            updateDashboard(metrics);
            
            currentAstData = data.ast_json;
            currentRaData = data.ra_json[0]; // Take first function
            
            // Static viz
            renderStaticViz(currentAstData, currentRaData);
        }
    } catch (e) {
        log('Connection error.', 'error');
    } finally {
        compileBtn.disabled = false;
    }
});

function parseMetrics(text) {
    const metrics = {};
    const lines = text.split('\n');
    lines.forEach(line => {
        if (line.includes('pre-opt')) metrics.preIr = parseInt(line.match(/\d+/));
        if (line.includes('post-opt')) metrics.postIr = parseInt(line.match(/\d+/));
        if (line.includes('Execution time')) metrics.time = parseFloat(line.match(/[\d\.]+/));
        if (line.includes('Peak memory')) metrics.mem = parseInt(line.match(/\d+/));
    });
    return metrics;
}

function updateDashboard(metrics) {
    if (metrics.preIr) mPreIr.textContent = metrics.preIr;
    if (metrics.postIr) mPostIr.textContent = metrics.postIr;
    if (metrics.time) mTime.textContent = metrics.time.toFixed(4);
    if (metrics.mem) mMem.textContent = metrics.mem;
    
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

runBtn.addEventListener('click', async () => {
    log('Running Benchmarks with QEMU...', 'info');
    runBtn.disabled = true;
    try {
        const response = await fetch('/api/run', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                code: editor.value,
                input: "" 
            })
        });
        const data = await response.json();
        log(data.stdout || 'Run complete');
        const metrics = parseMetrics(data.metrics);
        updateDashboard(metrics);
    } catch (e) {
        log('Run failed.', 'error');
    } finally {
        runBtn.disabled = false;
    }
});

// Init
initChart();
loadExamples();
log('System Ready.');

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
