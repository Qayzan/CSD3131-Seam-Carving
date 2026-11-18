// app.js -- the page: picture loading, controls, drawing. All seam carving
// happens in worker.js (the team's C++ code compiled to WebAssembly).
'use strict';

const $ = (id) => document.getElementById(id);
const worker = new Worker('worker.js');

const state = {
    algorithms: [],
    algorithm: null,
    original: null,          // ImageData at working size
    current: null,           // ImageData of the latest finished picture
    width: 0, height: 0,     // current picture size
    seamsRemoved: 0,
    view: 'result',
    running: false,
    base: { energyMs: 0, findMs: 0, removeMs: 0, count: 0 },  // totals from earlier runs
    run: null,               // stats of the run in progress
    planLength: 0,
    planStartSeams: 0,
};

const view = $('view');
const ctx = view.getContext('2d');

// ---------------------------------------------------------------- helpers
const fmtMs = (ms) => (ms >= 1000 ? (ms / 1000).toFixed(2) + ' s' : ms >= 10 ? ms.toFixed(0) + ' ms' : ms.toFixed(2) + ' ms');
// Same, as HTML with the unit set small and muted (for the readout)
const unitHTML = (text) => text.replace(/ (ms|s|k|M)$/, '<small>$1</small>');
const fmtNum = (x) => (x >= 1e6 ? (x / 1e6).toFixed(2) + ' M' : x >= 1e4 ? (x / 1e3).toFixed(1) + ' k' : x.toFixed(0));

function setStatus(text, isError = false) {
    $('status').textContent = text;
    $('status').classList.toggle('err', isError);
}

function setRunning(on) {
    state.running = on;
    $('playLabel').textContent = on ? 'Pause' : 'Carve';
    $('playIcon').setAttribute('href', on ? '#i-pause' : '#i-play');
    for (const id of ['step', 'reset', 'sample', 'file', 'workSize', 'compareBtn', 'order'])
        $(id).disabled = on;
    for (const el of document.querySelectorAll('#algos input, .preset')) el.disabled = on;
}

function toImageData(rgba, width, height) {
    return new ImageData(rgba, width, height);
}

// ---------------------------------------------------------------- loading pictures
async function loadBitmap(bitmap) {
    const limit = Number($('workSize').value);
    let w = bitmap.width, h = bitmap.height;
    if (limit > 0 && Math.max(w, h) > limit) {
        const s = limit / Math.max(w, h);
        w = Math.max(1, Math.round(w * s));
        h = Math.max(1, Math.round(h * s));
    }
    if (w * h > 4_000_000) {
        setStatus('That picture is very large, choose a smaller working size', true);
        return;
    }
    const c = new OffscreenCanvas(w, h);
    const g = c.getContext('2d');
    g.imageSmoothingQuality = 'high';
    g.drawImage(bitmap, 0, 0, w, h);
    state.original = g.getImageData(0, 0, w, h);
    view.width = w;
    view.height = h;
    $('targetW').max = w;
    $('targetH').max = h;
    $('targetW').value = Math.max(1, Math.round(w * 0.7));
    $('targetH').value = h;
    state.width = 0; state.height = 0;
    updateTargetLabels();
    updateHeading();
    $('picInfo').textContent = w === bitmap.width && h === bitmap.height ? `${w} × ${h} px` : `${bitmap.width} × ${bitmap.height} → ${w} × ${h} px`;
    resetPicture();
}

function resetPicture() {
    state.base = { energyMs: 0, findMs: 0, removeMs: 0, count: 0 };
    state.run = null;
    $('cards').innerHTML = '';
    const copy = new Uint8ClampedArray(state.original.data);
    worker.postMessage({ type: 'load', rgba: copy, width: state.original.width, height: state.original.height }, [copy.buffer]);
}

async function loadSample() {
    setStatus('Loading picture');
    try {
        const blob = await (await fetch($('sample').value)).blob();
        $('picTitle').textContent = $('sample').selectedOptions[0].textContent;
        await loadBitmap(await createImageBitmap(blob));
    } catch (e) {
        setStatus('Could not load the sample picture: ' + e.message, true);
    }
}

