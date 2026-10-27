# Git workflow and GitHub set-up

## 1. Branches

| Branch | Purpose |
|---|---|
| `main` | Stable integration branch. Always passes CI. Changes arrive only through pull requests. |
| `feature/ff-dynamic-programming` | Q1 (FF) |
| `feature/hm-greedy` | Q3 (HM) |
| `feature/wn-graph-cut` | Q4 (WN) |
| `docs/yn-theory` | Q2 notes (YN) |
| `docs/ko-analysis` | Q5 notes (KO) |
| `docs/rb-coordination` | Q6 / coordination (RB) |

Branches are created by their owners when they start. They don't all need to exist on day one.

**Rules:** pull requests into `main`; no force-pushing to shared branches; merge `main` into your branch to update it. Don't rebase branches other people use. Squash-merge or merge-commit are both fine. Pick one in the repository settings and keep it.

## 2. Creating and publishing the repository (FF, one time)

**Done on 2026-10-27.** FF ran `publish-repo.ps1` (kept outside the repository), which did the steps below: first commit `dd8d114` on `main`, created the **private** repository [Qayzan/CSD3131-Seam-Carving](https://github.com/Qayzan/CSD3131-Seam-Carving), pushed `main` and invited the five teammates. The manual steps are kept here for reference. Run these on your Windows machine in PowerShell:

```powershell
cd "C:\Users\fauza\OneDrive\Desktop\School\Y3T1\CSD3131\Assignment\Assignment2\CSD3131-Seam-Carving"

# 0. Put the .github folder in place (unzip github-folder.zip here), then delete .github\probe.txt if it exists

# 1. Initialise git with main as the default branch and make the first commit
git init -b main
git config user.name  "Fauzan Fauzy"
git config user.email "<the email on your GitHub account>"
git add -A
git status               # check: no .venv, __pycache__ or *.egg-info listed
git commit -m "Milestone 1: repository scaffolding, CI, docs and Q1 design"

# 2. Install and sign in to GitHub CLI (once)
winget install --id GitHub.cli
gh auth login            # GitHub.com → HTTPS → "Login with a web browser"

# 3. Create the PRIVATE repository and push main
gh repo create CSD3131-Seam-Carving --private --source . --remote origin --push `
  --description "Content-aware image resizing using Dynamic Programming, Greedy Algorithms and Graph Cut — CSD3131 Algorithms Assignment 2."

# 4. Verify
gh repo view --web       # the Actions tab should show CI running on main
git status               # should say: Your branch is up to date with 'origin/main'
```

Without the CLI: create an empty **private** repository named `CSD3131-Seam-Carving` on github.com (no README, no .gitignore, no licence, because those already exist locally). Then run:

```powershell
git remote add origin https://github.com/<your-username>/CSD3131-Seam-Carving.git
git push -u origin main
```

Keep it private until the team agrees otherwise. The report needs a GitHub link the instructor can open. Before submission, either invite the instructor's account or make the repository public. Decide this as a team.

## 3. Inviting teammates

You need each person's **GitHub username** (not their email). Then, for each one:

```powershell
gh api -X PUT repos/<your-username>/CSD3131-Seam-Carving/collaborators/<their-username> -f permission=push
```

Or use the web UI: **Settings → Collaborators → Add people**. Each person must accept the email or GitHub notification. Write the usernames into `.github/CODEOWNERS` once known.

| Member | GitHub username | Invited | Accepted |
|---|---|---|---|
| FF Fauzan | `Qayzan` | owner | — |
| YN Yarni | `nursyazliyarni` | ☑ 2026-10-27 | ☐ |
| HM Hwee Min | `lhweemin` | ☑ 2026-10-27 | ☐ |
| WN Wani | `waneyyj` | ☑ 2026-10-27 | ☐ |
| KO Kervin | `kervino` | ☑ 2026-10-27 | ☐ |
| RB Rifqah | `riffyyyy` | ☑ 2026-10-27 | ☐ |

Usernames were taken from the collaborators of our Assignment 1 repository and invited with write (push) access. Check who has accepted with `gh api repos/Qayzan/CSD3131-Seam-Carving/invitations --jq ".[].invitee.login"` (anyone still listed has not accepted yet).

## 4. Protecting `main`

**Plan check first.** For a **private** repository owned by a **personal** account, GitHub only offers branch protection and rulesets on **GitHub Pro** or higher. On GitHub Free they apply only to public repositories ([about protected branches](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-protected-branches/about-protected-branches), [about rulesets](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-rulesets/about-rulesets)). Students can usually get Pro free through the GitHub Student Developer Pack. Check your account's plan under **Settings → Billing and plans**.

If available: **Settings → Rules → Rulesets → New branch ruleset**:

- Target: `main` (Default branch)
- ✅ Restrict deletions
- ✅ Block force pushes
- ✅ Require a pull request before merging (1 approval; dismiss stale approvals)
- ✅ Require status checks to pass → add **"Lint and test (Python 3.10)"** and **"Lint and test (Python 3.12)"**. These names appear after CI has run once.
- (Optional, after CODEOWNERS is filled in) ✅ Require review from Code Owners

If protection is **not** available on your plan, the same rules still apply by agreement (see CONTRIBUTING.md). CI still runs on every pull request, and a failing check stays visible.

## 5. Everyday commands

```bash
git switch main && git pull                 # update
git switch -c feature/hm-greedy             # start your branch (first time)
git switch feature/hm-greedy && git merge main   # bring it up to date
git push -u origin feature/hm-greedy        # publish it
gh pr create --base main --fill             # open a PR (or use the website)
```

## 6. A note on OneDrive

The local copy currently sits inside a OneDrive-synced folder, like Assignment 1 did. OneDrive can occasionally clash with git's internal files. If you see odd errors (locked `.git/index.lock`, duplicated files named `…-LAPTOP`), clone a working copy to a non-synced folder such as `C:\dev\CSD3131-Seam-Carving` and treat GitHub as the source of truth.
