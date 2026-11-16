# Milestone log (repository)

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

## Milestone 1b — Switch to C++ and correct the task split (2026-11-01, WN)

**Why:** the Milestone 1 set-up used Python and a task split that did not match the one the team agreed (10 items, see `docs/team.md`).

**Files changed:** removed `pyproject.toml`, `requirements.txt`, `requirements-dev.txt`, `src/seam_carving/__init__.py`, `tests/test_package.py`. Every other `src/seam_carving/*.py` placeholder became an **empty** `.cpp` (and `interfaces.py` became an empty `interfaces.hpp`) for its owner to write. Added `CMakeLists.txt`. CI (`ci.yml`) now builds with CMake and runs CTest on Linux and Windows. Rewrote `README.md`, `CONTRIBUTING.md`, `docs/team.md`, `docs/requirements.md`, `docs/interfaces.md`, `docs/algorithm_design.md` and the small READMEs for C++ and the correct owners. Updated `docs/git_workflow.md`, `.gitignore`, `.editorconfig`, the PR and issue templates and `CODEOWNERS`.

**Features:** Graph Cut part 1 (item 4, WN): `seam_graph.hpp/.cpp` (graph construction) and `max_flow.hpp/.cpp` (Edmonds-Karp), with `tests/test_graph_cut_part1.cpp`.

**Tests run (Windows, Visual Studio 2022, CMake, Release):** `cmake --build build --config Release` → built with no warnings at `/W4`; `ctest --test-dir build -C Release` → **1/1 passed** (`test_graph_cut_part1`: 29 checks, including max-flow = DP minimum on 1,060 random grids). The Linux CI job has not run yet.

**Known issues / pending:**
- Branch names follow the new split. Owners create their branches; old branches (`feature/ff-dynamic-programming`, `docs/yn-theory`, `docs/ko-analysis`, `docs/rb-coordination`) can be deleted by their owners.
- `origin/feature/ff-dynamic-programming` contains a Python DP and Python shared code written before the switch. They are not on `main`.
- Image library still to be decided by FF (proposal: stb_image, no install).

**Next:** shared foundation in C++ (FF), then DP (YN), Greedy (HM) and Graph Cut part 2 (KO).

**Update 15 Nov:** all of Question 1 is now FF's, including the DP code (item 2). YN writes the Question 2 report parts (items 6 and 8, DP). The foundation and the DP finder are in pull request #2.
