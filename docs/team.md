# Team T11 — members and responsibilities

> **Please verify.** Full names and SIT IDs are copied from our Assignment 1 repository (`T11-IDs/README.md`). The team ID **T11** is assumed to carry over to Assignment 2. Confirm both before they go on the report cover: the brief requires "team id and team members (with SIT ID)".

## The work, as 10 items

The brief's tasks are split into 10 items. Each item has exactly one owner (table below).

| # | Item | Brief reference |
|---|---|---|
| 1 | **Shared foundation:** energy map (Sobel), seam removal, resize loop, horizontal seams via transpose | Task 1(a), 1(c) |
| 2 | **DP seam carving** (vertical + horizontal) | Task 1(b) |
| 3 | **Greedy seam carving** | Task 3 |
| 4 | **Graph Cut seam carving** (min-cut / max-flow) | Task 4 |
| 5 | Prove the number of seams grows at least exponentially in m | Q2(a)(i) |
| 6 | Define the seam cost and give the DP sub-problem recurrence | Q2(a)(ii) |
| 7 | Discuss whether Divide and Conquer speeds up the Sobel convolution | Q2(b) |
| 8 | Time and space complexity | Q5(b) |
| 9 | Compare DP vs Greedy vs Graph Cut on seams, visual quality and runtime | Tasks 3, 4 and Q5(c) |
| 10 | Improvements: do you need the whole DP table, does multithreading help? | Q5(d) |

The report write-up is spread across all items: every owner writes the report text for their own items. Bonus items (step-by-step visualisation, interactive resizing, CI/CD) are optional.

## Who does what

| # | Initials | Member | SIT ID | GitHub | Items | Load |
|---|---|---|---|---|---|---|
| 1 | FF | Muhammad Fauzan bin Mohamed Fauzy | 2403386 | `Qayzan` | All of Question 1: shared foundation (1) + DP seam carving (2), and the Divide-and-Conquer Sobel discussion (7) | Heavy. The DP finder moved here on 15 Nov so Question 1 is one person's code |
| 2 | YN | Nur Syazliyarni Binte Zazali (Yarni) | 2402942 | `nursyazliyarni` | Question 2 report: DP recurrence (6) + DP complexity (8, DP part) | Medium. Written work only since 15 Nov; the DP code is FF's |
| 3 | HM | Lee Hwee Min | 2403441 | `lhweemin` | Greedy (3) + exponential-seams proof (5) + benchmark harness | Medium. Greedy is light, so the proof and timing tools fill the gap |
| 4 | WN | Siti Syazwani Binte Jusri (Wani) | 2402594 | `waneyyj` | Graph Cut part 1: graph construction + max-flow (4) | Heavy |
| 5 | KO | Kervin Ong Guan Cheng | 2403337 | `kervino` | Graph Cut part 2: seam extraction + check against DP (4) + Graph Cut complexity (8, Graph Cut part) | Heavy |
| 6 | RB | Rifqah Batrisya Binte Nazarudi | 2401601 | `riffyyyy` | Comparison of the three methods (9) + improvements (10) + report assembly (links, team IDs, AI Usage Declaration, formatting) | Medium. Heavy near the deadline, since it depends on everyone else finishing |

## Files and branches

| Owner | Files | Branch |
|---|---|---|
| FF | `src/seam_carving/image_utils.cpp`, `energy.cpp`, `interfaces.hpp`, `seam_ops.cpp`, `carver.cpp`, `dynamic_programming.cpp` (+ their headers), `src/main.cpp` (demo app), their tests; report notes for item 7 | `feature/ff-foundation` |
| YN | report notes for items 6 and 8 (DP), in `docs/theory/` and `docs/analysis/` | `docs/yn-theory` |
| HM | `src/seam_carving/greedy.cpp` (+ header), `tests/test_greedy.cpp`, `benchmarks/`; report notes for item 5 | `feature/hm-greedy` |
| WN | `src/seam_carving/seam_graph.*`, `src/seam_carving/max_flow.*`, `tests/test_graph_cut_part1.cpp` | `feature/wn-graph-cut` |
| KO | `src/seam_carving/graph_cut.cpp` (+ header), `tests/test_graph_cut.cpp`; report notes for item 8 (Graph Cut) | `feature/ko-graph-cut` |
| RB | `docs/analysis/`, `docs/report/` | `docs/rb-analysis-report` |

> **Branches from the first set-up.** `origin/feature/ff-dynamic-programming` holds a Python DP written before this split. The project is now C++, and the C++ DP is FF's (`dynamic_programming.cpp`). The old branches (`docs/yn-theory`, `docs/ko-analysis`, `docs/rb-coordination`) belong to the old split. Each owner creates the branch from the table above and can delete their old one.

## Interfaces between people

- **FF → everyone:** image I/O, the energy map, the shared types (`interfaces.hpp`: `EnergyMap`, `Seam`, `Orientation`, `SeamFinder`), seam removal and the carving loop. The contract is in [interfaces.md](interfaces.md). Teammates *call* these. They don't re-implement them.
- **HM, KO → FF:** a `findSeam(energy, orientation)` for DP, greedy and graph cut that follows the contract, so the carver, benchmarks and demo can run all three.
- **WN → KO (Graph Cut):** `buildVerticalSeamGraph(energy)` builds the flow network, and `FlowNetwork::maxFlow(source, sink)` leaves the residual capacities in `edges()`. KO reads the source side S from them (BFS from the source through edges with capacity > 0) and turns it into the seam. The details are at the top of `src/seam_carving/seam_graph.hpp`.
- **FF → KO:** the DP finder (`dp::findSeam`) is the reference KO checks the graph cut seam against.
- **HM → everyone, RB:** the benchmark harness times all three finders the same way. RB uses its results for the comparison (item 9).
- **YN, KO → RB:** the DP and Graph Cut complexity (item 8) feed the comparison and improvements (items 9, 10).
- **HM, YN, FF:** items 5, 6 and 7 together form the Question 2 answers. Use the same notation as [algorithm_design.md](algorithm_design.md) §2.3.

## Internal timeline (proposal — adjust at the next meeting)

| Week | Goal |
|---|---|
| Now | Repository switched to C++; everyone can build with CMake and run `ctest` |
| +1 week | Shared foundation merged (image I/O, energy, interfaces, seam ops). DP merged with tests |
| +2 weeks | Greedy and Graph Cut (parts 1 + 2) merged. Question 2 drafts ready |
| +3 weeks | Benchmarks, comparison, improvements; report draft; demo / bonus work |
| Week 12 | Presentation recorded. **Submit by Fri 20 Nov, 23:59** |
| Week 13 | Project quiz, Tue 24 Nov (tutorial) |

## Open questions for the team

1. **Shared deliverables still need owners.** These are the slides, the 12-minute presentation (max 2 minutes per person, everyone presents), the video upload and peer evaluation. Report assembly is RB's (item list above).
2. **Bonus (up to 10 marks).** (a) step-by-step visualisation of all seams, (b) interactive resizing, (c) CI/CD. CI already exists in this repository. Decide who owns (a) and (b) once items 1–4 work.
3. **Image library.** Proposal: `stb_image` / `stb_image_write` (two public-domain header files in `third_party/stb/`, nothing to install) with a hand-written Sobel. FF decides as part of item 1.
