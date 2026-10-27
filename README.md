# CSD3131-Seam-Carving

Content-aware image resizing using Dynamic Programming, Greedy Algorithms and Graph Cut — CSD3131 Algorithms Assignment 2 (Team T11, DigiPen Institute of Technology Singapore).

> **Status: Milestone 2 (shared utilities).** Image loading/saving, the Sobel energy map, the shared `Seam`/`Orientation` types and seam validation/removal are implemented and tested. Question 1 (Dynamic Programming) and the carving loop come next (Milestone 3). See [docs/milestones.md](docs/milestones.md) and [docs/algorithm_design.md](docs/algorithm_design.md).

## What is seam carving?

Seam carving (Avidan & Shamir, 2007) shrinks an image by repeatedly removing a **seam**: a connected path of pixels with one pixel per row (vertical seam) or per column (horizontal seam). It removes the seam whose total **energy** (how much the pixels stand out from their neighbours) is lowest. Low-energy regions such as sky or plain walls disappear first, and important objects keep their shape. Plain scaling squashes those objects, and cropping cuts them off.

The assignment compares three ways to find the minimum-energy seam (Q1, Q3, Q4), plus theory, analysis and a quiz.

## Who does what

| Member | GitHub | Question | Your branch | Where your work goes |
|---|---|---|---|---|
| FF — Muhammad Fauzan | `Qayzan` | **Q1** Dynamic Programming (vertical + horizontal seams, repeated resizing). Also the shared code everyone uses and repo upkeep | `feature/ff-dynamic-programming` | `src/seam_carving/dynamic_programming.py`, shared modules (`image_utils`, `energy`, `interfaces`, `seam_ops`, `carver`), `tests/` |
| YN — Yarni | `nursyazliyarni` | **Q2** Theory: why the number of seams is exponential, how the problem splits into sub-problems, divide & conquer for convolution | `docs/yn-theory` | `docs/theory/` |
| HM — Hwee Min | `lhweemin` | **Q3** Greedy (choose the cheapest next pixel row by row) and its comparison with DP | `feature/hm-greedy` | `src/seam_carving/greedy.py`, `tests/test_greedy.py` |
| WN — Wani | `waneyyj` | **Q4** Graph Cut (min s–t cut / max-flow) and its comparison with DP | `feature/wn-graph-cut` | `src/seam_carving/graph_cut.py`, `tests/test_graph_cut.py` |
| KO — Kervin | `kervino` | **Q5** Analysis and discussion: complexity, comparing the three methods, improvements | `docs/ko-analysis` | `docs/analysis/` (uses results from `benchmarks/`) |
| RB — Rifqah | `riffyyyy` | **Q6** Team project quiz (Week 13): coordination and preparation, details to be agreed | `docs/rb-coordination` | to be agreed |

Full names, SIT IDs and open questions are in [docs/team.md](docs/team.md).

## How each person works on their question

**The idea in plain words.** `main` is the team's clean, working copy. Nobody edits it directly. Each person gets their own **branch**: a private copy where you can change things without affecting anyone else. When your part works, you open a **pull request (PR)**, which asks to merge your branch into `main`. GitHub then runs the checks automatically (lint and tests, called **CI**). A teammate looks it over, and then it is merged. Because everyone works in different files, people almost never step on each other.

**One-time set-up**

