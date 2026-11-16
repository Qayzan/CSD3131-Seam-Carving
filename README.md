# CSD3131-Seam-Carving

Content-aware image resizing using Dynamic Programming, Greedy Algorithms and Graph Cut — CSD3131 Algorithms Assignment 2 (Team T11, DigiPen Institute of Technology Singapore).

> **Status: C++ project set-up.** The C++17 / CMake structure, CI and design documents are in place. Each member writes their own item in their own files in `src/seam_carving/`. So far only Graph Cut part 1 (graph construction + max-flow) is implemented. See [docs/team.md](docs/team.md) for who does what.

## What is seam carving?

Seam carving (Avidan & Shamir, 2007) shrinks an image by repeatedly removing a **seam**: a connected path of pixels with one pixel per row (vertical seam) or per column (horizontal seam). It removes the seam whose total **energy** (how much the pixels stand out from their neighbours) is lowest. Low-energy regions such as sky or plain walls disappear first, and important objects keep their shape. Plain scaling squashes those objects, and cropping cuts them off.

The assignment compares three ways to find the minimum-energy seam (DP, Greedy, Graph Cut), plus theory, analysis and a quiz.

## Who does what

| # | Member | GitHub | Items | Your branch | Where your work goes |
|---|---|---|---|---|---|
| 1 | FF — Muhammad Fauzan | `Qayzan` | All of Question 1: shared foundation — energy map (Sobel), seam removal, resize loop (1) — and DP seam carving, vertical + horizontal (2) | `feature/ff-foundation` | `image_utils`, `energy`, `interfaces`, `seam_ops`, `carver`, `dynamic_programming`, `src/main.cpp` |
| 2 | YN — Yarni | `nursyazliyarni` | All of Question 2: seam-count proof (5), DP recurrence (6), Divide-and-Conquer Sobel discussion (7); plus DP complexity in Q5(b) | `docs/yn-theory` | `docs/theory/`, `docs/analysis/` |
| 3 | HM — Hwee Min | `lhweemin` | Greedy (3) + benchmark harness | `feature/hm-greedy` | `greedy`, `tests/test_greedy.cpp`, `benchmarks/` |
| 4 | WN — Wani | `waneyyj` | Graph Cut part 1: graph construction + max-flow (4) | `feature/wn-graph-cut` | `seam_graph`, `max_flow`, `tests/test_graph_cut_part1.cpp` |
| 5 | KO — Kervin | `kervino` | Graph Cut part 2: seam extraction + check against DP (4) + Graph Cut complexity (8, Graph Cut part) | `feature/ko-graph-cut` | `graph_cut`, `tests/test_graph_cut.cpp` |
| 6 | RB — Rifqah | `riffyyyy` | Comparison of the three methods (9) + improvements (10) + report assembly (links, team IDs, AI Usage Declaration, formatting) | `docs/rb-analysis-report` | `docs/analysis/`, `docs/report/` |

All source files are in `src/seam_carving/` (`.cpp`, plus a `.hpp` header when other files need to call your code). Full names, SIT IDs, the 10 items and the hand-overs between people are in [docs/team.md](docs/team.md).

## How each person works on their item

**The idea in plain words.** `main` is the team's clean, working copy. Nobody edits it directly. Each person gets their own **branch**: a private copy where you can change things without affecting anyone else. When your part works, you open a **pull request (PR)**, which asks to merge your branch into `main`. GitHub then builds the code and runs the tests automatically (called **CI**). A teammate looks it over, and then it is merged. Because everyone works in different files, people almost never step on each other.

**One-time set-up**

