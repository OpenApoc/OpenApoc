#include "game/state/gamestate.h"
#include "game/state/rules/aequipment_type.h"
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
		LogError("No aequipement type matching ID \"%s\"", id.cStr());
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
			LogError("No equipment set (score) matching ID \"%s\"", id.cStr());
			return nullptr;
		}
	}
	return it->second;
}

StateRef<AEquipmentType> AEquipment::getPayloadType()
{
	if (type->type == AEquipmentType::Type::Weapon && type->ammo_types.size() > 0)
	{
		return payloadType;
	}
	return type;
}

std::list<sp<AEquipmentType>> EquipmentSet::generateEquipmentList(GameState &state)
{
	std::list<sp<AEquipmentType>> output;

	if (weapons.size() > 0)
	{
		auto wd = weapons[randBoundsExclusive(state.rng, 0, (int)weapons.size())];
		output.push_back(wd.weapon);
		if (wd.clip)
		{
			for (int i = 0; i < wd.clip_amount; i++)
				output.push_back(wd.clip);
		}
	}
	if (grenades.size() > 0)
	{
		auto gd = grenades[randBoundsExclusive(state.rng, 0, (int)grenades.size())];
		for (int i = 0; i < gd.grenade_amount; i++)
			output.push_back(gd.grenade);
	}
	if (equipment.size() > 0)
	{
		auto ed = equipment[randBoundsExclusive(state.rng, 0, (int)equipment.size())];
		for (auto e : ed.equipment)
			output.push_back(e);
	}

	return output;
}

sp<EquipmentSet> EquipmentSet::getByScore(const GameState &state, const int score)
{
	for (auto &es : state.equipment_sets_by_score)
	{
		if (es.second->is_appropriate(score))
			return es.second;
	}
	LogError("No equipment set matching score %d", score);
	return nullptr;
}

sp<EquipmentSet> EquipmentSet::getByLevel(const GameState &state, const int level)
{
	for (auto &es : state.equipment_sets_by_level)
	{
		if (es.second->is_appropriate(level))
			return es.second;
	}
	LogError("No equipment set matching level %d", level);
	return nullptr;
}
}
