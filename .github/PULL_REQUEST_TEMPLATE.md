## What does this PR do?

<!-- One or two sentences. Link the issue if there is one: "Closes #12". -->

## Item / area

- [ ] 1 Shared foundation: energy map, seam removal, resize loop, transpose (FF)
- [ ] 2 DP seam carving (FF)
- [ ] 3 Greedy seam carving (HM)
- [ ] 4 Graph Cut part 1: graph construction + max-flow (WN)
- [ ] 4 Graph Cut part 2: seam extraction + check against DP (KO)
- [ ] 5 Exponential number of seams (HM)
- [ ] 6 Seam cost and DP recurrence (YN)
- [ ] 7 Divide and Conquer for Sobel (FF)
- [ ] 8 Complexity: DP part (YN) / Graph Cut part (KO)
- [ ] 9 Comparison of the three methods (RB)
- [ ] 10 Improvements (RB)
- [ ] Report assembly (RB)
- [ ] Benchmark harness (HM)
- [ ] Shared docs, build or tooling

## Checklist

- [ ] `cmake --build build --config Release` builds with no errors or new warnings
- [ ] `ctest --test-dir build -C Release` passes locally (paste the summary below)
- [ ] New behaviour has tests; algorithm results are checked against an independent method where possible
- [ ] Follows the shared contract in `docs/interfaces.md` (sizes, seam coordinates, return types)
- [ ] Only touches files owned by the author, or the owner has agreed in this PR
- [ ] Docs / report notes updated if behaviour or results changed
- [ ] AI assistance recorded in `docs/ai_usage_log.md` (needed for the report's AI declaration)

## Test output

```
<paste ctest summary here>
```

## Screenshots / sample outputs (if visual)
