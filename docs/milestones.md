# Milestone log (Question 1 + repository)

Each milestone ends with: files changed, features, tests actually run, known issues, requirements covered and next steps.

## Milestone 1 — Repository and requirements (2026-10-27)

**Files created:** `README.md`, `CONTRIBUTING.md`, `pyproject.toml`, `requirements.txt`, `requirements-dev.txt`, `.gitignore`, `.gitattributes`, `.editorconfig`, `.github/` (CI workflow, PR template, issue templates, CODEOWNERS template), `docs/` (`requirements.md`, `team.md`, `algorithm_design.md`, `interfaces.md`, `git_workflow.md`, `ai_usage_log.md`, `milestones.md`, owner placeholders for theory/analysis/report/presentation), `src/seam_carving/` (documented placeholders, no algorithms), `tests/test_package.py`, READMEs for `examples/`, `benchmarks/` and `assets/images/`.

**Features:** installable package (`pip install -e ".[dev]"`); lint/format/test tooling; CI on Python 3.10 and 3.12; collaboration rules; requirement checklist mapped to the rubric; Question 1 DP design with a brute-force-verified worked example; draft shared interface contract.

**Tests run:**
- Python 3.13 (set-up workspace, fresh venv): `ruff check .` → all checks passed; `ruff format --check .` → 28 files formatted; `pytest` → **10 passed**.
- Python 3.10.12 (Linux VM on FF's laptop, fresh venv): `ruff check .` → passed; `ruff format --check .` → passed; `pytest` → **10 passed**.
- `ci.yml` parses as valid YAML. Its first run on GitHub (triggered by the push of `main`) has not been checked yet.

**Known issues / pending:**
- ~~Git not initialised and nothing published yet.~~ Done 2026-10-27: private repository `Qayzan/CSD3131-Seam-Carving` created, `main` pushed (commit `dd8d114`, 38 files), five teammates invited. Invitations not yet accepted.
- First CI run on GitHub not yet checked (Actions tab).
- Team names, SIT IDs and team id T11 copied from Assignment 1. Needs confirmation.
- Branch protection on a private personal repository needs GitHub Pro (Student Pack). See `docs/git_workflow.md` §4.
- Design decisions awaiting team review: energy function, tie-breaking, removal order, interface signature.

**Requirements covered:** R7c (CI part of the bonus), groundwork for R1a–R1c, R9a (AI log, team table).

**Next:** Milestone 2. Implement `image_utils`, `energy`, `interfaces`, `seam_ops` with tests (including a NumPy reference that cross-checks the Sobel energy).

## Milestone 2 — Shared utilities with tests (2026-10-27, branch `feature/ff-dynamic-programming`)

**Files changed:** `src/seam_carving/interfaces.py`, `image_utils.py`, `energy.py`, `seam_ops.py` (placeholders replaced by implementations), `src/seam_carving/__init__.py` (docstring); new tests `tests/test_interfaces.py`, `test_image_utils.py`, `test_energy.py`, `test_seam_ops.py`, plus `tests/conftest.py` (seeded `rng` fixture) and `tests/helpers.py` (worked-example energy matrix, random valid seam generator); `docs/interfaces.md` (now v1, implemented), `README.md` (status), this log, `docs/ai_usage_log.md`.

**Features:**
- `interfaces`: `Orientation`; `Seam` (frozen; indices copied to read-only `int64`, malformed indices rejected at construction, value equality, `len()`, `coordinates()`); `SeamFinder` protocol (runtime-checkable); `CarveResult`.
- `image_utils`: `validate_image`; `to_grayscale` (BT.601 weights in float64, no rounding); `load_image` (BGR→RGB, alpha dropped, 16-bit scaled to 8-bit, greyscale stays 2-D) and `save_image` (RGB→BGR, creates folders). Both go through `np.fromfile`/`imdecode` and `imencode`/`tofile` so non-ASCII Windows/OneDrive paths work.
- `energy`: `gradient_energy` = |Sobel x| + |Sobel y| on float64 greyscale, 3×3, `BORDER_REFLECT_101`; `gradient_energy_reference`, a plain-NumPy version used as an independent cross-check.
- `seam_ops`: `validate_seam` (length, range, 8-connectivity, optional energy check with clear error messages), `seam_energy`, `seam_mask`, `remove_seam` (one boolean-mask pass; horizontal via transpose; dtype/channels/pixel order kept; refuses to remove the last column/row), `highlight_seam`.

**Tests run** (Python 3.10.12, NumPy 2.2.6, OpenCV 5.0.0, ruff 0.17.3; Linux VM on FF's laptop, fresh venv):
- `ruff check .` → all checks passed; `ruff format --check .` → 35 files already formatted.
- `pytest` → **120 passed** (test_energy 28, test_image_utils 28, test_interfaces 17, test_package 10, test_seam_ops 37).
- Highlights: OpenCV energy equals the NumPy reference (max difference ≤ 1e-9) on random RGB and greyscale images of 7 shapes from 1×1 to 17×23; a hand-computed step-edge energy; no `uint8` wrap-around; removal checked against a slow `np.delete` reference on 20 random valid seams for each of 11 shape/orientation cases (incl. RGB, 1-row and 2-column images); PNG round trips incl. a non-ASCII path.
- Mutation check (done by hand, not part of the suite): six deliberate bugs were each caught by at least one test — zero-padding border (16 failures), Sobel kernel size 5 (13), BGR luma weights (2), allowing jumps of 2 in `validate_seam` (2), skipping BGR→RGB on load (1), removing everything except the seam (16).
- Not yet run: CI on GitHub for this branch (runs when the PR is opened), Python 3.12 locally.

**Known issues / pending:**
- The contract (§5 of `docs/interfaces.md`) still needs HM's and WN's review; their invitations were not accepted at the time of writing.
- `load_image` keeps 8-bit greyscale files 2-D, while `IMREAD_COLOR` would have made them 3-channel. All shared code accepts both, but finders should only rely on the energy map.

**Requirements covered:** R1a (energy map, Sobel) ✅; R1c groundwork (seam removal and validation; the repeat-until-target loop is Milestone 3); R7c (more tests running in CI).

**Next:** Milestone 3. `dynamic_programming.py` (cumulative table M, int8 back-pointers, leftmost tie-break, horizontal via transpose) and `carver.carve` (energy recomputed before every seam, width first, timings and optional frames), starting with the 4×5 worked example as the first test.

## Milestone 3 — Question 1: DP seam finder and the carving loop (2026-10-27, branch `feature/ff-dynamic-programming`)

**Files changed:** `src/seam_carving/dynamic_programming.py`, `src/seam_carving/carver.py` (placeholders replaced), `src/seam_carving/interfaces.py` and `__init__.py` (docstrings).

**Features:**
- `dynamic_programming`: `cumulative_energy` (full table M + `int8` back-pointers, for figures and tests); `backtrack`; `find_vertical_seam`, which keeps only the previous row of M plus the back-pointer table (design §2.10 option 2); `find_horizontal_seam` via the transposed energy map; `find_seam(energy, orientation)` as the shared entry point. Each row is computed in one vectorised step; ties go to the leftmost predecessor and the leftmost bottom cell. Energy maps that are not 2-D, empty, non-numeric or contain NaN/∞ are rejected.
- `carver`: `carve(image, target_height, target_width, finder=None, order=..., keep_frames=..., energy_fn=...)` — recomputes the energy before every seam, hands it to the finder read-only, checks every returned seam (validity, orientation and reported energy), times energy / finder / removal separately, and optionally keeps every frame. Orders: width first (default), height first, alternate. `seams_in_original_coordinates` maps all removed seams back onto the original picture. Shrinking only.

**Tests run:** see Milestone 4 (written and run together).

**Known issues:** enlargement (seam insertion) and the optimal removal order (Avidan & Shamir's transport map) are out of scope (design §2.8).

## Milestone 4 — Tests, including a brute-force oracle (2026-10-27)

**Files changed:** `tests/helpers.py` (oracles), new `tests/test_dynamic_programming.py`, `tests/test_carver.py`, `tests/test_integration.py`.

**What is tested:**
- **Worked example** (design §2.5): exact table M, the tie at row 1 column 2, seam `[1, 1, 2, 3]`, energy 7, and that the 4×5 grid has exactly 95 seams.
- **Exhaustive-search oracle** (`brute_force_find_seam`, tries every seam): 150 random integer matrices up to 6×6 (values 0–5, so ties are common) must give *exactly* the same seam in both orientations — this also proves the leftmost tie rule; 60 random real-valued matrices must give the same minimum energy.
- **Plain-loop reference** of the recurrence: identical M and back-pointer tables on 6 shapes; the memory-saving finder agrees with the full table on 5 shapes.
- **Edge cases:** uniform energy, a wall with one gap, seams hugging the left/right border, a diagonal valley, an unreachable cheap pixel, 1×1 / 1×n / n×1 maps, non-square maps in both orientations, negative and integer energies, invalid maps.
- **Contract:** `find_seam` is a `SeamFinder`, does not modify its input, works on read-only input, reports the right energy, is deterministic.
- **Carver:** exact target sizes for all three orders, greyscale and RGB, invalid targets, removal order, a spy proving the energy is recomputed on the current image before every seam, any finder can be plugged in (a "remove column 0" finder must equal a crop), broken finders (out-of-range seam, wrong energy, wrong orientation, writing to the energy map) are caught, timings and frames, and the original-coordinate mapping.
- **End to end:** DP and the exhaustive oracle carve **identical images**, seam by seam, through a full repeated resize (12 random images); every seam of a real carve is optimal for the energy at that step; a flat background with a high-contrast object loses only background pixels when the width or height shrinks by 25–30%; PNG round trip.

**Tests run** (Python 3.10.12, NumPy 2.2.6, OpenCV 5.0.0, ruff 0.17.3; Linux VM on FF's laptop): `ruff check .` and `ruff format --check .` pass; `pytest` → **436 passed** in about 2 s (test_dynamic_programming 252, test_carver 44, test_seam_ops 37, test_energy 28, test_image_utils 28, test_interfaces 17, test_integration 16, test_package 10, test_examples 4 — the last file is Milestone 5).

**Mutation check** (by hand, not part of the suite): each of these deliberate bugs made tests fail — left border not +∞ (261 failures), right border 0 instead of +∞ (258), ties to the rightmost predecessor (70), backtracking with the wrong sign (188), rightmost bottom cell (53), horizontal seams without transposing (242), carving the old energy map instead of recomputing it (1, the spy test).

**Speed** (same machine, indicative only — proper benchmarks are Milestone 6): one DP seam on a random 600×800 energy map ≈ 20 ms; in the demo (300×480 → 300×336, 144 seams) ≈ 6.3 ms per seam for the DP, 1.5 s for the whole resize including energy and removal.

## Milestone 5 — Demo (2026-10-27)

**Files changed:** new `examples/demo_dp.py`, `examples/make_sample_image.py`, `assets/images/synthetic_scene.png` (480×300, drawn by our script, CC0), `tests/test_examples.py`; `examples/README.md`, `assets/images/README.md`, `README.md`.

**Features:** one command (`python examples/demo_dp.py [image] --width … --height … [--order …] [--frames]`) saves the energy map, the DP table M as a heat map, the first vertical and horizontal seams, all removed seams drawn on the original picture, the result, a comparison with plain resizing and centre cropping, and (with `--frames`) every step for the video; it prints the timings.

**Tests run:** `tests/test_examples.py` (4 tests, included in the 436 above): the demo writes all 8 pictures and the right number of frames on a small image; the committed sample picture equals the generator's output; target-size parsing. Manually ran `python examples/demo_dp.py --frames` on the sample picture: 144 seams removed, 8 pictures + 145 frames written, outputs checked by eye.

**Observation for the report (Q1/Q5):** on the sample picture the 30% width reduction keeps the sun, tree and roof intact but narrows the house and the mountain sides. Every vertical seam has to cross the picture from top to bottom, and the flat inside of the house wall has low gradient energy, so seams pass through it. Plain resizing squashes everything and the centre crop cuts the sun off. With `--width 0.85` the house is only slightly narrower. Forward energy (Rubinstein et al., 2008) is the known fix and a candidate improvement for Q5(d).

**Next:** push the branch and open the pull request (CI on Python 3.10 and 3.12), team review of `docs/interfaces.md`, then Milestone 6 (benchmarks and optimisation notes for Q5) once the greedy and graph-cut finders exist.
