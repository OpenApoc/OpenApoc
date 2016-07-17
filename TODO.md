# OpenApoc 'TODO' list

Note, this is not a complete list, expect items to grow and spawn sub-items as they are progressed!

~~strikethrough~~ items are completed

## Core framework
- ~~Font loader~~
- ~~Palette loader~~
- ~~Sound effects~~
- ~~Music~~
- ~~SMK Video~~
- ~~PCX file loader~~
- ~~PCK image file loader~~
  - ~~'normal' PCK images~~
  - ~~'shadow' PCK images~~
  ' ~~'strategy map' PCK images~~
- ~~Accelerated renderer~~
  - ~~'Mobile' OpenGL|ES3 renderer~~

## 'GameState' serialization
- ~~Generate initial gamestate from Apoc files~~
- ~~Allow 'patching' gamestate~~
- ~~Save games~~
- ~~Load games~~
- Saved games can 'depend' on other GameState packages (e.g. Mods)
- Save GameState minimiser (IE only save the things that 'changed')
- GameState GUI editor

## TileMap
- ~~Use for CityMap~~
- Use for TacMap
- ~~Draw vehicles/agents~~
- ~~Projectile drawing~~
- ~~Projectile hit detection~~

## CityScape
- ~~Load CityMap~~
- ~~Load building descriptions~~
- Vehicles
  - ~~Flying vehicle pathfinding~~
  - Ground vehicle pathfinding
  - ~~Animated UFO drawing~~
  - ~~'crashed' UFOs~~
  - ~~Vehicle damage~~
  - Vehicle AI:
    - ~~AI Vehicles fire at enemies~~
    - Vehicle retaliation fire
	- Guided missiles
  - ~~AI Vehicles move between buildings~~
  - AI vehicles patrol
  - UFO AI Missions
    - Infiltration
    - Subversion
    - Escort
    - Return to portal when done
  - User-controllable vehicle commands:
    - ~~Goto Building~~
    - ~~Goto Location~~
    - Attack Building
    - ~~Attack Vehicle~~
      - ~~Prefer shooting at 'current target'~~
    - Goto crashed UFO
    - Goto alien dimension portal
    - ~~Different 'aggressiveness'~~
    - ~~Different 'height'~~
- Building destruction
  - Decide how building tiles are 'supported'
  - ~~Collapse tiles when support is lost~~
- ~~Building selection~~
- Vehicle selection
  - Outline selected or target vehicles
- ~~Player Vehicle status in UI~~
- Enemy vehicle status in UI
- Agent status in UI
- Organization status in UI
  - Loop through buildings when clicked
- ~~Mini base view~~
- ~~Purchase new base~~
- Show message history
- Show alien dimension
- Agent/people-tube pathfinding
- Organisation relationships
  - Relationship drop when under attack (separate attack mission from random hit)
  - Relationship drop/boost when attacking others
  - Relationship modifiers for allies
  - Organisations relation recovery

## City MiniMap
- ~~Show building tiles~~
- ~~Show vehicle tiles~~
- Show weapon paths
- Show vehicle mission targets
- Show recent alien alert circles
- Highlight selected vehicle
- Highlight selected organization's buildings
- Show organization relations in colour

## BaseScape
- ~~Draw current base~~
- ~~Draw city minimap~~
- ~~Draw mini base view~~
- ~~Build new facility~~
  - ~~Limit due to funds~~
  - ~~Limit due to research~~
  - ~~Limit due to facility size~~
- ~~Destroy Facility~~
  - ~~Disallow in-use labs~~
  - Disallow in-use stores
  - Disallow in-use living quarters
  - Disallow in-use alien containment
- ~~Research screen~~
  - ~~Assign research project~~
  - ~~Assign scientists~~
  - ~~Research project progression & completion~~
  - Show 'icon' for some topics (e.g. vehicle equipment) on selection screen
  - Assign engineering project
  - ~~Limit available research based on pre-requisite research~~
  - Limit available research based on required items
  - Limit available research based on required event (successful crashed ufo mission, entered alien dimension etc.)
- Hire new agents
  - ~~Agent stats generation~~
  - Create new agents at the end of each day
  - Agent hire UI
  - Agent living quarters limit
- ~~Vehicle equip screen~~
- Agent equip screen
- Market
  - Generate initial available items
  - Re-stock at the end of each week
  - Restrict items not yet available
  - Restrict items from hostile organizations
  - Item price changes based on next week stock
  - Item stock/price changes for xcom/alien items
- Inventory
  - Organisation-wide storage for AI (on the market)
  - Base storage for X-Com
  - Stores limit
- Alien containment
  - Current state tracking
  - Dispose UI

## UfoPaedia
- ~~Show category selection~~
- ~~Show individual topics~~
- ~~Progress topics with buttons~~
- ~~Progress categories with buttons~~
- ~~Show category list with '(i)' button~~
- ~~Limit categories vased on required research~~
- Show stats related to topic
  - ~~Organisation stats~~
  - Agent equipment stats
  - Vehicle equipment stats
  - Vehicle stats

## Current score/summary summary
- Show stats at end of day
- Show stats on cityscape UI button press
- Show stats at end of week
- Funding calculations

## Infiltration graph
- Show currently infiltrated org %age
- Show historical infiltration graph

## Tactical game
- Read Tactical maps
- Draw Tactical maps
- Agent movement
- Agent AI
- Enemy movement
- Enemy AI
- Spawn enemies
- Spawn agents
- Spawn neutrals
- Line-of-sight & fog-of-war
- Pathfinding
- End-of-mission summary
  - Alien equipment
  - Alien containment
  - Score
  - Loss of agents
  - Organisation relationship changes?
  - Loss of agent equipment / ammo

## Misc Gameplay
- Agent training
- Agent health/medbay use
- Agent experience
- Alien UFO production
- Alien Overspawn missions
- Alien progression
- Organisation funds simulation
- Organisation 'attack player base when hostile'
- Organisation 'raids/illegal flyer' attacks
- Organisation 'pay-off' screen
