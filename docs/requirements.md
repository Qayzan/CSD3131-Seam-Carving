# Requirements checklist

Taken from `Assignment 2.pdf` (100 marks + 10 bonus) and `Rubrics.pdf`. Requirement IDs (R1…) are referenced in pull requests and in the report so every requirement can be traced to code, tests and text. "Item" refers to the 10 work items in [team.md](team.md).

## Deliverables in the brief

| ID | Requirement (brief item) | Item | Owner | Where it will live |
|---|---|---|---|---|
| R1a | **Task 1(a)** Energy map per pixel: gradient (intensity difference between neighbours) or a filter such as Sobel | 1 | FF | `energy.cpp`, `tests/test_energy.cpp` ✅ |
| R1b | **Task 1(b)** Dynamic programming finds the minimum-cumulative-energy seam, **vertical and horizontal** | 2 | FF | `dynamic_programming.cpp`, `tests/test_dynamic_programming.cpp` ✅ |
| R1c | **Task 1(c)** Remove the seam, **recalculate the energy map**, repeat until the target size; horizontal seams via transpose | 1 | FF | `seam_ops.cpp`, `carver.cpp`, `tests/test_carver.cpp` ✅ |
| R2a-i | **Q2(a)(i)** Show the number of seams grows at least exponentially in m (n > 1) | 5 | YN | `docs/theory/` → report |
| R2a-ii | **Q2(a)(ii)** With energies e[i,j] and seam cost = sum, relate the problem to sub-problems (DP recurrence) | 6 | YN | `docs/theory/` → report |
| R2b | **Q2(b)** Discuss whether divide & conquer could speed up gradient / Sobel convolution | 7 | YN | `docs/theory/item7_divide_and_conquer_sobel.md` is FF's technical draft and benchmark input; YN owns the final Question 2(b) write-up |
| R3 | **Task 3** Greedy seam carving (row-by-row local minimum). Compare seams, visual quality and running time with DP | 3 (+9) | HM (comparison: RB) | `greedy.cpp`, `tests/test_greedy.cpp` |
| R4a | **Task 4** Graph cut, part 1: s–t graph with infinite backward and diagonal edges; min cut via max-flow (**Ford–Fulkerson / Edmonds–Karp**) | 4 | WN | `seam_graph.*`, `max_flow.*`, `tests/test_graph_cut_part1.cpp` |
| R4b | **Task 4** Graph cut, part 2: seam from the residual graph, horizontal via transpose, check against DP | 4 | KO | `graph_cut.cpp`, `tests/test_graph_cut.cpp` |
| R4c | **Task 4** Compare graph cut seams, visual quality and running time with DP | 9 | RB (data: KO, HM) | `benchmarks/` + report |
| R5a | **Q5(a)** Answer the Item 2 questions | 5, 6, 7 | YN (with technical input from FF and HM) | report |
| R5b | **Q5(b)** Time and space complexity | 8 | YN (DP), KO (Graph Cut) | `docs/analysis/` → report |
| R5c | **Q5(c)** Comparison of DP, greedy and graph cut | 9 | RB | `benchmarks/` + report |
| R5d | **Q5(d)** Improvements, e.g. is the whole DP table needed? Does multithreading help? | 10 | RB | report |
| R6 | **Q6** Project quiz, Week 13 Tue 24 Nov: closed book, team-based, 10–20 MCQs on our code and report. A score multiplier may apply (80–100% → 1.00 … < 40% → 0.70) | — | all | — |
| R7a | **Bonus** Step-by-step visualisation with all seams shown | — | FF | `web/` demo: each seam drawn before removal, *Removed seams* view ✅ |
| R7b | **Bonus** User interaction to edit image sizes | — | FF | `web/` demo: target-size sliders, play / pause / step, own pictures ✅ |
| R7c | **Bonus** CI/CD | — | FF | `.github/workflows/ci.yml` ✅ (CI: CMake build + CTest on Linux and Windows; web demo built every run, CD to Netlify once the secrets are set) |
| R8 | **Presentation** ≤ 2 min per person, ≤ 12 min total. Demo the app running. Upload a recording (e.g. YouTube) and put the link in the report | — | all | `docs/presentation/` |
| R9a | **Report** (XSite Dropbox): team id + members with SIT IDs; answers to Item 2; algorithm, design, implementation, results/evaluation, comparison and discussion (highlight algorithm and analysis); links to GitHub code and the video; **AI usage declaration** with how AI was used and the main prompts | all | RB assembles; each owner writes their items | `docs/report/` |
| R9b | **Other materials**: slides, test inputs/outputs (optional) | — | all | `docs/presentation/`, `examples/` |
| R9c | **Deadline: Week 12, Fri 20 Nov, 23:59** | — | all | — |

## Constraints in the brief

- Any language is allowed. **The team uses C++17** (built with CMake). Third-party libraries may be used for image I/O, gradient magnitude or Sobel, and maths.
- **Library content-aware resizing / seam-carving functions are not allowed.** Each algorithm is implemented by us.
- Q2(a) is a starting aid: "your final implementation does not need to follow this question."

## Rubric (100 marks)

| Item | Points | "Excellent" band says… | How this repository supports it |
|---|---|---|---|
| Design (algorithms) — accuracy for non-programming answers | 20 | Excellent and efficient design, clear explanation and justification, no error, **considers alternative approaches** | `algorithm_design.md` (design, alternatives, justification); comparison of DP / greedy / graph cut |
| Implementation (algorithms) — explanation for non-programming answers | 20 | Correct result, good programming practice, **proof of detailed testing of all cases**, efficient code covering all scenarios | Tests with independent oracles, CI, documented edge cases |
| Analysis (e.g. Big-O) | 20 | In-depth analysis, no error, excellent arguments | Complexity tables in `algorithm_design.md` and `docs/analysis/`; measured timings |
| Documentation | 20 | Well-structured, clear and accurate, **beyond the requirement (e.g. references, comparative study)**, follows submission instructions fully | Reference list, requirement traceability (this file), report folder |
| Presentation | 15 | Exceptional, consistent and professional; full and accurate explanation of all parts | Demo app, figures generated from code |
| Peer evaluation | 5 | Present | — |

## Things not yet specified by the brief (ask the instructor if they matter)

- Required file naming for Assignment 2 submissions. Assignment 1 used `T11-<six IDs>`. The Assignment 2 brief does not repeat it.
- Whether image **enlargement** (seam insertion) is expected. The brief's steps only describe removal, so it is out of scope unless requested.
- A required energy function. The brief allows gradient-based energy or Sobel.
