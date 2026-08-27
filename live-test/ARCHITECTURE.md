# Live-Test Harness Architecture

## Goal

Build an automated, scriptable QA layer for the real OpenApoc executable.

The harness should drive real UI/gameplay flows, wait for expected game states, capture screenshots, logs, and saves, and produce machine-readable pass/fail artifacts. It should add a slower regression layer above CTest, not replace CTest.

## Split Of Responsibilities

The clean shape is three layers:

```text
framework/      generic runtime hooks and process services
game/harness/   game-aware harness API
live-test/      external QA runner, scripts, assertions, and artifacts
```

This is deliberately both framework and game code, but not two harnesses. The framework gives the harness somewhere safe to run each frame. The game harness owns the actual game-aware command surface. `live-test/` consumes that surface as an external QA tool.

## Framework Layer

The framework layer should stay generic. It should not know about `GameState`, forms, controls, city screens, battles, or UI stage types.

Expected framework changes:

- Add disabled-by-default harness config:
  - `Framework.Harness.Enable`
  - `Framework.Harness.Port`
  - `Framework.Harness.WarpCursor`
- Provide a neutral per-frame hook mechanism, for example:

```cpp
using FrameHook = std::function<void(Framework &)>;
void addFrameHook(FrameHook hook);
```

- Keep existing generic primitives available to the harness:
  - event injection through `Framework::pushEvent()`
  - current stage/status access
  - display size and resize
  - screenshot writing
  - clean quit through stage commands

The framework should not parse high-level harness commands such as `CONTROL BUTTON_NEWGAME` or `GS gamestate.player.balance`. Those belong above it.

## Game Harness Layer

`game/harness/` owns the harness API that the external QA runner talks to.

Suggested files:

```text
game/harness/harness_api.h
game/harness/harness_api.cpp
game/harness/harness_protocol.h
game/harness/harness_protocol.cpp
```

The module should be installed from `game/main.cpp`:

```cpp
up<Framework> fw(new Framework(UString(argv[0]), true));
HarnessApi::install(*fw);
fw->run(mksp<BootUp>());
```

`HarnessApi::install()` should check `Framework.Harness.Enable` and, if enabled, attach its polling function to the framework frame hook:

```cpp
void HarnessApi::install(Framework &fw)
{
	if (!Options::harnessEnable.get())
	{
		return;
	}
	static HarnessApi api(Options::harnessPort.get());
	fw.addFrameHook([](Framework &fw) { api.poll(fw); });
}
```

The game harness can know about game/UI concepts because it lives at the top of the runtime dependency graph. It may include game UI, forms, and state headers without forcing lower framework code to know about them.

## Protocol

The first protocol should be line-oriented and local-only. Commands should return exactly one line beginning with `OK` or `ERR`.

Core commands:

```text
STATUS
HELP
QUIT

CLICK <x> <y> [left|right|middle]
MOVE <x> <y>
DOWN <x> <y> [left|right|middle]
UP <x> <y> [left|right|middle]
SCROLL <x> <y> <dy> [dx]
KEY <name>
KEYDOWN <name>
KEYUP <name>
TEXT <string>

SCREENSHOT <path>
SAVE <path>
RESIZE <width> <height>

UI [filter]
CONTROLS [<id>]
CONTROL <id> [click|toggle|get|set <value>|item <N> ...]
GS <query>
ACTION <verb> [args...]
```

`STATUS`, raw input, screenshot, resize, and quit mostly wrap framework services. `UI`, `CONTROLS`, `CONTROL`, `GS`, and `ACTION` are game-aware commands.

Named `CONTROL` actions should be preferred for ordinary UI widgets because a missing control returns an error instead of silently clicking the wrong coordinate. Raw input should remain available for map tiles and runtime widgets that have no stable name.

## Probes

Scripts must not depend on C++ object layout. They should depend on stable probe names exposed through `GS <query>`.

Useful first probes:

- `stage.name`
- `stage.detail`
- `gamestate.current_city.exists`
- `gamestate.current_city.id`
- `gamestate.current_base.name`
- `gamestate.player.balance`
- `gamestate.messages.count`
- `log.errors`
- `log.warnings`
- `save.digest`

The query namespace is the compatibility contract. Internals can move later as long as these names keep their meaning.

## UI Discovery

The awkward part is discovering live forms and controls without scattering harness code through the forms toolkit.

Preferred approach:

- Add a small, generic forms observation hook if no existing API exposes active forms.
- Keep the hook neutral: forms report construction/destruction and visible/update/render activity.
- Keep command interpretation in `game/harness/`, not in `forms/`.

Acceptable minimal hook examples:

```cpp
using VisibleFormHook = std::function<void(const sp<Form> &)>;
void Form::addVisibleFormHook(VisibleFormHook hook);
```

or:

```cpp
const std::vector<sp<Form>> &UI::visibleForms();
```

The key rule is that forms may expose facts about live controls, but they should not own the harness protocol.

## Live-Test Runner

`live-test/` owns the actual QA product.

Suggested layout:

```text
live-test/
  README.md
  Vagrantfile
  bin/run
  scripts/
    boot-main-menu.json
    load-cityscape.json
  artifacts/
```

Runner responsibilities:

- Start OpenApoc under Xvfb/headless rendering.
- Pass deterministic config and `Framework.Harness.Enable=1`.
- Connect to the harness socket.
- Load scenario JSON.
- Send commands and wait for expected `OK`/`ERR` replies or probe values.
- Enforce timeouts.
- Capture screenshots, logs, saves, and probe snapshots.
- Write `result.json`.
- Quit the game cleanly.

Example script shape:

```json
{
  "name": "boot-main-menu",
  "fixture": {
    "config": {
      "Game.SkipIntro": true,
      "Game.ASyncLoading": false,
      "Config.Save": false
    }
  },
  "steps": [
    {"send": "STATUS", "until": {"field": "stage", "equals": "MainMenu"}, "timeout_ms": 10000},
    {"send": "CONTROLS", "expect": {"contains": "BUTTON_NEWGAME"}},
    {"send": "SCREENSHOT build/tas/boot-main-menu/screenshots/main-menu.png"},
    {"send": "QUIT"}
  ],
  "assertions": [
    {"probe": "stage.name", "equals": "MainMenu"},
    {"probe": "log.errors", "equals": 0},
    {"artifact": "main-menu.png", "nonblank": true}
  ]
}
```

## Review Story

The PR should be explainable as:

> OpenApoc gains a disabled-by-default harness API for live automated QA. Framework changes are limited to neutral runtime hooks and generic process services. Game-specific command/probe behavior lives in `game/harness/`. The external TAS runner, scripts, assertions, and artifacts live under `live-test/`.

That keeps the game harness powerful enough to be useful while avoiding a spread of protocol logic through the engine.

