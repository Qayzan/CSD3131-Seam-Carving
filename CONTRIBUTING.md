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
git commit -m "Q3: choose minimum neighbour per row"
git push -u origin feature/hm-greedy       # first push; afterwards just git push
```

Then open a **pull request into `main`** on GitHub and fill in the template.

### Branch names

| Owner | Branch |
|---|---|
| FF | `feature/ff-dynamic-programming` |
| HM | `feature/hm-greedy` |
| WN | `feature/wn-graph-cut` |
| YN | `docs/yn-theory` |
| KO | `docs/ko-analysis` |
| RB | `docs/rb-coordination` |

Use `fix/<initials>-<topic>` for small fixes.

## 3. Rules

- **Never push directly to `main`.** Everything goes through a pull request. If the repository plan allows it, `main` is protected (see [docs/git_workflow.md](docs/git_workflow.md)).
- **Never force-push** (`git push --force`) to `main` or to anyone else's branch.
- **Edit only your own files.** If you need a change in someone else's file or in shared code (`energy.py`, `interfaces.py`, `seam_ops.py`, `carver.py`, `image_utils.py`), ask them or open an issue. Shared-code changes need FF's review.
- **Do not use library seam-carving / content-aware resize functions.** The brief forbids them. OpenCV may be used for I/O, gradients and Sobel.
- **Do not commit large files.** Keep test images small (under ~1 MB). Videos go on YouTube or a drive, never in git.
- **Record AI help.** Add a line to [docs/ai_usage_log.md](docs/ai_usage_log.md) whenever an AI tool shaped your design, code or text. The report needs an AI declaration with the main prompts.

## 4. Before you open a pull request

```bash
ruff check .            # lint
ruff format .           # auto-format (CI runs `ruff format --check .`)
pytest                  # all tests must pass
```

CI runs the same three checks on every pull request (Python 3.10 and 3.12). A red CI blocks merging.

## 5. Code style

- Python 3.10+, formatted by `ruff format`, line length 100.
- `snake_case` functions and variables, `PascalCase` classes, `UPPER_CASE` constants.
- Every public function gets a docstring stating **array shapes, dtypes and coordinate conventions**. These are where seam-carving bugs usually hide.
- Comments explain *why*, not *what*. The rubric rewards "well explained and commented" code.
- Prefer clear NumPy over clever tricks. Add a simple reference version in the tests when you optimise.

## 6. Tests

- Put tests in `tests/test_<module>.py`.
- Use tiny hand-checkable inputs (3×3, 4×5) in addition to real images.
- Where possible, compare against an **independent oracle**. For example, the exhaustive brute-force seam search in the DP tests also works for greedy and graph-cut tests.
- Never claim results you did not run. Paste the real `pytest` summary into your PR.

## 7. Commit messages

Start with the area, then say what changed in the imperative mood:

```
Q1: backtrack seam from bottom row using back-pointers
shared: convert BGR to RGB in load_image
docs: add Q2(a)(i) proof sketch
```
