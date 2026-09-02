# OpenApoc Development Guide

This document collects the day-to-day development workflow for OpenApoc. It is a companion to `README.md`, which covers user-facing setup and platform-specific dependency installation, and `CODE_STYLE.md`, which is the source of truth for coding style.

## Repository Setup

Clone the repository. Most third-party libraries are fetched and built by vcpkg from `vcpkg.json`, so CMake configuration needs `-DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake`.

OpenApoc needs data from a lawful copy of the original X-COM: Apocalypse release. Place `cd.iso` in `data/`, or set `CD_PATH` during CMake configuration to point at the ISO or extracted CD tree:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCD_PATH=/path/to/cd.iso
```

Do not commit `cd.iso`, original executables, extracted original assets, generated dumps of proprietary resources, or local build directories.

## Build Configuration

The normal out-of-tree CMake flow is:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

`RelWithDebInfo` is a good default for development because it keeps debug symbols while avoiding the very slow runtime cost of a full debug build. Use `Debug` when stepping through logic where optimization gets in the way:

```sh
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
```

Useful CMake options include:

- `BUILD_LAUNCHER=ON|OFF` to enable or skip the Qt launcher.
- `USE_SYSTEM_QT=ON|OFF` to use a system Qt installation instead of the vcpkg-provided package where supported.
- `ENABLE_TESTS=ON|OFF` to build or skip the test binaries.
- `EXTRACT_DATA=ON|OFF` to run the data extractor as part of the default target.
- `CD_PATH=/path/to/cd.iso` to override the default `data/cd.iso` path.
- `USE_PCH=ON|OFF` to enable precompiled headers.
- `LTO=ON|OFF` to request link-time optimization where supported.

On Linux and macOS, run the game from the repository root unless you have installed it into a system prefix. The runtime expects the `data/` directory to resolve from the current working directory:

```sh
./build/bin/OpenApoc
```

## Tests

Build tests with `ENABLE_TESTS=ON` and run them with CTest:

```sh
ctest --test-dir build
```

For a specific failing test, run the generated test executable directly from the build tree. Keep tests focused on behaviour and regression risk: small unit tests for isolated logic, broader tests for changes that touch serialization, data extraction, game state, city simulation, or battlescape rules.

## Formatting And Static Checks

Follow `CODE_STYLE.md` for C++ style, naming, indentation, formatting, and clang-tidy guidance. Do not duplicate or reinterpret those rules here.

Format changed C++ files with:

```sh
clang-format -i path/to/file.cpp path/to/file.h
```

When using a CMake build directory, the repository also provides formatting targets:

```sh
cmake --build build --target format-sources
```

If clang-tidy is configured in your build, use the `tidy` target or the repository tooling around `tools/lint-tidy.sh`.

## Debugging

Use a build with symbols for normal debugging:

```sh
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
gdb --args ./build-debug/bin/OpenApoc
```

On Linux and macOS, `RelWithDebInfo` is often enough for backtraces and crash investigation:

```sh
gdb --args ./build/bin/OpenApoc
```

On Windows, use Visual Studio's CMake integration or generated CMake projects and keep the working directory at the repository root so `data/` is found. Release builds can emit PDB files when `MSVC_PDB` is enabled.

For memory errors, prefer sanitizer builds where your compiler and platform support them:

```sh
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Sanitize
cmake --build build-asan
```

When debugging game behaviour, record the scenario, save file if one is safe to share, expected behaviour, observed behaviour, and whether the issue reproduces with a clean configuration. For data extraction issues, also record the source game version, ISO/source path, and relevant extractor logs.

## Code Changes

Keep patches small and reviewable. A good OpenApoc change usually has one behavioural purpose, touches the narrowest set of files needed, and includes a test or a clear manual reproduction note.

Every branch, commit, and pull request should be tied to a GitHub issue. If you want to do something, create an issue and go for it :)

Prefer existing engine patterns over new abstractions. The codebase already has conventions for `StateRef`, serialization, logging, game-state rules, UI forms, and extractor tables; use them unless the local pattern is the bug.

When changing data extraction, keep original-game inputs external and user-provided. Extractors may read a lawful local copy of the original game, but generated data that derives from proprietary assets should not be committed unless the project already has an established, legally acceptable pattern for that exact class of data.

## Clean-Room And Copyright Rules

OpenApoc itself is licensed under the GNU General Public License version 3, as provided in `LICENSE`. Contributions to OpenApoc must be compatible with that license.

The original X-COM: Apocalypse source code, executable code, game data, art, audio, video, text, and other assets are not licensed by GPLv3 and are not licensed as part of this repository. They remain proprietary material owned by their respective rights holders. The project requires users to supply their own lawful copy of the original game.

Treat reverse engineering and original-game compatibility work with a clean-room mindset:

- Do not copy original source, decompiler output, disassembly listings, proprietary assets, or generated dumps of original resources into the repository.
- Describe observed behaviour in your own words and implement it independently in OpenApoc's own code style and architecture.
- Keep notes factual: source version, method, observed values, offsets when appropriate, and confidence. Avoid laundering guesses into constants.
- Prefer small, testable behaviour changes over large rewrites that mix evidence, refactoring, and feature work.
- If a behaviour claim depends on original-game research, say so in the PR and explain how it was observed or derived without redistributing copyrighted material.
- Be careful with security as well as copyright: do not ask reviewers to run untrusted binaries, open unknown disc images, or execute tools downloaded from unofficial sources just to validate a patch.

As a practical copyright baseline, X-COM: Apocalypse was published in 1997 and has not had its original source released under an open-source license. Under the U.S. corporate-work term assumption, copyright protection would last 95 years from publication, through the end of 2092, with public-domain status no earlier than 1 January 2093 unless the law or ownership facts change. Other jurisdictions can differ, including life-plus-70 rules in Europe. Do not treat the original game as abandonware or public domain.

## Pull Requests

Before opening a PR:

- Link the issue the work is meant to resolve.
- Build locally.
- Run the relevant tests, or explain why they could not be run.
- Format changed C++ files according to `CODE_STYLE.md`.
- Keep original-game files and local build artifacts out of the diff.
- Explain the user-visible or developer-visible behaviour changed by the patch.
- For reverse-engineering-sensitive changes, explain the clean-room evidence without including proprietary source, binaries, or assets.

Large mixed PRs are hard to review. Split unrelated work into separate changes: build fixes, refactors, extractor changes, gameplay behaviour, UI changes, and documentation should normally stand on their own.
