# OpenApoc Hotkeys

OpenApoc eventually aims to implement all the controls from the original, as well as improve interface usability by introducing new hotkeys that make sense (things like mouse scroll for lists etc.). So, eventually you can expect all hotkeys from the original manual to work, in addition to new ones. For now, however, refer to this file to find out what hotkeys do what, as well as to learn about debug keys (cheats).

## General UI

- [Mousewheel] Scroll lists
- [Esc] Go back, close form, "Cancel" option
  - For example, "OK" button in screens, "Cancel" or "No" option in dialogues
- [Enter] Go forward, "Confirm" option, first option
  - For example, "OK" option in screens, "Confirm" or "Yes" option in dialogues, real time in briefing
- [Space] Skip, second (non-cancel) option
  - For example, "No" in "Yes/No/Cancel" dialogue, equip button in squad assignment, checkbox to pause when event happens again, turn based in briefing

## General

- [Middle Click] Move camera to location
- [Arrows] Move camera around
- [WASD] Move camera around
- [Tab] Toggle map
- [Space] Pause/Resume time (including TB)
- [Esc] Options menu
- [C] Toggle follow mode
- [M] Show log
- [Home] Zoom to last event
- [PrintScreen] Save a screenshot (`screenshotNNN.png` in the working directory)

## Ufopaedia

- [Up] / [Down] Next / previous section
- [Right] / [Left] Next / previous topic
- [Esc] / [Enter] Close

## Cityscape

### Vanilla controls note

- Vanilla city controls were not very consistent and quite limited. For starters, right-clicking mouse in city moved map, while in battle, it didn't. There were no ways to quickly open frequently used screens (namely equipment, location, base screens) without clicking their buttons, and when using Alt/Shift hotkeys, they acted as if you clicked a corresponding button, meaning that if you missed your selection mode changed.
- Instead, OpenApoc introduces a new, consistent control scheme, which is explained in the next two sections.
- For those who prefer to still use the vanilla control scheme, there is a feature to disable in the options menu called "Improved city control scheme". When disabled, vanilla control scheme will apply:
  - Right click will move screen to cursor
  - Alt+LMB will order vehicle attack
  - Shift+LMB will order moving to building
  - Left-clicking on building will always open building screen
  - No mouse controls introduced by OpenApoc will function

### Mouse

- [Ctrl]
  - when giving move orders to Agents, forces Agents to move on foot (never call a taxi), and allows use of personal teleporter
  - when giving move orders to Vehicles, allows manual use of teleporter
  - when giving attack orders forces Vehicles to attack their target (instead of recovering UFOs or escorting owned vehicles)
  - note that in vanilla, attacking owned vehicles was impossible, as such, a feature toggle is required for it to work (on by default)
- [Alt] Opens ufopaedia
- [Alt/Shift] + [Click] orders vehicles directly, Alt+Shift targets buildings and Shift targets vehicles or locations, while left click is aggressive and right is non-aggressive
- [Alt] + [Left Click] Open ufopaedia screen for the object (vehicle type, building function or even who fired the projectile!)
- [Alt] + [Right Click] Open ufopaedia screen for object's owner (be that vehicle or building)
- [Alt] + [Shift] + [Left Click] Order attack building
- [Alt] + [Shift] + [Right Click] Order goto building
- [Shift] + [Left Click] Order attack vehicle
- [Shift] + [Right Click] Order follow vehicle / goto location
- [Left Click]
  - Issues orders
  - Base screen for building that contains a base
  - Building screen for buildings
- [Right Click]
  - Building screen for buildings

### Mouse clicks on vehicle / agent icons

- [Shift] + [Right Click] Open location screen
- [Ctrl] + [Shift] + [Right Click] Open equipment screen
- [Alt] + [Shift] + [Right Click] Open equipment screen

### Mouse unit selection

- [Ctrl] makes selection additive
- [Left Click] Select agent/vehicle (as the only object selected)
- [Ctrl] + [Left Click] Add agent/vehicle to selection and make it first in the list
- [Right Click] Remove agent/vehicle from selection

### Keyboard

- [0], [1] ... [5] Control time
- [N] Manual control (if using vanilla scheme then [M] is the manual control key and there is no way to open message log, as per vanilla)

