---
name: ci-triage
description: Fetch and classify a failed GitHub Actions job for this repo — distinguish out-of-memory kills, six-hour timeouts, configure errors and genuine test failures, and identify what was in flight. Use whenever a CI job fails.
---

# CI triage

## Fetching logs

`gh run view --log` refuses while the run is still in progress. This always works:

```bash
JID=$(gh run view <run-id> --json jobs -q '.jobs[]|select(.name=="<job>")|.databaseId')
gh api --allow-escape-sequences "repos/fair-acc/gr-digitizers/actions/jobs/$JID/logs" \
  | sed 's/\x1b\[[0-9;]*[a-zA-Z]//g' > job.log
```

Without `--allow-escape-sequences` the call fails; without the `sed` the log is unreadable.

## Classification

| signature                                                            | cause                                             |
| -------------------------------------------------------------------- | ------------------------------------------------- |
| `exit code 137` + "runner has received a shutdown signal"            | OOM — the kernel killed the runner                |
| `Build` step with **no conclusion** at all                           | OOM so hard the step never reported               |
| conclusion `cancelled` at ~360 min                                   | six-hour job timeout                              |
| `CMake Error: generator : Ninja / Does not match ... Unix Makefiles` | a cached `_deps` sub-build from another generator |
| `***Failed <n> sec`                                                  | the test's own bound, not a ctest timeout         |
| `***Timeout`                                                         | ctest `TIMEOUT` property                          |

## After an OOM

The runner has 4 cores and about 16 GB. Read the last twenty lines of the log: the compiles and links in flight there
are the peak. Sum their measured peak RSS (the `measure-build-cost` skill) before limiting the build's parallelism. A
FetchContent GR4 build, as in the emcc lanes, compiles GR4's own heavy units.

Grep `Hits:` for the ccache hit rate, which explains most duration differences between runs.

## gr-digitizers specifics

| signature                                                          | cause                                                                                    |
| ------------------------------------------------------------------ | ---------------------------------------------------------------------------------------- |
| gcc/clang lanes fail on a GR4 symbol, emcc passes (or the reverse) | the debs track GR4 `main`, emcc builds the pin: GR4 `main` moved; bump the pin and adapt |
| `Install gnuradio4 packages` fails                                 | GR4's `latest` release is being republished; re-run later                                |
| `DemoRda3Server.jar is still a Git LFS pointer`                    | LFS fetch failed                                                                         |
| a bot step red, lane green                                         | the bots never gate; read the sticky comment                                             |
