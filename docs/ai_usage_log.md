# AI usage log

The brief requires an **AI Usage Declaration** in the report: "specify how AI tools were utilized (e.g., during design, implementation, or report writing) and provide the main prompts used." Every member adds an entry whenever an AI tool meaningfully shaped design, code, tests or text. Short and honest is best. The report section is assembled from this table.

| Date | Member | Tool | Stage | What it was used for | Main prompt (summary or quote) | What we checked / changed ourselves |
|---|---|---|---|---|---|---|
| 2026-10-27 | FF | Claude (Anthropic), Cowork | Design, repository set-up | Read the brief and rubric; proposed the Python/NumPy/OpenCV/pytest architecture; created the repository scaffolding (structure, CI, templates, CONTRIBUTING, docs); drafted the Q1 DP design, shared interface contract and requirement checklist | "CSD3131 Assignment 2 — Fauzan's Dynamic Programming Implementation & GitHub Repository Setup … I am responsible only for Question 1 … establish a professional, scalable GitHub repository and shared project architecture … Begin with Milestone 1." | Worked DP example verified by brute-force enumeration (all 95 seams). Team names/IDs copied from Assignment 1 repo — to be confirmed by the team. Design decisions (energy, tie rule, order of removals) open for team review |

| 2026-10-27 | FF | Claude (Anthropic), Cowork | Implementation, testing | Milestone 2: implemented the shared modules (`interfaces`, `image_utils`, `energy`, `seam_ops`) and their pytest suites, following the design in `algorithm_design.md` and the contract in `interfaces.md` | "Continuing CSD3131 Assignment 2 … start Milestone 2 (shared utilities with tests) on branch feature/ff-dynamic-programming" | Ran ruff and pytest (120 passed). OpenCV energy cross-checked against an independent NumPy implementation and a hand-computed example; seam removal checked against a slow `np.delete` reference; six deliberate bugs injected to confirm the tests catch them |

## How to write an entry

- **Stage:** design / implementation / testing / debugging / report writing / presentation.
- **Main prompt:** paste the key prompt or a faithful one-line summary.
- **What we checked:** how you verified or changed the output (tests run, proof checked by hand, rewrote section …). This shows understanding, and the project quiz will test it.
