// worker.js -- runs the WebAssembly seam carver off the main thread, so the
// page stays responsive while seams are being found and removed.
//
// Messages in:  load {rgba, width, height}
//               run {plan: [0|1...], algorithm, delayMs, showSeams}   0 = vertical, 1 = horizontal
//               pause
//               view {kind: 'energy' | 'seams'}
//               compare {rgba, width, height, targetWidth, targetHeight, order, algorithms}
// Messages out: ready {algorithms}, frame {...}, done {...}, view {...},
//               compareResult {...}, compareDone, error {message}
/* global createSeamCarving */
importScripts('seam_carving.js');

let M = null;
let carver = null;
let runToken = 0;          // bumped to cancel a running loop
const ready = createSeamCarving().then((module) => {
    M = module;
    postMessage({ type: 'ready', algorithms: M.algorithms() });
});

// Yield to the event loop (so 'pause' can arrive) without the 4 ms setTimeout clamp.
const channel = new MessageChannel();
const waiting = [];
channel.port1.onmessage = () => waiting.shift()?.();
const yieldNow = () => new Promise((resolve) => { waiting.push(resolve); channel.port2.postMessage(0); });
const sleep = (ms) => (ms > 0 ? new Promise((r) => setTimeout(r, ms)) : yieldNow());

function postImage(type, rgba, width, height, extra = {}) {
    postMessage({ type, rgba, width, height, ...extra }, [rgba.buffer]);
}

function info() {
    return { seamsRemoved: carver.seamsRemoved(), width: carver.width(), height: carver.height() };
}

async function run({ plan, algorithm, delayMs, showSeams }) {
    const token = ++runToken;
    const stats = { energyMs: 0, findMs: 0, removeMs: 0, seamEnergy: 0, count: 0, lastEnergy: null };
    let lastFrame = 0;
    for (let i = 0; i < plan.length; i++) {
        if (token !== runToken) return;              // paused or restarted
        const found = carver.findSeam(plan[i], algorithm);
        if (!found.ok) { postMessage({ type: 'error', message: found.error }); return; }
        stats.energyMs += found.energyMs;
        stats.findMs += found.findMs;
        stats.seamEnergy += found.energy;
        stats.lastEnergy = found.energy;
        if (showSeams) {
            postImage('frame', carver.pendingRGBA(), carver.width(), carver.height(),
                      { pending: true, orientation: plan[i], stats: { ...stats }, ...info() });
            await sleep(delayMs);
            if (token !== runToken) return;
        }
        const removed = carver.removePending();
        if (!removed.ok) { postMessage({ type: 'error', message: removed.error }); return; }
        stats.removeMs += removed.removeMs;
        stats.count++;
        const now = performance.now();
        const last = i === plan.length - 1;
        if (showSeams || last || now - lastFrame > 40) {
            lastFrame = now;
            postImage('frame', carver.currentRGBA(), carver.width(), carver.height(),
                      { pending: false, stats: { ...stats }, ...info() });
        }
        await sleep(showSeams ? delayMs : 0);
    }
    if (token === runToken) postMessage({ type: 'done', ...info() });
}

onmessage = async ({ data }) => {
    await ready;
    try {
        switch (data.type) {
        case 'load':
            runToken++;
            if (carver) carver.delete();
            carver = new M.Carver(data.rgba, data.width, data.height);
            postImage('frame', carver.currentRGBA(), carver.width(), carver.height(),
                      { pending: false, stats: null, loaded: true, ...info() });
            break;
        case 'run':
            if (carver) run(data);
            break;
        case 'pause':
            runToken++;
            if (carver && carver.hasPending()) {
                // Finish the seam that is on screen, so the picture is never left half-done.
                carver.removePending();
                postImage('frame', carver.currentRGBA(), carver.width(), carver.height(), { pending: false, stats: null, ...info() });
            }
            postMessage({ type: 'paused' });
            break;
        case 'view':
            if (!carver) break;
            if (data.kind === 'energy') postImage('view', carver.energyRGBA(), carver.width(), carver.height(), { kind: 'energy' });
            else postImage('view', carver.seamsOverlayRGBA(), carver.originalWidth(), carver.originalHeight(), { kind: 'seams' });
            break;
        case 'compare':
            for (const algorithm of data.algorithms) {
                const r = M.compare(data.rgba, data.width, data.height, data.targetWidth, data.targetHeight, algorithm, data.order);
                if (r.ok) postImage('compareResult', r.rgba, r.width, r.height, {
                    algorithm, seams: r.seams, totalSeamEnergy: r.totalSeamEnergy,
                    findMs: r.findMs, energyMs: r.energyMs, removeMs: r.removeMs, totalMs: r.totalMs });
                else postMessage({ type: 'compareResult', algorithm, error: r.error });
                await yieldNow();
            }
            postMessage({ type: 'compareDone' });
            break;
        default:
            break;
        }
    } catch (e) {
        postMessage({ type: 'error', message: String(e && e.message ? e.message : e) });
    }
};