async function loadFile(file) {
    if (!file || !file.type.startsWith('image/')) { setStatus('Choose an image file', true); return; }
    try {
        $('picTitle').textContent = file.name.replace(/\.[^.]+$/, '');
        await loadBitmap(await createImageBitmap(file));
    } catch (e) {
        setStatus('Could not read that picture: ' + e.message, true);
    }
}

// ---------------------------------------------------------------- plan + run
function removalPlan(cols, rows, order) {
    const plan = [];
    if (order === 0) { for (let i = 0; i < cols; i++) plan.push(0); for (let i = 0; i < rows; i++) plan.push(1); }
    else if (order === 1) { for (let i = 0; i < rows; i++) plan.push(1); for (let i = 0; i < cols; i++) plan.push(0); }
    else for (let k = 0; k < cols || k < rows; k++) { if (k < cols) plan.push(0); if (k < rows) plan.push(1); }
    return plan;
}

let pendingRun = null;   // run to start once a reset has finished loading

function startRun(singleSeam = false) {
    if (!state.original || !state.algorithm) return;
    const tw = Number($('targetW').value), th = Number($('targetH').value);
    if (tw > state.width || th > state.height) {
        // Only shrinking is possible, so start again from the original picture.
        pendingRun = { singleSeam };
        setStatus('Starting again from the original picture');
        resetPicture();
        return;
    }
    let plan = removalPlan(state.width - tw, state.height - th, Number($('order').value));
    if (singleSeam) plan = plan.slice(0, 1);
    if (plan.length === 0) { setStatus('Already at the target size'); return; }
    if (state.algorithm === 'graphcut' && state.width * state.height > 40000)
        setStatus('Graph cut solves a max-flow per seam, so it is slow on big pictures. Try the 480 px working size');
    else setStatus(singleSeam ? 'Removing one seam' : `Removing ${plan.length} seam${plan.length > 1 ? 's' : ''}`);
    const speed = Number($('speed').value);
    state.planLength = plan.length;
    state.planStartSeams = state.seamsRemoved;
    state.run = null;
    setRunning(!singleSeam);
    worker.postMessage({
        type: 'run', plan, algorithm: state.algorithm,
        showSeams: singleSeam || speed >= 0,
        delayMs: singleSeam ? 450 : Math.max(0, speed),
    });
}

function finishRun() {
    if (state.run) {
        state.base.energyMs += state.run.energyMs;
        state.base.findMs += state.run.findMs;
        state.base.removeMs += state.run.removeMs;
        state.base.count += state.run.count;
        state.run = null;
    }
    setRunning(false);
    updateStats();
    refreshView();
}

// ---------------------------------------------------------------- drawing
function drawOnView(imageData) {
    ctx.clearRect(0, 0, view.width, view.height);
    ctx.putImageData(imageData, 0, 0);
}

function drawBeforeAfter() {
    const before = $('beforeCanvas'), after = $('afterCanvas');
    before.width = state.original.width; before.height = state.original.height;
    before.getContext('2d').putImageData(state.original, 0, 0);
    after.width = state.current.width; after.height = state.current.height;
    after.getContext('2d').putImageData(state.current, 0, 0);
    // Same scale for both, so the result really looks narrower / shorter.
    after.style.width = (100 * state.current.width / state.original.width) + '%';
    $('afterCaption').textContent = `Result, ${state.current.width} × ${state.current.height}`;
}

function setLegend() {
    const L = {
        result: '<span class="li"><i style="background:var(--cut)"></i>Next vertical seam, just before it is cut</span><span class="li"><i style="background:var(--cut-h)"></i>Next horizontal seam</span>',
        energy: 'Energy of the current picture. Bright pixels are edges and stay, dark pixels are flat and go first',
        seams: '<span class="li"><i style="background:var(--cut)"></i>Removed by vertical seams</span><span class="li"><i style="background:var(--cut-h)"></i>Removed by horizontal seams</span>',
        compare: 'Original and result at the same scale',
    };
    $('legend').innerHTML = L[state.view];
}

function refreshView() {
    if (!state.current) return;
    const ba = state.view === 'compare';
    $('sheet').hidden = ba;
    $('beforeAfter').hidden = !ba;
    setLegend();
    if (state.view === 'result') drawOnView(state.current);
    else if (state.view === 'compare') drawBeforeAfter();
    else worker.postMessage({ type: 'view', kind: state.view });
}

