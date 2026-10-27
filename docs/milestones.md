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
