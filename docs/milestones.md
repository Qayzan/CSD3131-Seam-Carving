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