// ---------------------------------------------------------------- rulers
const SVG = 'http://www.w3.org/2000/svg';
function svgEl(tag, attrs, text) {
    const e = document.createElementNS(SVG, tag);
    for (const k in attrs) e.setAttribute(k, attrs[k]);
    if (text != null) e.textContent = text;
    return e;
}
// A ruler along one edge of the picture, in picture pixels: ticks, the
// current size (red cut line) and the target size (dashed).
function drawRuler(svg, horizontal, total, current, target) {
    svg.replaceChildren();
    const len = horizontal ? svg.clientWidth : svg.clientHeight;
    const depth = horizontal ? svg.clientHeight : svg.clientWidth;
    if (!len || !total) return;
    const scale = len / total;
    const steps = [10, 20, 25, 50, 100, 200, 250, 500, 1000];
    const minor = steps.find((s) => s * scale >= 6) || 1000;
    const major = steps.find((s) => s * scale >= 56 && s % minor === 0) || minor * 10;
    const line = (pos, from, cls) => svg.append(svgEl('line', horizontal
        ? { x1: pos, x2: pos, y1: depth - from, y2: depth, class: cls }
        : { y1: pos, y2: pos, x1: depth - from, x2: depth, class: cls }));
    for (let v = 0; v <= total; v += minor) {
        const pos = Math.round(v * scale) + 0.5;
        const isMajor = v % major === 0;
        line(pos, isMajor ? 8 : 4, isMajor ? 'tick' : 'tick minor');
        if (isMajor && v > 0 && v < total - major * 0.4) {
            svg.append(svgEl('text', horizontal
                ? { x: pos + 3, y: depth - 9 }
                : { x: depth - 9, y: pos + 3, transform: `rotate(-90 ${depth - 9} ${pos + 3})` }, String(v)));
        }
    }
    if (target < total) line(Math.round(target * scale) + 0.5, depth, 'goal');
    if (current < total) {
        const pos = Math.round(current * scale);
        line(pos, depth, 'now');
        const label = svgEl('text', horizontal ? { x: pos + 4, y: 9, class: 'nowLabel' }
                                               : { x: 9, y: pos - 4, class: 'nowLabel', transform: `rotate(-90 9 ${pos - 4})` }, String(current));
        svg.append(label);
    }
}
function drawRulers() {
    if (!state.original) return;
    drawRuler($('rulerTop'), true, state.original.width, state.width || state.original.width, Number($('targetW').value));
    drawRuler($('rulerLeft'), false, state.original.height, state.height || state.original.height, Number($('targetH').value));
    const max = state.original.width + 24;
    $('sheet').style.maxWidth = max + 'px';
}
window.addEventListener('resize', drawRulers);

function setFill(input) {
    const min = Number(input.min) || 0, max = Number(input.max) || 1;
    input.style.setProperty('--fill', (100 * (Number(input.value) - min) / Math.max(1, max - min)) + '%');
}

function updateHeading() {
    if (!state.original) return;
    const o = state.original;
    $('picSize').textContent = state.width && (state.width !== o.width || state.height !== o.height)
        ? `${o.width} × ${o.height} → ${state.width} × ${state.height}` : `${o.width} × ${o.height}`;
}

function updateTargetLabels() {
    setFill($('targetW')); setFill($('targetH'));
    drawRulers();
    $('wOut').textContent = $('targetW').value + ' px';
    $('hOut').textContent = $('targetH').value + ' px';
}

function updateStats() {
    const r = state.run || { energyMs: 0, findMs: 0, removeMs: 0, count: 0 };
    const count = state.base.count + r.count;
    const find = state.base.findMs + r.findMs, energy = state.base.energyMs + r.energyMs, remove = state.base.removeMs + r.removeMs;
    $('sSize').innerHTML = state.width ? `${state.width}<small class="x">×</small>${state.height}` : '–';
    $('sSeams').textContent = String(state.seamsRemoved);
    if (state.run && state.run.lastEnergy != null) $('sLast').innerHTML = unitHTML(fmtNum(state.run.lastEnergy));
    $('sFind').innerHTML = count ? unitHTML(fmtMs(find / count)) : '–';
    $('sEnergy').innerHTML = count ? unitHTML(fmtMs(energy / count)) : '–';
    $('sTotal').innerHTML = count ? unitHTML(fmtMs(find + energy + remove)) : '–';
    const done = state.seamsRemoved - state.planStartSeams;
    $('bar').style.transform = `scaleX(${state.planLength ? Math.min(1, done / state.planLength) : 0})`;
}

