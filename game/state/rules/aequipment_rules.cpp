#include "game/state/gamestate.h"
#include "game/state/rules/aequipment_type.h"

namespace OpenApoc
{
	AEquipmentType::AEquipmentType()
		:store_space(0) {};

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
			LogError("No aequipement type matching ID \"%s\"", id.c_str());
			return nullptr;
		}
		return it->second;
	}
}