1. Accept the invitation: check your email, or go to https://github.com/Qayzan/CSD3131-Seam-Carving/invitations.
2. Clone the repository and install everything. Follow [Quick start](#quick-start) below.
3. Create your branch from the table above and publish it:

   ```bash
   git switch -c feature/hm-greedy        # use YOUR branch name
   git push -u origin feature/hm-greedy
   ```

**Every time you work**

1. Bring your branch up to date with the latest `main`:

   ```bash
   git switch main && git pull
   git switch feature/hm-greedy && git merge main
   ```

2. Edit **only the files in your row** of the table. If you need a change in someone else's file, ask them or open an issue.
3. Save your progress with small commits: `git add <files>` then `git commit -m "Greedy: pick lowest of three neighbours"`.
4. Before pushing, run the checks: `ruff check .`, `ruff format .`, `pytest`.
5. Push: `git push`. When a piece is ready, open a pull request into `main` (`gh pr create --base main --fill`, or the green button on GitHub) and fill in the checklist.
6. Wait for CI to go green and for one teammate's review, then merge.

**How the pieces connect**

- **Q3 and Q4 plug into the shared code.** Your seam finder takes an energy map and returns a seam in the agreed format. Then the shared carving loop, demos and benchmarks work with it unchanged. The format is in [docs/interfaces.md](docs/interfaces.md). The shared code (`interfaces`, `energy`, `seam_ops`, `image_utils`) is in place from Milestone 2, so you can import it now; `seam_ops.validate_seam(seam, energy.shape, energy=energy)` checks your output against the contract.
- **Q2 and Q1 share notation.** The DP recurrence in [docs/algorithm_design.md](docs/algorithm_design.md) is the sub-problem structure Q2 asks about. Use the same symbols in the report.
- **Q5 uses everyone's results.** The timing and seam-energy numbers come from `benchmarks/`, run on all three methods.
- **Theory and analysis (Q2, Q5)** are Markdown files in `docs/`. They go through branches and pull requests too, so the report has one tidy source.

More detail (rules, commit messages, code style) is in [CONTRIBUTING.md](CONTRIBUTING.md) and [docs/git_workflow.md](docs/git_workflow.md).

## Quick start

Requires **Python 3.10+**.

```bash
# 1. Clone (private repository — accept your collaborator invitation first)
git clone https://github.com/Qayzan/CSD3131-Seam-Carving.git
cd CSD3131-Seam-Carving

# 2. Create and activate a virtual environment
python -m venv .venv
# Windows (PowerShell):  .venv\Scripts\Activate.ps1
# macOS / Linux:         source .venv/bin/activate

# 3. Install the package in editable mode with the developer tools
python -m pip install --upgrade pip
python -m pip install -e ".[dev]"

# 4. Check everything works
ruff check .
ruff format --check .
pytest
```

If you only need the dependencies (no editable install): `pip install -r requirements-dev.txt`, then run the tests with `python -m pytest`.

## Project structure

```
CSD3131-Seam-Carving/
├── src/seam_carving/          # the Python package
│   ├── image_utils.py         # load/save images, RGB conventions          (shared, FF)
│   ├── energy.py              # energy maps (Sobel gradient magnitude)     (shared, FF)
│   ├── interfaces.py          # Seam / Orientation / SeamFinder contract   (shared, FF)
│   ├── seam_ops.py            # validate, remove and highlight seams       (shared, FF)
│   ├── carver.py              # repeated resizing with any seam finder     (shared, FF)
│   ├── dynamic_programming.py # Question 1                                 (FF)
│   ├── greedy.py              # Question 3 — placeholder                   (HM)
│   └── graph_cut.py           # Question 4 — placeholder                   (WN)
├── tests/                     # pytest suites (one file per module)
├── examples/                  # runnable demonstrations (Milestone 5)
├── benchmarks/                # timing / comparison scripts (Milestone 6)
├── assets/images/             # small test images with their source/licence
├── docs/
│   ├── requirements.md        # the brief and rubric, turned into a checklist
│   ├── team.md                # members, IDs, responsibilities
│   ├── algorithm_design.md    # architecture + Question 1 DP design
│   ├── interfaces.md          # shared contract for all seam finders (draft)
│   ├── git_workflow.md        # branches, PRs, GitHub set-up, invitations
│   ├── ai_usage_log.md        # AI assistance log for the report's AI declaration
│   ├── milestones.md          # what each milestone delivered and how it was tested
│   ├── theory/                # Question 2 notes (YN)
│   ├── analysis/              # Question 5 notes (KO)
│   ├── report/                # report drafts and figures
│   └── presentation/          # slides and video link
└── .github/                   # CI workflow, PR and issue templates, CODEOWNERS
```

## Conventions (summary)

- Images are NumPy arrays indexed `image[row, col]`, in **RGB** order (`uint8`, shape `(H, W, 3)`), or greyscale `(H, W)`. OpenCV's BGR order is converted at load/save time only.
- Energy maps are `float64` arrays of shape `(H, W)`.
- A vertical seam is an integer array `cols` of length `H`. `cols[i]` is the column removed from row `i`. A horizontal seam is an array `rows` of length `W`.
- Every seam finder has the same signature, so the algorithms can be swapped and benchmarked fairly. See [docs/interfaces.md](docs/interfaces.md).

## Rules from the brief that affect the code

- OpenCV and NumPy may be used for reading/writing images, gradients and Sobel filtering.
- Library functions that already do content-aware resizing or seam carving **must not** be used. Every seam algorithm here is written by the team.
- The energy map is **recalculated after every seam removal**.

## Contributing

Read [CONTRIBUTING.md](CONTRIBUTING.md) before your first pull request. In short: work on your own branch, keep `main` green, and open a pull request with the checklist completed.

## Deliverables and dates (from the brief)

| Item | When |
|---|---|
| Report (PDF, with GitHub link, video link and AI declaration) + other materials | Week 12, **Fri 20 Nov, 23:59** |
| Team project quiz (closed book, 10–20 MCQs on our code and report) | Week 13, Tue 24 Nov, during tutorial |

## References

- S. Avidan and A. Shamir, "Seam carving for content-aware image resizing," *ACM SIGGRAPH 2007*. https://dl.acm.org/doi/10.1145/1275808.1276390
- M. Rubinstein, A. Shamir and S. Avidan, "Improved seam carving for video retargeting," *ACM SIGGRAPH 2008*. https://dl.acm.org/doi/pdf/10.1145/1360612.1360615
- A. Das, "Real-world dynamic programming: seam carving," 2019. https://avikdas.com/2019/05/14/real-world-dynamic-programming-seam-carving.html
- Wikipedia, "Seam carving." https://en.wikipedia.org/wiki/Seam_carving
