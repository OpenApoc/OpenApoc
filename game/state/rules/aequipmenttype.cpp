#include "game/state/rules/aequipmenttype.h"
#include "game/state/gamestate.h"
#include "game/state/shared/aequipment.h"
#include "game/state/tilemap/tilemap.h"
#include <climits>

namespace OpenApoc
{

template <> const UString &StateObject<AEquipmentType>::getPrefix()
{
	static UString prefix = "AEQUIPMENTTYPE_";
	return prefix;
}

template <> const UString &StateObject<AEquipmentType>::getTypeName()
{
	static UString name = "AEquipmentType";
	return name;
}

template <>
sp<AEquipmentType> StateObject<AEquipmentType>::get(const GameState &state, const UString &id)
{
	auto it = state.agent_equipment.find(id);
	if (it == state.agent_equipment.end())
	{
		LogError("No aequipement type matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<EquipmentSet>::getPrefix()
{
	static UString prefix = "EQUIPMENTSET_";
	return prefix;
}

template <> const UString &StateObject<EquipmentSet>::getTypeName()
{
	static UString name = "EquipmentSet";
	return name;
}

template <>
sp<EquipmentSet> StateObject<EquipmentSet>::get(const GameState &state, const UString &id)
{
	auto it = state.equipment_sets_by_score.find(id);
	if (it == state.equipment_sets_by_score.end())
	{
		it = state.equipment_sets_by_level.find(id);
		if (it == state.equipment_sets_by_level.end())
		{
			LogError("No equipment set (score) matching ID \"%s\"", id);
			return nullptr;
		}
	}
	return it->second;
}

std::list<const AEquipmentType *> EquipmentSet::generateEquipmentList(GameState &state)
{
	std::list<const AEquipmentType *> output;

	if (weapons.size() > 0)
	{
		auto wd = weapons[randBoundsExclusive(state.rng, 0, (int)weapons.size())];
		output.push_back(wd.weapon.get());
		if (wd.clip)
		{
			for (int i = 0; i < wd.clip_amount; i++)
			{
				output.push_back(wd.clip.get());
			}
		}
	}
	if (grenades.size() > 0)
	{
		auto gd = grenades[randBoundsExclusive(state.rng, 0, (int)grenades.size())];
		for (int i = 0; i < gd.grenade_amount; i++)
		{
			output.push_back(gd.grenade.get());
		}
	}
	if (equipment.size() > 0)
	{
		auto ed = equipment[randBoundsExclusive(state.rng, 0, (int)equipment.size())];
		for (auto &e : ed.equipment)
		{
			output.push_back(e.get());
		}
	}

	return output;
}

sp<EquipmentSet> EquipmentSet::getByScore(const GameState &state, const int score)
{
	for (auto &es : state.equipment_sets_by_score)
	{
		if (es.second->isAppropriate(score))
			return es.second;
	}
	LogError("No equipment set matching score %d", score);
	return nullptr;
}

sp<EquipmentSet> EquipmentSet::getByLevel(const GameState &state, const int level)
{
	for (auto &es : state.equipment_sets_by_level)
	{
		if (es.second->isAppropriate(level))
			return es.second;
	}
	LogError("No equipment set matching level %d", level);
	return nullptr;
}

bool AEquipmentType::canBeUsed(GameState &state, StateRef<Organisation> owner) const
{
	if (owner == state.getPlayer() && !(state.current_battle && state.current_battle->skirmish) &&
	    !research_dependency.satisfied())
	{
		return false;
	}
	return true;
}

float AEquipmentType::getRoundsPerSecond() const
{
	return (float)TICKS_PER_SECOND / (float)fire_delay;
}

int AEquipmentType::getRangeInTiles() const { return range / (int)VELOCITY_SCALE_BATTLE.x; }

int AEquipmentType::getRangeInMetres() const { return range / 16; }
} // namespace OpenApoc
