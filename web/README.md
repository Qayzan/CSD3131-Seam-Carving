# Seam Carving Studio (interactive web demo)

A browser app that runs the team's C++ seam carving code, compiled to WebAssembly. It is the "App Demo" for the report and covers bonus items 7(a) and 7(b):

- **7(a) step by step, all seams visualised:** every seam is drawn in red (vertical) or blue (horizontal) just before it is removed. The *Removed seams* tab draws every removed seam on the original picture.
- **7(b) user interaction:** pick a picture (samples, or your own by drag and drop), set the target width and height with sliders, choose DP / greedy / graph cut, then play, pause, step one seam at a time, reset and download the result.
- **Comparison:** *Run comparison* resizes the picture once per algorithm with the shared `carve()` loop and shows the results side by side, with the total seam energy and the time spent.

Nothing in `web/` re-implements an algorithm. `bindings.cpp` only connects the page to `sobelEnergy`, `dp::findSeam`, `greedy::findSeam`, graph cut (as soon as KO's `graph_cut.hpp` or `seam_extraction.hpp` is on `main`), `validateSeam`, `removeSeam` and `carve()`. The browser's results are pixel-for-pixel identical to `seam_carver` (checked for DP and greedy, width-only and both directions).

| File | What it is |
|---|---|
| `bindings.cpp` | The `Carver` class the page drives one seam at a time, plus `compare()` (Emscripten embind) |
| `CMakeLists.txt` | Builds `seam_carving.js` (the WebAssembly is embedded in it) and copies `site/` next to it |
| `site/index.html`, `style.css`, `app.js` | The page: controls, drawing, comparison cards |
| `site/worker.js` | Runs the carver in a Web Worker so the page never freezes |
| `site/samples/` | Sample pictures (the tower photo is from Wikimedia Commons, CC BY-SA) |

## Build and run locally

You need the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) (CI uses version 4.0.10).

```bash
emcmake cmake -S web -B build-web
cmake --build build-web
python -m http.server -d build-web/site 8000      # then open http://localhost:8000
```

Open it through a web server, not by double-clicking `index.html`: browsers do not start Web Workers from `file://` pages.

You can also skip installing Emscripten: every CI run builds the site and attaches it as the artifact **seam-carving-web** (GitHub → Actions → the latest run → Artifacts).

## Put it online

The `build-web/site` folder is a plain static website (six files plus the samples).

- **Netlify by hand:** drag the folder (or the unzipped CI artifact) onto <https://app.netlify.com/drop>.
- **Netlify automatically (CD):** add the repository secrets `NETLIFY_AUTH_TOKEN` and `NETLIFY_SITE_ID` (GitHub → Settings → Secrets and variables → Actions). After that, every push to `main` that passes CI deploys the new version.

Graph cut solves a max-flow problem per seam, so it is only practical on small pictures; the page warns about this and the comparison skips it above 40,000 pixels.
