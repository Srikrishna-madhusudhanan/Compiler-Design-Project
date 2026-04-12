const express = require('express');
const cors = require('cors');
const bodyParser = require('body-parser');
const { exec } = require('child_process');
const path = require('path');
const fs = require('fs');

const app = express();
const port = 3000;

app.use(cors());
app.use(bodyParser.json());
app.use(express.static(path.join(__dirname, 'public')));
app.use('/lib', express.static(path.join(__dirname, 'node_modules')));

const COMPILER_PATH = './build/parser';
const ROOT_DIR = path.join(__dirname, '..');

app.post('/api/compile', (req, res) => {
    const { code, optimizationLevel, useMetrics } = req.body;
    const tempFile = path.join(__dirname, 'temp_input.c');
    
    fs.writeFileSync(tempFile, code);
    
    // Cleanup old interference files
    const oldFiles = fs.readdirSync(ROOT_DIR);
    oldFiles.forEach(f => {
        if (f.endsWith('_interference.json')) fs.unlinkSync(path.join(ROOT_DIR, f));
    });

    let command = `${COMPILER_PATH} -O${optimizationLevel} ${useMetrics ? '--metrics' : ''} ${tempFile}`;
    
    exec(command, { cwd: ROOT_DIR }, (error, stdout, stderr) => {
        // Cleanup temp file
        if (fs.existsSync(tempFile)) fs.unlinkSync(tempFile);
        
        let result = {
            stdout,
            stderr,
            error: error ? error.message : null,
            ir: '',
            ir_opt: '',
            metrics: '',
            ast_json: null,
            ra_json: []
        };
        
        const irFile = path.join(ROOT_DIR, 'ir.txt');
        if (fs.existsSync(irFile)) result.ir = fs.readFileSync(irFile, 'utf8');
        
        const irOptFile = path.join(ROOT_DIR, 'ir_opt.txt');
        if (fs.existsSync(irOptFile)) result.ir_opt = fs.readFileSync(irOptFile, 'utf8');
        
        const metricsFile = path.join(ROOT_DIR, 'compiler_metrics.txt');
        if (fs.existsSync(metricsFile)) result.metrics = fs.readFileSync(metricsFile, 'utf8');
        
        const astJsonFile = path.join(ROOT_DIR, 'ast.json');
        if (fs.existsSync(astJsonFile)) {
            try {
                result.ast_json = JSON.parse(fs.readFileSync(astJsonFile, 'utf8'));
            } catch (e) {}
        }
        
        // Look for all _interference.json files
        const files = fs.readdirSync(ROOT_DIR);
        files.forEach(file => {
            if (file.endsWith('_interference.json')) {
                try {
                    result.ra_json.push(JSON.parse(fs.readFileSync(path.join(ROOT_DIR, file), 'utf8')));
                } catch (e) {}
            }
        });
        
        res.json(result);
    });
});

app.post('/api/run', (req, res) => {
    const { input } = req.body;
    // We assume the last compilation was successful and output.s exists
    const qemuRunScript = path.join(ROOT_DIR, 'scripts', 'qemu_run.sh');
    
    // We need a test file to "run", but the parser doesn't take input the same way as qemu_run.sh
    // qemu_run.sh takes a source file and compiles it first.
    // To be efficient, we might want to just run the generated output.s
    // But qemu_run.sh is easier to reuse.
    
    // For simplicity, we'll re-run with qemu_run.sh using a saved version of the current code
    const tempFile = path.join(__dirname, 'temp_input_run.c');
    fs.writeFileSync(tempFile, req.body.code);
    
    let command = `${qemuRunScript} --metrics ${tempFile} "${input || ''}"`;
    
    exec(command, { cwd: ROOT_DIR }, (error, stdout, stderr) => {
        if (fs.existsSync(tempFile)) fs.unlinkSync(tempFile);
        
        let result = {
            stdout,
            stderr,
            error: error ? error.message : null,
            metrics: ''
        };
        
        const metricsFile = path.join(ROOT_DIR, 'compiler_metrics.txt');
        if (fs.existsSync(metricsFile)) result.metrics = fs.readFileSync(metricsFile, 'utf8');
        
        res.json(result);
    });
});

app.get('/api/examples', (req, res) => {
    const searchDir = (dir) => {
        let results = [];
        const list = fs.readdirSync(dir);
        list.forEach(file => {
            const fullPath = path.join(dir, file);
            const stat = fs.statSync(fullPath);
            if (stat && stat.isDirectory()) {
                results = results.concat(searchDir(fullPath));
            } else if (file.endsWith('.c')) {
                results.push({
                    name: path.relative(path.join(ROOT_DIR, 'test'), fullPath),
                    content: fs.readFileSync(fullPath, 'utf8')
                });
            }
        });
        return results;
    };
    
    try {
        const examples = searchDir(path.join(ROOT_DIR, 'test'));
        res.json(examples);
    } catch (e) {
        res.json([]);
    }
});

app.listen(port, () => {
    console.log(`PaniniC GUI server running at http://localhost:${port}`);
});
