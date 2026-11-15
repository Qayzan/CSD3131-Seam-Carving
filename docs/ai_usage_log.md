# AI usage log

The brief requires an **AI Usage Declaration** in the report: "specify how AI tools were utilized (e.g., during design, implementation, or report writing) and provide the main prompts used." Every member adds an entry whenever an AI tool meaningfully shaped design, code, tests or text. Short and honest is best. The report section is assembled from this table.

| Date | Member | Tool | Stage | What it was used for | Main prompt (summary or quote) | What we checked / changed ourselves |
|---|---|---|---|---|---|---|
| 2026-10-27 | FF | Claude (Anthropic), Cowork | Design, repository set-up | Read the brief and rubric; proposed the Python/NumPy/OpenCV/pytest architecture; created the repository scaffolding (structure, CI, templates, CONTRIBUTING, docs); drafted the Q1 DP design, shared interface contract and requirement checklist | "CSD3131 Assignment 2 — Fauzan's Dynamic Programming Implementation & GitHub Repository Setup … I am responsible only for Question 1 … establish a professional, scalable GitHub repository and shared project architecture … Begin with Milestone 1." | Worked DP example verified by brute-force enumeration (all 95 seams). Team names/IDs copied from Assignment 1 repo — to be confirmed by the team. Design decisions (energy, tie rule, order of removals) open for team review |
| 2026-11-15 | FF | Claude (Anthropic), Cowork | Integration review, implementation, testing, report writing | Audited every branch; implemented the C++ shared foundation (interfaces, stb image I/O, Sobel energy incl. separable and divide-and-conquer versions, seam operations, carver), its tests and the `seam_carver` demo; drafted the item 7 (Q2(b)) section | "Complete FF's C++ foundation … inspect ALL branches first … implement interfaces.hpp, image_utils, energy, seam_ops, carver, src/main.cpp … test everything … item 7 divide and conquer for Sobel … do not push or merge without asking me" | Built with CMake and ran CTest (6/6 programs pass; also under AddressSanitizer/UBSan); Sobel checked against an independent reference and hand calculations; integration with HM's greedy and KO's seam extraction tested on a scratch branch and on real photos; item 7 timings measured with `seam_carver --bench-sobel` |

## How to write an entry

- **Stage:** design / implementation / testing / debugging / report writing / presentation.
- **Main prompt:** paste the key prompt or a faithful one-line summary.
- **What we checked:** how you verified or changed the output (tests run, proof checked by hand, rewrote section …). This shows understanding, and the project quiz will test it.
