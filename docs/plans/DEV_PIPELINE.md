# Development pipeline

Agreed 2026-11-27 (D678). The owner asked for the most suitable pipeline and answered "ตามแนะนำ". It keeps every
rule in [AGENTS](../../AGENTS.md) and adds the changes marked **new**.

## Stages

| Stage | What | Gate |
|---|---|---|
| 0. Sort | **Question:** answer with measured numbers. **UI fix with no result change:** go to 2. **Engine behaviour:** go to 1. **Owner's check result:** record the words verbatim and close the item | — |
| 1. Contract | Sources → contract with options and rows → every option measured with temporary patches and probes; callgrind when the hot path moves | The owner chooses |
| 2. Code | Rows first, each with a forcing check; `core/` imports nothing, files ≤ 500 lines, no uncited default | — |
| 3. Local ladder | (a) the touched test group, seconds; (b) **new:** the touched groups under `build/sanitizer` when a commit adds or edits tests; (c) `check`; (d) `multra-cli 42` byte-identical when `core/` moved; (e) callgrind on the hot path; (f) `multra-check-docs` | All pass |
| 4. Record | RECORD row and index line, PROGRESS (≤ 10), NEXT, the Windows sheet when the owner must look | `check-docs` |
| 5. Push and CI | **New:** every push to `claude/**` runs `early (sanitizer)` (`early.yml`), so a memory error shows before a pull request exists; the full matrix (`native.yml`) runs on the pull request and on main | Green |
| 6. Package | **New:** one *Package binaries* run per owner sitting, carrying every fix waiting for a look, named in the sheet by run and commit | — |
| 7. Owner on Windows | **New:** at most five items per round. Look first at what Linux cannot see; geometry Linux can check is tested ([UI_GEOMETRY](../reference/UI_GEOMETRY.md)) | The owner's words |
| 8. Merge | **New:** one pull request per topic, opened when its decisions close (about 5–10 D-rows), not one per branch lifetime | CI green; the owner merges |

## Why each new step

- **CI on push.** BA263's use-after-free (written with D661 on 2026-11-25) was caught only when PR #232 opened,
  two days and dozens of commits later.
  - The first version ran the full matrix on every push. A review on 2026-11-27 found two problems:
    - each push ran twice (runs 782 and 783 on one commit);
    - `cancel-in-progress` cancelled Windows jobs before they finished (runs 780 and 781).
  - Now only the sanitizer runs on push. It is the job that catches this class of defect, at about a sixth of the
    cost, under a job name of its own.
- **Sanitizer locally.** The same defect fails `build/sanitizer` in seconds once that tree is built. A first build
  takes about 10 minutes and 11 GB.
- **One package per sitting.** Packages #147–#155 were nine builds of about an hour each in one week; the owner
  had to follow which run carried which fix.
- **Five items per round.** The Windows sheet holds 14 items, and the carved-out looks about 30 more.
- **One pull request per topic.** PR #232 carried 69 commits, 208 files and about 13,000 added lines.
