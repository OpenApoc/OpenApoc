# Live Test Harness

This directory is for running OpenApoc through the actual game executable, not only through unit tests.

The intended harness is a TAS-style runner: a deterministic replay script drives the real SDL/OpenApoc event loop, while read-only probes collect facts from the running game and compare them with assertions. The goal is to catch regressions in UI flow, rendering, save/load, and gameplay state that CTest unit tests cannot exercise.

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

The runner should execute the real game binary under a virtual display:

```sh
xvfb-run -s "-screen 0 1280x720x24" \
  ./build/bin/OpenApoc \
  --Config.Save=false \
  --Framework.AudioBackends=null \
  --Framework.TargetFPS=60 \
  --Framework.FrameLimit=1800 \
  --Game.SkipIntro=true \
  --Game.ASyncLoading=false \
  --TAS.Script=tests/tas/load-cityscape.json \
  --TAS.Artifacts=build/tas/load-cityscape
```

The `TAS.*` options do not exist yet. They describe the proposed harness interface.

### TAS Script

A TAS script should contain fixture setup, an input timeline, and assertions.

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
  "timeline": [
    {"frame": 1, "action": "wait_for_stage", "stage": "CityView", "timeout": 600},
    {"frame": 601, "action": "screenshot", "name": "city-loaded.png"}
  ],
  "assertions": [
    {"probe": "stage.name", "equals": "CityView"},
    {"probe": "gamestate.current_city.exists", "equals": true},
    {"probe": "log.errors", "equals": 0},
    {"artifact": "city-loaded.png", "nonblank": true}
  ]
}
```

### Event Injection

The runner should inject events inside the engine instead of using external mouse automation.

OpenApoc already translates SDL input into internal framework events and queues them through `Framework::pushEvent()`. TAS replay should create the same internal `KeyboardEvent`, `MouseEvent`, and `TextEvent` objects at deterministic frames. That keeps tests close to the real game while avoiding host focus and window-manager failures.

### Probes

Assertions should read from a small, stable read-only probe registry rather than from arbitrary object internals.

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