### Debug (cheats)

- [F1] Debug hotkeys are toggled on/off
- [Ctrl] + [Alt] + [Shift] + [Left Click] destroys scenery
- [Ctrl] + [Alt] + [Shift] + [Right Click] collapses building
- [A] gives every vehicle weapon and ammo to current base
- [W] warp to alien dimension and back
- [T] toggle vehicle target markers
- [R] repairs all scenery
- [B] spawn UFO on base assault mission
- [U] spawns three crashed UFOs
- [X] crashes every vehicle on map
- [-] destroys every selected vehicle
- [P] regenerates dimension portals
- [PgUp] / [PgDown] displays only one layer of map, with other layers being transparent
- [F2] show road pathfinding map
- [F3] highlight walkmode, collapsing tiles, basement tiles
- [F4] show aliens in buildings on strategy map
- [F5] show vehicle paths (blue flying, yellow ground)
- [F6] dump voxelmap for line of fire to tileviewvoxels.png
- [F7] dump voxelmap for line of fire to tileviewvoxels.png (fast way, calculate 1/4 of points)
- [F8] dump voxelmap for line of sight to tileviewvoxels.png
- [F9] dump voxelmap for line of sight to tileviewvoxels.png (fast way, calculate 1/4 of points)
- [F10] highlight tube in city
- [F11] highlight roads in city
- [F12] highlight hills in city
- [Numpad 1379] show only roads/tubes that have an outgoing connection in this direction
- [Numpad 28] show only tubes that have an outgoing connection down [2] or up [8]
- [Numpad 0] show all roads/tubes again
- [Numpad 5]
  - For tubes, switch between showing only tiles with defined tube passability, or to also include tiles belonging to buildings
  - For roads, switch between showing only tiles marked as "road", or to also include tiles marked with road direction
  - For hills, switch between showing only tiles marked as "road", or to also include tiles marked with hill direction
- Debug tile cursor: [W][A][S][D] nudge the cursor north/west/south/east, [R] / [F] move it up / down a level. Keys claimed by the debug bindings above take priority (here: [W], [A], [R])

#### Debug view flags

The cityscape debug keys above toggle these flags in `CityTileView`:

- `DEBUG_FORCE_ALIEN_DIMENSION` - [W] - while set, warps to the alien dimension and holds the view there (without triggering research unlocks); clearing it lets the normal auto-return logic warp back
- `DEBUG_SHOW_VEHICLE_TARGETS` - [T] - draws a marker on every vehicle target
- `DEBUG_SHOW_ROAD_PATHFINDING` - [F2] - overlays the road pathfinding map
- `DEBUG_WALK_MODE_DISPLAY` - [F3] - cycles (0-5) through highlight modes for walk mode, collapsing tiles and basement tiles
- `DEBUG_SHOW_ALIEN_CREW` - [F4] - shows aliens present in buildings on the strategy map
- `DEBUG_SHOW_VEHICLE_PATH` - [F5] - draws vehicle paths (blue flying, yellow ground)
- `DEBUG_SHOW_TUBES` - [F10] - highlights people tubes
- `DEBUG_SHOW_ROADS` - [F11] - highlights roads
- `DEBUG_SHOW_HILLS` - [F12] - highlights hills
- `DEBUG_ISOLATED_LAYER` - [PgUp] / [PgDown] - renders only the selected map layer opaque (-1 shows all layers)
- `DEBUG_CONNECTION_FILTER` - [Numpad 1379/28] - shows only roads/tubes with an outgoing connection in the selected direction (-1, via [Numpad 0], shows all)
- `DEBUG_STRICT_TILE_FILTER` - [Numpad 5] - switches between showing only tiles with a defined type and also including building or direction-marked tiles

## Base screens

### Debug (cheats)

- [F10]
  - On base view: finish all facilities
  - On research screen: mark project as requiring just 100 more points (basically complete project at next update if you have at least 2 people assigned)

## Vehicle equipment

- [Shift] makes item auto-equip into first available slot, or auto-remove to base stores

## Agent equipment

