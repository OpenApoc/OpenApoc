#pragma once
#include "game/state/battlemappart_type.h"
#include "game/state/stateobject.h"

namespace OpenApoc
{
class BattleMapTileset
{
  public:
	StateRefMap<BattleMapPartType> map_part_types;
};
}