// ---------------------------------------------------------------- worker messages
worker.onmessage = ({ data }) => {
    switch (data.type) {
    case 'ready':
        state.algorithms = data.algorithms;
        buildAlgorithmList();
        if (state.algorithms.length < 2)
            $('compareNote').textContent = 'Only dynamic programming is in this build so far. Greedy and graph cut appear here once their owners add them';
        loadSample();
        break;
    case 'frame': {
        const img = toImageData(data.rgba, data.width, data.height);
        state.width = data.width;
        state.height = data.height;
        drawRulers();
        updateHeading();
        state.seamsRemoved = data.seamsRemoved;
        if (data.stats) state.run = data.stats;
        if (!data.pending) state.current = img;
        if (state.view === 'result') drawOnView(img);
        else if (!data.pending && state.view === 'compare') drawBeforeAfter();
        updateStats();
        if (data.loaded) {
            $('sLast').textContent = '–';
            state.planLength = 0;
            updateStats();
            refreshView();
            if (pendingRun) { const p = pendingRun; pendingRun = null; startRun(p.singleSeam); }
            else setStatus('Ready');
        }
        break;
    }
    case 'done':
        finishRun();
        setStatus(`Done, ${state.width} × ${state.height} after ${state.seamsRemoved} seams`);
        break;
    case 'paused':
        finishRun();
        setStatus('Paused');
        break;
    case 'view':
        if (data.kind === state.view) drawOnView(toImageData(data.rgba, data.width, data.height));
        break;
    case 'compareResult':
        addCompareCard(data);
        break;
    case 'compareDone':
        markBest();
        $('compareBtn').disabled = false;
        setStatus('Comparison finished');
        break;
    case 'error':
        finishRun();
        setStatus('Error: ' + data.message, true);
        break;
    default:
        break;
    }
};
worker.onerror = (e) => setStatus('The seam carver failed to start: ' + (e.message || 'unknown error'), true);

// ---------------------------------------------------------------- algorithm list
function buildAlgorithmList() {
    const box = $('algos');
    box.innerHTML = '';
    if (state.algorithms.length === 0) { box.textContent = 'No method compiled in'; return; }
    state.algorithm = state.algorithms[0].id;
    for (const a of state.algorithms) {
        const label = document.createElement('label');
        label.className = 'method';
        label.innerHTML = `<input type="radio" name="algo" value="${a.id}"><strong></strong><small></small>`;
        label.querySelector('strong').textContent = a.label;
        label.querySelector('small').textContent = a.note;
        const input = label.querySelector('input');
        input.checked = a.id === state.algorithm;
        input.addEventListener('change', () => { state.algorithm = a.id; markAlgo(); });
        box.appendChild(label);
    }
    markAlgo();
}
function markAlgo() {
    for (const l of document.querySelectorAll('.method'))
        l.classList.toggle('on', l.querySelector('input').checked);
}

// ---------------------------------------------------------------- comparison
function runComparison() {
    if (!state.original) return;
    const tw = Number($('targetW').value), th = Number($('targetH').value);
    if (tw === state.original.width && th === state.original.height) { setStatus('Set a target smaller than the picture first'); return; }
    const pixels = state.original.width * state.original.height;
    const algos = state.algorithms.map((a) => a.id).filter((id) => id !== 'graphcut' || pixels <= 40000);
    $('cards').innerHTML = '';
    for (const a of state.algorithms)
        if (!algos.includes(a.id)) addCompareCard({ algorithm: a.id, error: 'Skipped, too slow at this size. Use the 480 px working size and a small picture' });
    $('compareBtn').disabled = true;
    setStatus('Comparing');
    const copy = new Uint8ClampedArray(state.original.data);
    worker.postMessage({ type: 'compare', rgba: copy, width: state.original.width, height: state.original.height,
                         targetWidth: tw, targetHeight: th, order: Number($('order').value), algorithms: algos }, [copy.buffer]);
}

