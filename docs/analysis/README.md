# Analysis and discussion (Question 5)

| Item | Question | Owner |
|---|---|---|
| 5, 6, 7 | Q5(a) Answer the Item 2 questions (see [../theory/](../theory/)) | HM, YN, FF |
| 8 | Q5(b) Time and space complexity | YN (DP part), KO (Graph Cut part) |
| 9 | Q5(c) Compare DP vs Greedy vs Graph Cut on seams, visual quality and runtime | RB |
| 10 | Q5(d) Improvements: do you need the whole DP table, does multithreading help? | RB |

RB works on the `docs/rb-analysis-report` branch. Inputs: the DP complexity table, DP-table memory options and multithreading discussion in [../algorithm_design.md](../algorithm_design.md) §2.9–2.11, the Graph Cut design and complexity in §3, and measured timings from HM's benchmark harness in `benchmarks/`.
