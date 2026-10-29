---
name: wasm-build
description: Configure and run the local Emscripten/WebAssembly build of gr-digitizers as the emcc CI lanes do, including the tests. Use when reproducing an emcc CI failure.
---

# WASM build

The emcc lanes build without picoscope and timing. GR4 then comes from FetchContent at the pinned `GIT_TAG`, which is
heavy the first time.

```bash
source "${EMSDK:-$HOME/git/emsdk}/emsdk_env.sh"
emcmake cmake -S . -B build-wasm -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_PICOSCOPE=FALSE -DENABLE_TIMING=FALSE -DDISABLE_EXTERNAL_DEPS_WARNINGS=ON \
  -DCMAKE_CROSSCOMPILING_EMULATOR="$(which node)" -DGNURADIO_PARSE_REGISTRATIONS_TOOL_CXX_COMPLILER=g++-15
cmake --build build-wasm --parallel 2
ctest --test-dir build-wasm --output-on-failure 2>&1 | head -c 200000
```

## Traps

- **System node.** Use the system `node` as the emulator, not emsdk's older one, because the tests need its threading.
- **Registration tool.** GR4's block-registration parser runs on the host, so it needs a host compiler
  (`GNURADIO_PARSE_REGISTRATIONS_TOOL_CXX_COMPLILER`; the misspelling is GR4's).
- **No host filesystem.** A test that reads a file needs `--embed-file` in its `CMakeLists.txt`.
- **Looping tests.** Redirect test output through `head -c`, because a looping WASM binary can fill the disk.
