# CLAUDE.md (AGENTS.md) — gr-digitizers

> For LLM coding agents. gr-digitizers is a GR4 block library, and GR4's rules apply unchanged.
> This file adds only what differs here.

## 1 · Rules

GR4's [`CLAUDE.md`](https://github.com/fair-acc/gnuradio4/blob/main/CLAUDE.md) is authoritative: the working agreement,
design, naming, layout, documentation, C++23, GR4 conventions, testing, commits and the pre-submit checklist. Read it
first.

The local copies below are taken verbatim from GR4 `bd3d68bb` (2026-10-29) and are re-synced with each GR4 bump:

| File                            | Purpose                                                 |
| ------------------------------- | ------------------------------------------------------- |
| `CORE_DEVELOPMENT_GUIDELINE.md` | rationale                                               |
| `CORE_NAMING_GUIDELINE.md`      | naming                                                  |
| `CORE_AGENT_WORKFLOW.md`        | orchestration                                           |
| `CORE_REVIEW_GUIDELINE.md`      | review                                                  |
| `.claude/commands/`             | review commands and the refactor and fix-style commands |

Where these files name GR4 modules (`core`, `meta`, `algorithm`, `blocks/*`), the `blocklib/*` modules here count as
leaves. This file and the copies MUST NOT be changed without the author's express approval.

## 2 · Repository

- **Layout.**
  - Modules live in `blocklib/{picoscope,timing,opencmw}`. Each module has `include/fair/<module>/`, `src/` for its
    tools (`picoscope-cli`, `test-timing`) and `test/qa_*.{cc,cpp}`.
  - Keep each directory's file extension.
- **Options.**
  - `ENABLE_PICOSCOPE`, `ENABLE_TIMING`, `ENABLE_OPENCMW`.
  - `ENABLE_GR_DIGITIZERS_TESTING` builds the `qa_` tests.
  - `DISABLE_SENSITIVE_TESTS=1` at test time skips the timing-sensitive tests.
- **Hardware.**
  - Picoscope and timing tests that need a device skip themselves without one.
  - The OpenCMW tests use `CMW_NAMESERVER` and `CMW_DEVICE_HOST` when set.
  - A failure on a machine without hardware is real.
- **Where GR4 comes from.** `find_package(gnuradio4)` prefers an installed GR4. In CI that is the GR4 `latest` release
  debs in `/opt/gnuradio4-{gcc,clang}`, rebuilt from GR4 `main`.
  - Without an installed GR4, FetchContent builds GR4 at the `GIT_TAG` in `CMakeLists.txt`. Emscripten always uses
    this path.
  - A GR4 API change therefore reaches CI with GR4 `main`, not with the pin. Bump the pin together with the source change.
- **Toolchains:** GCC 15 (main, required checks), Clang 21, then Emscripten without picoscope and timing (the
  `wasm-build` skill).

## 3 · Building

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=g++-15 -DENABLE_GR_DIGITIZERS_TESTING=ON \
  [-DCMAKE_PREFIX_PATH=/opt/gnuradio4-gcc]
cmake --build build --parallel <N>
DISABLE_SENSITIVE_TESTS=1 ctest --test-dir build --output-on-failure
```

A FetchContent GR4 build compiles GR4's own translation units, which can exceed 7 GB each. Size `<N>` from measured
peak RSS (the `measure-build-cost` skill), not from the core count.

## 4 · Quality checks

- `./formatFiles.sh` runs the pinned formatters over the changed files, and `--all` over every file. It MUST run before
  each commit (the `restyle` skill).
- In CI, `ci.yml` runs GR4's quality bots from a checkout of GR4's `devtools/` at the pinned revision: formatting,
  naming, code-style, coverage, hot-paths and wasm-size.
  - Their scope is set in `.github/qualitybot.conf`.
  - The bots post sticky PR comments and statuses but never gate a merge.
  - Their history is kept on the `ci-metrics` branch.
- When a CI job fails, use the `ci-triage` skill.
