# Contributing

Six people work in this repository, so this file sets a few simple rules. The aim is that `main` always works and that each person's work stays clearly theirs. The project quiz asks each of us about our own submitted code.

## 1. One-time set-up

1. Accept the GitHub invitation (see [docs/git_workflow.md](docs/git_workflow.md)).
2. Clone the repository and follow **Quick start** in the [README](README.md).
3. Set your git identity so commits are attributed correctly:
   ```bash
   git config user.name  "Your Name"
   git config user.email "the-email-on-your-GitHub-account"
   ```

## 2. Daily workflow

```bash
git switch main
git pull                                   # get the latest main
git switch -c feature/hm-greedy            # first time only; afterwards: git switch feature/hm-greedy
git merge main                             # bring your branch up to date (no rebasing shared branches)
# ... work, run the checks below ...
git add <files>
git commit -m "Greedy: choose minimum neighbour per row"
git push -u origin feature/hm-greedy       # first push; afterwards just git push
```

Then open a **pull request into `main`** on GitHub and fill in the template.

### Branch names

| # | Owner | Items | Branch |
|---|---|---|---|
| 1 | FF | Shared foundation (1), DP (2), D&C Sobel (7) | `feature/ff-foundation` |
| 2 | YN | Report: recurrence (6), DP complexity (8) | `docs/yn-theory` |
| 3 | HM | Greedy (3), exponential proof (5), benchmarks | `feature/hm-greedy` |
| 4 | WN | Graph Cut part 1: graph + max-flow (4) | `feature/wn-graph-cut` |
| 5 | KO | Graph Cut part 2: extraction + DP check (4), Graph Cut complexity (8) | `feature/ko-graph-cut` |
| 6 | RB | Comparison (9), improvements (10), report assembly | `docs/rb-analysis-report` |

Use `fix/<initials>-<topic>` for small fixes.

## 3. Rules

- **Never push directly to `main`.** Everything goes through a pull request. If the repository plan allows it, `main` is protected (see [docs/git_workflow.md](docs/git_workflow.md)).
- **Never force-push** (`git push --force`) to `main` or to anyone else's branch.
- **Edit only your own files** (see the table in [docs/team.md](docs/team.md)). If you need a change in someone else's file or in shared code (`image_utils`, `energy`, `interfaces`, `seam_ops`, `carver`, `CMakeLists.txt`), ask them or open an issue. Shared-code changes need FF's review.
- **Do not use library seam-carving / content-aware resize functions.** The brief forbids them. Libraries may be used for image I/O, gradients and Sobel.
- **Do not commit build output or large files.** `build/` and compiled files are git-ignored. Keep test images small (under ~1 MB). Videos go on YouTube or a drive, never in git.
- **Record AI help.** Add a line to [docs/ai_usage_log.md](docs/ai_usage_log.md) whenever an AI tool shaped your design, code or text. The report needs an AI declaration with the main prompts.

## 4. Before you open a pull request

```bash
cmake -S . -B build                                    # once, or after adding files
cmake --build build --config Release                   # must build with no errors
ctest --test-dir build -C Release --output-on-failure  # all tests must pass
```

CI runs the same build and tests on every pull request, on Linux (g++) and Windows (MSVC). A red CI blocks merging. Fix compiler warnings in your own files too: CI builds with `/W4` and `-Wall -Wextra -Wpedantic`.

## 5. Code style

- C++17, 4-space indentation, lines up to about 100 characters.
- `camelCase` functions and variables, `PascalCase` types, `kCamelCase` or `UPPER_CASE` constants, `snake_case` file names.
- Every public function in a header gets a comment stating **sizes, coordinate conventions and what it throws**. These are where seam-carving bugs usually hide.
- Comments explain *why*, not *what*. The rubric rewards "well explained and commented" code.
- Prefer clear loops over clever tricks. Add a simple reference version in the tests when you optimise.
- Throw `std::invalid_argument` for bad input (wrong size, empty map) instead of crashing.

## 6. Tests

- Put tests in `tests/test_<topic>.cpp`. Each file is its own program: give it a `main()` that prints PASS/FAIL per check and returns 0 only when everything passed. CMake registers it with CTest automatically.
- Use tiny hand-checkable inputs (3×3, 4×5) in addition to real images.
- Where possible, compare against an **independent oracle**. For example, an exhaustive brute-force seam search, or the DP minimum: greedy energy ≥ optimum, graph cut energy = optimum.
- Never claim results you did not run. Paste the real `ctest` summary into your PR.

## 7. Commit messages

Start with the area, then say what changed in the imperative mood:

```
DP: backtrack seam from bottom row using back-pointers
Graph cut: build diagonal INF edges
shared: mirror borders in the Sobel energy
docs: add Q2(a)(i) proof sketch
```
