# CSD3131-Seam-Carving

Content-aware image resizing using Dynamic Programming, Greedy Algorithms and Graph Cut — CSD3131 Algorithms Assignment 2 (Team T11, DigiPen Institute of Technology Singapore).

> **Status: Milestone 1 (repository scaffolding).** The package structure, tooling, CI and design documents are in place. The algorithms are not implemented yet. Shared utilities and Question 1 (Dynamic Programming) come next. See [docs/algorithm_design.md](docs/algorithm_design.md).

## What is seam carving?

Seam carving (Avidan & Shamir, 2007) shrinks an image by repeatedly removing a **seam**: a connected path of pixels with one pixel per row (vertical seam) or per column (horizontal seam). It removes the seam whose total **energy** (how much the pixels stand out from their neighbours) is lowest. Low-energy regions such as sky or plain walls disappear first, and important objects keep their shape. Plain scaling squashes those objects, and cropping cuts them off.

The assignment compares three ways to find the minimum-energy seam:

| Question | Approach | Owner |
|---|---|---|
| Q1 | Dynamic Programming (vertical + horizontal seams, repeated resizing) | FF — Fauzan Fauzy |
| Q2 | Theory: exponential number of seams, sub-problem structure, divide & conquer for convolution | YN — Yarni |
| Q3 | Greedy (row-by-row local choice) | HM — Hwee Min |
| Q4 | Graph Cut (min s–t cut / max-flow) | WN — Wani |
| Q5 | Analysis and discussion (complexity, comparison, improvements) | KO — Kervin Ong |
| Q6 | Team project quiz (Week 13) — coordination / preparation | RB — Rifqah Batrisya |

The full ownership table and member SIT IDs are in [docs/team.md](docs/team.md).

## Quick start

Requires **Python 3.10+**.

```bash
# 1. Clone (after the repository has been published — see docs/git_workflow.md)
git clone https://github.com/<owner>/CSD3131-Seam-Carving.git
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