function addCompareCard(d) {
    const a = state.algorithms.find((x) => x.id === d.algorithm);
    const card = document.createElement('article');
    card.className = 'result';
    card.dataset.energy = d.error ? '' : String(d.totalSeamEnergy);
    const title = document.createElement('h3');
    title.textContent = a ? a.label : d.algorithm;
    if (d.error) {
        card.append(title);
        const p = document.createElement('p'); p.className = 'skip'; p.textContent = d.error; card.append(p);
    } else {
        const c = document.createElement('canvas');
        c.width = d.width; c.height = d.height;
        c.getContext('2d').putImageData(toImageData(d.rgba, d.width, d.height), 0, 0);
        const dl = document.createElement('dl');
        dl.innerHTML = `<dt>Total seam energy</dt><dd class="e"></dd><dt>Finding seams</dt><dd></dd><dt>Energy maps</dt><dd></dd><dt>Whole resize</dt><dd></dd>`;
        const dd = dl.querySelectorAll('dd');
        dd[0].innerHTML = unitHTML(fmtNum(d.totalSeamEnergy));
        dd[1].innerHTML = unitHTML(fmtMs(d.findMs));
        dd[2].innerHTML = unitHTML(fmtMs(d.energyMs));
        dd[3].innerHTML = unitHTML(fmtMs(d.totalMs));
        card.append(c, title, dl);
    }
    $('cards').append(card);
}

function markBest() {
    const cards = [...document.querySelectorAll('.result')].filter((c) => c.dataset.energy !== '');
    if (cards.length < 2) return;
    const best = Math.min(...cards.map((c) => Number(c.dataset.energy)));
    for (const c of cards) {
        if (Math.abs(Number(c.dataset.energy) - best) <= 1e-6 * Math.max(1, best)) {
            const b = document.createElement('small'); b.textContent = 'lowest energy';
            c.querySelector('h3').append(b);
        }
    }
}

// ---------------------------------------------------------------- events
$('play').addEventListener('click', () => {
    if (state.running) worker.postMessage({ type: 'pause' });
    else startRun(false);
});
$('step').addEventListener('click', () => startRun(true));
$('reset').addEventListener('click', () => { if (state.original) { pendingRun = null; resetPicture(); } });
$('sample').addEventListener('change', loadSample);
$('workSize').addEventListener('change', () => {
    const f = $('file').files[0];
    if (f) loadFile(f); else loadSample();
});
$('file').addEventListener('change', (e) => loadFile(e.target.files[0]));
for (const id of ['targetW', 'targetH']) $(id).addEventListener('input', updateTargetLabels);
for (const chip of document.querySelectorAll('.preset')) {
    chip.addEventListener('click', () => {
        if (!state.original) return;
        $('targetW').value = Math.max(1, Math.round(state.original.width * Number(chip.dataset.w)));
        $('targetH').value = Math.max(1, Math.round(state.original.height * Number(chip.dataset.h)));
        updateTargetLabels();
    });
}
for (const tab of document.querySelectorAll('.tab')) {
    tab.addEventListener('click', () => {
        for (const t of document.querySelectorAll('.tab')) t.setAttribute('aria-selected', String(t === tab));
        state.view = tab.dataset.view;
        refreshView();
    });
}
$('compareBtn').addEventListener('click', runComparison);
$('download').addEventListener('click', () => {
    if (!state.current) return;
    const c = document.createElement('canvas');
    c.width = state.current.width; c.height = state.current.height;
    c.getContext('2d').putImageData(state.current, 0, 0);
    c.toBlob((blob) => {
        const a = document.createElement('a');
        a.href = URL.createObjectURL(blob);
        a.download = `carved_${c.width}x${c.height}.png`;
        a.click();
        setTimeout(() => URL.revokeObjectURL(a.href), 2000);
    }, 'image/png');
});
const drop = $('drop');
for (const ev of ['dragenter', 'dragover']) drop.addEventListener(ev, (e) => { e.preventDefault(); drop.classList.add('over'); });
for (const ev of ['dragleave', 'drop']) drop.addEventListener(ev, (e) => { e.preventDefault(); drop.classList.remove('over'); });
drop.addEventListener('drop', (e) => loadFile(e.dataTransfer.files[0]));