1. Accept the invitation: check your email, or go to https://github.com/Qayzan/CSD3131-Seam-Carving/invitations.
2. Clone the repository and build it. Follow [Quick start](#quick-start) below.
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
4. Before pushing, build and run the tests (see [Quick start](#quick-start)).
5. Push: `git push`. When a piece is ready, open a pull request into `main` (`gh pr create --base main --fill`, or the green button on GitHub) and fill in the checklist.
6. Wait for CI to go green and for one teammate's review, then merge.

**How the pieces connect**

- **DP, Greedy and Graph Cut plug into the shared code.** Each seam finder takes an energy map and returns a seam in the agreed format. Then the shared carving loop, demo and benchmarks work with it unchanged. The format is in [docs/interfaces.md](docs/interfaces.md).
- **Graph Cut is two people.** WN builds the flow network and runs max-flow, and KO turns the result into a seam. The hand-over is described at the top of `src/seam_carving/seam_graph.hpp`.
- **Question 2 belongs to Yarni.** She owns items 5, 6 and 7 (all of Q2); FF provides Sobel code and benchmarks as supporting material, while HM's earlier seam-count notes may be reused.
- **The comparison uses everyone's results.** HM's benchmark harness times all three finders the same way, and RB writes the comparison and improvements from those numbers.

More detail (rules, commit messages, code style) is in [CONTRIBUTING.md](CONTRIBUTING.md) and [docs/git_workflow.md](docs/git_workflow.md).

## Quick start

Requires **CMake 3.16+** and a **C++17** compiler: Visual Studio 2022 (it includes CMake) or g++.

```bash
# 1. Clone (private repository — accept your collaborator invitation first)
git clone https://github.com/Qayzan/CSD3131-Seam-Carving.git
cd CSD3131-Seam-Carving

# 2. Configure and build (Windows: run these in "Developer PowerShell for VS 2022")
cmake -S . -B build
cmake --build build --config Release

# 3. Run all tests
ctest --test-dir build -C Release --output-on-failure
```

In Visual Studio you can also use **File → Open → Folder** on the repository. It reads `CMakeLists.txt` and builds the same targets.

**Want a solution file?** Double-click `open_in_visual_studio.bat` in the repository folder. It generates `build\vs\SeamCarving.slnx` (or `.sln` on older Visual Studio) with your installed Visual Studio and opens it. Press F5 to run the demo on the sample picture; the results land in `examples/output/vs/`. Run the script again after pulling new `.cpp` files. The solution isn't committed because CMake writes absolute paths from your own computer into it.

New `.cpp` files in `src/seam_carving/` and new `tests/test_*.cpp` files are picked up automatically by the next build. Each test file is its own program with its own `main()`, and returns 0 when all its checks pass.

## Project structure

```
CSD3131-Seam-Carving/
├── CMakeLists.txt             # builds the library, the demo app and every test
├── src/
│   ├── main.cpp               # demo app (added by FF)
│   └── seam_carving/          # all algorithm code
│       ├── image_utils.cpp    # load/save images, greyscale                (1, FF)
│       ├── energy.cpp         # energy map (Sobel)                          (1, FF)
│       ├── interfaces.hpp     # EnergyMap / Seam / Orientation / SeamFinder (1, FF)
│       ├── seam_ops.cpp       # validate, remove and highlight seams        (1, FF)
│       ├── carver.cpp         # repeated resizing with any seam finder      (1, FF)
│       ├── dynamic_programming.cpp  # DP seam finder                        (2, FF)
│       ├── greedy.cpp         # greedy seam finder                          (3, HM)
│       ├── seam_graph.hpp/.cpp      # Graph Cut part 1: graph construction  (4, WN)
│       ├── max_flow.hpp/.cpp        # Graph Cut part 1: Edmonds-Karp        (4, WN)
│       └── graph_cut.cpp      # Graph Cut part 2: seam extraction           (4, KO)
├── tests/                     # one test program per file: test_*.cpp
├── examples/                  # demonstrations and sample outputs
├── benchmarks/                # timing / comparison harness                 (HM)
├── assets/images/             # small test images with their source/licence
├── docs/
│   ├── requirements.md        # the brief and rubric, turned into a checklist
│   ├── team.md                # members, IDs, the 10 items, hand-overs
│   ├── algorithm_design.md    # architecture + DP design
│   ├── interfaces.md          # shared contract for all seam finders (draft)
│   ├── git_workflow.md        # branches, PRs, GitHub set-up, invitations
│   ├── ai_usage_log.md        # AI assistance log for the report's AI declaration
│   ├── milestones.md          # what each milestone delivered and how it was tested
│   ├── theory/                # Question 2 notes (items 5, 6, 7)
│   ├── analysis/              # complexity, comparison, improvements (items 8–10)
│   ├── report/                # report drafts and figures (RB assembles)
│   └── presentation/          # slides and video link
└── .github/                   # CI workflow, PR and issue templates, CODEOWNERS
```

## Conventions (summary)

- Images and energy maps are stored row by row and indexed `[row][col]`; row 0 is the top. Colour images are **RGB**, 8 bits per channel.
- Energy maps hold one non-negative `double` per pixel.
- A vertical seam is a `std::vector<int>` of length `rows`: entry `i` is the column removed from row `i`. A horizontal seam has length `cols`: entry `j` is the row removed from column `j`.
- Every seam finder has the same signature, so the algorithms can be swapped and benchmarked fairly. See [docs/interfaces.md](docs/interfaces.md).

## Rules from the brief that affect the code

- Third-party libraries may be used for reading/writing images, gradients, Sobel filtering and maths.
- Library functions that already do content-aware resizing or seam carving **must not** be used. Every seam algorithm here is written by the team.
- The energy map is **recalculated after every seam removal**.

## Contributing

Read [CONTRIBUTING.md](CONTRIBUTING.md) before your first pull request. In short: work on your own branch, keep `main` green, and open a pull request with the checklist completed.

## Deliverables and dates (from the brief)

| Item | When |
|---|---|
| Report (with team ID + SIT IDs, Question 2 answers, write-up, GitHub link, video link and AI declaration) + other materials | Week 12, **Fri 20 Nov, 23:59** |
| Presentation recording (≤ 2 min per person, ≤ 12 min total, demo the app running), linked in the report | with the report |
| Team project quiz (closed book, 10–20 MCQs on our code and report) | Week 13, Tue 24 Nov, during tutorial |

## References

- S. Avidan and A. Shamir, "Seam carving for content-aware image resizing," *ACM SIGGRAPH 2007*. https://dl.acm.org/doi/10.1145/1275808.1276390
- M. Rubinstein, A. Shamir and S. Avidan, "Improved seam carving for video retargeting," *ACM SIGGRAPH 2008*. https://dl.acm.org/doi/pdf/10.1145/1360612.1360615
- A. Das, "Real-world dynamic programming: seam carving," 2019. https://avikdas.com/2019/05/14/real-world-dynamic-programming-seam-carving.html
- T. H. Cormen et al., *Introduction to Algorithms*, 3rd ed., ch. 26 (Maximum Flow: Ford-Fulkerson, Edmonds-Karp, max-flow min-cut theorem).
- Wikipedia, "Seam carving." https://en.wikipedia.org/wiki/Seam_carving
