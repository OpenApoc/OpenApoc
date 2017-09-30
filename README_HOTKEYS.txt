List of hotkeys implemented for OpenApoc version 0.0.49

Note:

OpenApoc eventually aims to implement all the controls from the original, as well as improve interface usability by introducing new hotkeys that make sense (things like mouse scroll for lists etc.). So, eventually you can expect all hotkeys from original manual to work, in addition to new ones. For now, however, refer to this file to find out what hotkeys do what, as well as to learn about debug keys (cheats).

General UI Hotkeys:
- [Mousewheel] Scroll Lists
- [Esc] Go back, close form, "Cancel" option
  For example, "OK" button in screens, "Cancel" or "No" option in dialogues
- [Enter] Go forward, "Confirm" option, first option
  For example, "OK" option in screens, "Confirm" or "Yes" option in dialogues, real time in breifing
- [Space] Skip, second (non-cancel) option
  For example, equip button in squad assignment, checkbox to pause when event happens again, turn based in briefing
  
General Hotkeys:
- [Middle Click] Move camera to location
- [Arrows] Move camera around
- [TAB] Toggle map
- [Space] Pause/Resume time (including TB)
- [Escape] Options menu
- [C] Toggle Follow Mode
- [M] Show Log
- [Home] Zoom to last event

Cityscape Mouse:
* [Ctrl] when giving order that includes moving, forces Agents to move on foot (never call a taxi), as well as allows Vehicles and Agents manual use of teleporter
* [Alt] Opens object's ufopaedia screens
* [Shift] Opens object's management screens
- [Alt] + [Left Click] Opens ufopaedia screen for the object (vehicle type, building function or who fired the missile)
- [Alt] + [Right Click] Opens ufopaedia screen for object's owner (be that vehicle or building)
- [Shift] + [Left Click] 
  - Equipment screen for vehicles
  - Base screen for building that contains a base
  - Base buy screen for non-owned building that house a base
  - Building screen for other buildings
- [Shift] + [Right Click] 
  - Location screens for vehicles
  - Building screen for buildings
- [Left Click]
  - Issues orders
  - Open building screen
- [Right Click]
  - Issues orders
  - Base screen for building that contains a base
  - Base buy screen for non-owned building that house a base
  - Building screen for other buildings

Cityscape Mouse Unit Selection:
* [Ctrl] makes selection additive
- [Left Click]
  - Select Agent/Vehicle (as the only object selected)
- [Ctrl] + [Left Click]
  - Add Agent/Vehicle to selection and make it first in the list
- [Right Click]
  - Remove Agent/Vehicle from selection

Cityscape Keyboard:
- ... (none yet)

Cityscape Debug:
- Debug hotkeys are always active
- [R] repairs all buildings
- [F6] dump voxelmap for line of sight to tileviewvoxels.png
- [F7] dump voxelmap for line of sight to tileviewvoxels.png (fast way, calculate 1/4 of points)
- [F8] dump voxelmap for line of fire to tileviewvoxels.png
- [F9] dump voxelmap for line of fire to tileviewvoxels.png (fast way, calculate 1/4 of points)
- [F10] highlight tube in city
- [F11] highlight roads in city
- [F12] show aliens in buildings on strategy map

Battlescape Mouse:
* [Alt] When giving moving orders, makes unit keep facing to the target (making unit strafe or move backwards)
		When firing at a tile, makes the shot aim at the ground of the tile, rather than at unit's level
* [Shift] Turns cursor into attack mode
- [Left Click]
  - Order unit to execute action at cursor (move / throw / psi attack / teleport etc.)
  - Open probed unit's screen
  - Use item (weapon / teleport / prime grenade etc.)
- [Shift] + [Left Click] 
  - Order unit to fire at target tile, shot moving parallel to ground
- [Shift] + [Alt] + [Left Click] 
  - Order unit to fire at target tile, shot aimed at tile's ground
- [Right Click]
  - Turn towards cursor
  - Focus at enemy in RT
  - Use item's "auto" function (like prime grenade for impact)

Battlescape Mouse Unit Selection:
* [Ctrl] makes selection additive
- [Left Click]
  - Select Unit (as the only unit selected)
- [Ctrl] + [Left Click]
  - Add Unit to selection and make it first in the list
- [Right Click]
  - Remove Unit from selection

Battlescape Keyboard:
- [PgUp]/[PgDown] Change map levels
- [V] Toggle layering
- [F2] Prone Mode
- [F3] Walk Mode
- [F4] Run Mode
- [F5] Cease Fire Mode
- [F6] Aimed Mode
- [F7] Snap Mode
- [F8] Auto Mode
- [F9] Evasive Mode
- [F10] Normal Mode
- [F11] Aggressive Mode
- [Backspace] Kneel Mode
- [1..6] Select squad
- [Shift] + [1..6] Select Unit in current squad (as the only unit selected)
- [Shift] + [Ctrl] + [1..6] Add Unit to selection and make it first in the list
- [Alt] + [1..6] Go through spotted enemies of unit
- [Enter] Open Inventory
- [[] Throw right hand item
- []] Throw left hand item
- ['] Drop right hand item
- [\] Drop left hand item
- [Y] Confirm priming
- [N] Cancel priming
- [E] End your turn
- [S] Open Save menu
- [L] Open Load menu
- [J] Make unit jump (down from a cliff)

Battlescape Debug:
- [F1] Debug hotkeys are toggled on/off, when they are on some of the normal hotkeys are off
- [Middle Click] Activate teleportation mode for unit regardless of wether it holds a charged teleporter
- [E] Force end current turn in TB mode
- [R] Reveal whole map and show debug lines for which unit sees which unit
- [S] Stun units 
  - with Ctrl held will affect small area around cursor, without will affect only unit under cursor
  - with Shift held will affect everything except cursor
  - with Shift and Ctrl held will affect everything except what's in small area around cursor
- [K] Same as [S] but removes units from map (units count as reterated)
- [P] Lower morale of every unit to the point where they will eventually suffer a low morale event
- [Shift] + [P] gives every unit 0 psi defense and 100 psi energy/attack
- [H] Restore stats of every unit, heals stun damage and fatal wounds
- [T] Restore TU of every player unit
- [N] Toggle notification popups
- [F] Re-link support lines for battlescape map parts
- [Numpad 0] Spawn vortex mine explosion at cursor
- [Numpad 1-9] Spawn a shot with over 9000 at cursor in specified direction (5 being "down")
- [F6] dump voxelmap for line of sight to tileviewvoxels.png
- [F7] dump voxelmap for line of sight to tileviewvoxels.png (fast way, calculate 1/4 of points)
- [F8] dump voxelmap for line of fire to tileviewvoxels.png
- [F9] dump voxelmap for line of fire to tileviewvoxels.png (fast way, calculate 1/4 of points)