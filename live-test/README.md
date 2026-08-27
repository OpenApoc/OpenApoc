# Live Test Harness

This directory is for running OpenApoc through the actual game executable, not only through unit tests.

The intended harness is an automated, scriptable QA layer for the real game executable. The engine exposes an opt-in local command socket, and an external TAS-style runner drives that socket with scenario scripts while probes, screenshots, logs, and save artifacts provide pass/fail evidence.

The goal is to catch regressions in UI flow, rendering, save/load, and gameplay state that CTest unit tests cannot exercise.

## Layers

### Runtime Environment

`Vagrantfile` is the legacy starting point. It currently provisions a Linux VM, installs build and graphics dependencies, links local X-COM data into the guest, and builds the game.

The modern version should provide:

- A current Ubuntu or Debian guest.
- SDL2, Qt6, Boost, Vorbis, libunwind, CMake, and compiler packages.
- Xvfb and Mesa software OpenGL for headless rendering.
- Dummy audio for deterministic, non-interactive runs.
- The repository mounted at `/vagrant`.
- Original game data mounted locally, never committed.

Expected environment variables for harness runs:

```sh
SDL_AUDIODRIVER=dummy
LIBGL_ALWAYS_SOFTWARE=1
```

### Game Runner

The runner should execute the real game binary under a virtual display with the harness socket enabled:

```sh
xvfb-run -s "-screen 0 1280x720x24" \
  ./build/bin/OpenApoc \
  --Config.Save=false \
  --Framework.AudioBackends=null \
  --Framework.TargetFPS=60 \
  --Framework.FrameLimit=1800 \
  --Game.SkipIntro=true \
  --Game.ASyncLoading=false \
  --Framework.Harness.Enable=1 \
  --Framework.Harness.Port=17321
```

The runner process owns the script, timeout, assertion, and artifact policy. The game process owns the live command/probe surface.

### Engine Harness Socket

When `Framework.Harness.Enable=1`, OpenApoc listens on a loopback-only command socket. Replies are one line, beginning with `OK` or `ERR`.

Useful commands:

- `STATUS`: current stage, display size, mouse position, port, and stage detail.
- `CONTROLS` / `CONTROL <id> ...`: list and drive live named UI controls.
- `UI [filter]`: dump the live control tree with resolved rectangles.
- `CLICK`, `MOVE`, `DOWN`, `UP`, `SCROLL`, `KEY`, `TEXT`: raw input for nameless widgets.
- `GS <query>`: read game-state probes through the game-layer query hook.
- `SCREENSHOT <path>` and `SAVE <path>`: write debugging and state artifacts.
- `RESIZE`, `HELP`, and `QUIT`.

This is the engine-side control layer. It avoids host-level mouse automation while letting the QA runner address controls by name where possible, then fall back to raw input for map tiles and other nameless runtime widgets.

### TAS Script

A TAS script should contain fixture setup, socket commands, waits, and assertions.

Example:

```json
{
  "name": "load-cityscape",
  "fixture": {
    "data": "data",
    "cd": "data/cd.iso",
    "save": "tests/tas/fixtures/week1-city.zip",
    "config": {
      "Game.SkipIntro": true,
      "Game.ASyncLoading": false,
      "Config.Save": false
    }
  },
  "steps": [
    {"send": "STATUS", "until": {"field": "stage", "equals": "CityView"}, "timeout_ms": 10000},
    {"send": "GS gamestate.current_city.exists", "expect": {"equals": "true"}},
    {"send": "SCREENSHOT build/tas/load-cityscape/screenshots/city-loaded.png"}
  ],
  "assertions": [
    {"probe": "stage.name", "equals": "CityView"},
    {"probe": "gamestate.current_city.exists", "equals": true},
    {"probe": "log.errors", "equals": 0},
    {"artifact": "city-loaded.png", "nonblank": true}
  ]
}
```

### Command Injection

The runner should drive the game through the harness socket instead of using external mouse automation.

The socket commands create the same internal `KeyboardEvent`, `MouseEvent`, and `TextEvent` objects and queue them through `Framework::pushEvent()`. Named `CONTROL` actions should be preferred for normal UI widgets because they fail loudly when a control is absent. Raw input remains available for map tiles and other nameless widgets.

### Probes

Assertions should read from a small, stable probe surface rather than from arbitrary object internals. The current engine hook is `GS <query>`; over time, the query names should become the stable compatibility contract used by scripts.

Useful first probes:

- `stage.name`
- `stage.stack`
- `frame.number`
- `log.errors`
- `log.warnings`
- `gamestate.current_city.exists`
- `gamestate.current_base.name`
- `gamestate.player.balance`
- `ui.visible_form`
- `ui.focused_control`
- `screenshot.hash`
- `save.digest`

### Artifacts

Each run should write a directory under `build/tas/<script-name>/`:

```text
build/tas/load-cityscape/
  result.json
  log.txt
  probes.json
  screenshots/
  final-save.zip
```

The result file should be the machine-readable pass/fail summary. Screenshots and logs are for debugging.

## Determinism

The first version should disable async loading, disable config writes, use a fixed frame limit, run with dummy audio, and prefer save-file fixtures over clicking through long setup flows.

Pixel-perfect image comparisons should not be the first target. Start with nonblank screenshots, expected stage/state probes, log cleanliness, and save/load digests. Pixel stability can come later.

## Relationship To CTest

CTest remains the fast unit and serialization test layer. The live-test harness should add slower smoke and regression coverage through the real game executable.

The intended shape is:

```text
ctest                     # fast unit coverage
live-test run smoke       # real executable smoke coverage
live-test run regression  # selected TAS scripts
```

## Current Status

This directory currently contains the legacy Vagrant entry point and the harness design notes. The TAS runner, `TAS.*` config options, probe registry, scripts, and artifacts are not implemented yet.