- Usual selection controls apply, with option to de-select with right click as well
- [Shift] makes item auto-equip into first available slot, or auto-remove to base stores / ground
- [Ctrl] makes you remove clip when clicking on weapon
- [1] ... [0] applies equipment template to every selected agent
- [Ctrl] + [1] ... [0] remembers current agent's equipment set as a template

## Battlescape

### Mouse

- [Alt]
  - When giving moving orders, makes unit keep facing to the target (making unit strafe or move backwards)
  - When firing at a tile, makes the shot aim at the ground of the tile, rather than at unit's level
- [Shift] Turns cursor into attack mode
- [Mousewheel] Change map levels
- [Left Click]
  - Order unit to execute action at cursor (move / throw / psi attack / teleport etc.)
  - Open probed unit's screen
  - Use item (weapon / teleport / prime grenade etc.)
- [Shift] + [Left Click] Order unit to fire at target tile, shot moving parallel to ground
- [Shift] + [Alt] + [Left Click] Order unit to fire at target tile, shot aimed at tile's ground
- [Right Click]
  - Turn towards cursor
  - Focus at enemy in RT
  - Use item's "auto" function (like prime grenade for impact)

### Mouse unit selection

- [Ctrl] makes selection additive
- [Left Click] Select unit (as the only unit selected)
- [Ctrl] + [Left Click] Add unit to selection and make it first in the list
- [Right Click] Remove unit from selection

### Keyboard

- [PgUp] / [PgDown] Change map levels
- [V] Toggle layering
- [F2] Prone mode
- [F3] Walk mode
- [F4] Run mode
- [F5] Cease fire mode
- [F6] Aimed mode
- [F7] Snap mode
- [F8] Auto mode
- [F9] Evasive mode
- [F10] Normal mode
- [F11] Aggressive mode
- [Backspace] Kneel mode
- [1..6] Select squad
- [Shift] + [1..6] Select unit in current squad (as the only unit selected)
- [Shift] + [Ctrl] + [1..6] Add unit to selection and make it first in the list
- [Ctrl] + [1..6] Assign current selection to squad N (units that don't fit in a full squad are left where they are)
- [Alt] + [1..6] Go through spotted enemies of unit
- [Enter] Open inventory
- [[] Throw left hand item
- []] Throw right hand item
- [\] Drop left hand item
- ['] Drop right hand item
- [Y] Confirm priming
- [N] Cancel priming
- [E] End your turn
- [Ctrl] + [S] Open save menu
- [L] Open load menu
- [J] Make unit jump (down from a cliff)

### Debug (cheats)

- [F1] Debug hotkeys are toggled on/off, when they are on some of the normal hotkeys are off
- [Middle Click] Activate teleportation mode for unit regardless of whether it holds a charged teleporter
- [E] Force end current turn in TB mode
- [Q] Order AI prone stance for units at cursor
- [R] Reveal whole map and show debug lines for which unit sees which unit
- [S] Stun units
  - with [Ctrl] held will affect small area around cursor, without will affect only unit under cursor
  - with [Shift] held will affect everything except cursor
  - with [Shift] and [Ctrl] held will affect everything except what's in small area around cursor
- [K] Same as [S] but removes units from map (units count as retreated)
- [P] Lower morale of every unit to the point where they will eventually suffer a low morale event
- [Shift] + [P] gives every unit 0 psi defense and 100 psi energy/attack
- [H] Restore stats of every unit, heals stun damage and fatal wounds
- [T] Restore TU of every player unit
- [F] Re-link support lines for battlescape map parts
- [Numpad 0] Spawn vortex mine explosion at cursor
- [Numpad 1-9] Spawn a shot with over 9000 damage at cursor in specified direction (5 being "down")
- [F6] dump voxelmap for line of fire to tileviewvoxels.png
- [F7] dump voxelmap for line of fire to tileviewvoxels.png (fast way, calculate 1/4 of points)
- [F8] dump voxelmap for line of sight to tileviewvoxels.png
- [F9] dump voxelmap for line of sight to tileviewvoxels.png (fast way, calculate 1/4 of points)
- Debug tile cursor: [W][A][S][D] nudge the cursor north/west/south/east, [R] / [F] move it up / down a level. Keys claimed by the debug bindings above take priority (here: [S], [R], [F])
