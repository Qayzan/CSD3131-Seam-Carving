# Seam Carving Studio (interactive web demo)

A browser app that runs the team's C++ seam carving code, compiled to WebAssembly. It is the "App Demo" for the report and covers bonus items 7(a) and 7(b):

- **7(a) step by step, all seams visualised:** every seam is drawn in red (vertical) or blue (horizontal) just before it is removed. The *Removed seams* tab draws every removed seam on the original picture.
- **7(b) user interaction:** pick a picture (samples, or your own by drag and drop), set the target width and height with sliders, then play, pause, step one seam at a time, reset and download the result.
- **Comparison:** *Run comparison* resizes the picture once per method with the shared `carve()` loop and shows the results side by side, with the total seam energy and the time spent. It fills up as methods are added.

Nothing in `web/` re-implements an algorithm. `bindings.cpp` only connects the page to `sobelEnergy`, `dp::findSeam`, `validateSeam`, `removeSeam` and `carve()`. The browser's results are pixel-for-pixel identical to `seam_carver` (checked for DP, width-only and both directions).

## Adding your method (HM, KO)

The demo is the shared base plus the DP. Greedy and graph cut are plugged in by their owners:

1. In `web/bindings.cpp`, include your header next to `dynamic_programming.hpp`.
2. Uncomment (or add) your line in `algorithmList()`, for example
   `a.push_back({"greedy", "Greedy", "Cheapest neighbour row by row", greedy::findSeam});`
3. Rebuild and open the page. Your method shows up in *Method*, plays seam by seam and joins the comparison. Your finder must follow the contract in `docs/interfaces.md` (`findSeam(const EnergyMap&, Orientation)`).

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

Graph cut solves a max-flow problem per seam, so once it is added it is only practical on small pictures. The page warns about this and the comparison skips it above 40,000 pixels.
