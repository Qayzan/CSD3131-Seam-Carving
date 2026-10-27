# Team T11 — members and responsibilities

> **Please verify.** Full names and SIT IDs are copied from our Assignment 1 repository (`T11-IDs/README.md`). The team ID **T11** is assumed to carry over to Assignment 2. Confirm both before they go on the report cover: the brief requires "team id and team members (with SIT ID)".

| Initials | Member | SIT ID | GitHub | Responsibility | Branch |
|---|---|---|---|---|---|
| FF | Muhammad Fauzan bin Mohamed Fauzy | 2403386 | `Qayzan` | **Q1** Dynamic Programming seam carving; repository set-up, shared utilities, integration | `feature/ff-dynamic-programming` |
| YN | Nur Syazliyarni Binte Zazali (Yarni) | 2402942 | `nursyazliyarni` | **Q2** Theory — (a)(i) exponential number of seams, (a)(ii) relating to sub-problems, (b) divide & conquer for convolution | `docs/yn-theory` |
| HM | Lee Hwee Min | 2403441 | `lhweemin` | **Q3** Greedy algorithm + comparison with DP | `feature/hm-greedy` |
| WN | Siti Syazwani Binte Jusri (Wani) | 2402594 | `waneyyj` | **Q4** Graph Cut (min-cut / max-flow) + comparison with DP | `feature/wn-graph-cut` |
| KO | Kervin Ong Guan Cheng | 2403337 | `kervino` | **Q5** Analysis and discussion — (a) answers to Q2, (b) time/space complexity, (c) comparison, (d) improvements | `docs/ko-analysis` |
| RB | Rifqah Batrisya Binte Nazarudi | 2401601 | `riffyyyy` | **Q6** Project quiz — coordination / preparation (to be agreed) | `docs/rb-coordination` |

## Open questions for the team

1. **Question 6 is a team quiz, not a coding task.** RB's deliverable still needs agreeing. Ideas: a shared revision sheet of likely quiz questions about our code and report, keeping the internal schedule, and report assembly. It is *not* a coding deliverable unless RB chooses one.
2. **Shared deliverables still need owners.** These are the report assembly, slides, the 12-minute presentation (max 2 minutes per person), the video upload, the AI declaration and peer evaluation.
3. **Bonus (up to 10 marks).** These are: (a) step-by-step visualisation of all seams, (b) user-adjustable image sizes, (c) CI/CD. CI already exists in this repository. Decide who owns (a) and (b) once Q1–Q4 work.
4. **Q5(a) "Answer the questions (Item 2)"** overlaps with YN's Q2 work. Agree who writes the final report text, so it is neither duplicated nor missing.

## Interfaces between people

- **FF → everyone:** image I/O, energy maps, the `Seam` / `SeamFinder` contract, seam removal and the carving loop. Their contract is in [interfaces.md](interfaces.md). Teammates *call* these. They don't need to re-implement them.
- **HM, WN → FF:** a `find_seam(energy, orientation)` that follows the contract, so the carver, benchmarks and demo can run it.
- **FF, HM, WN → KO:** timings and seam energies produced by `benchmarks/` (Milestone 6), for the comparison in Q5.
- **YN ↔ FF:** the DP recurrence in [algorithm_design.md](algorithm_design.md) is the same structure Q2(a)(ii) asks about. Keep the notation consistent in the report.

## Internal timeline (proposal — adjust at the next meeting)

| Week | Goal |
|---|---|
| Now | Repository live, everyone invited and able to run `pytest` |
| +1 week | Shared utilities merged (energy, interfaces, seam ops). Q1 DP merged with tests |
| +2 weeks | Q3 Greedy and Q4 Graph Cut merged. Q2 drafts ready |
| +3 weeks | Benchmarks + Q5 analysis; report draft; demo / bonus work |
| Week 12 | Presentation recorded. **Submit by Fri 20 Nov, 23:59** |
| Week 13 | Project quiz, Tue 24 Nov (tutorial) |
