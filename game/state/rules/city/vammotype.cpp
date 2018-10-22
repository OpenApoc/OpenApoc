#include "game/state/rules/city/vammotype.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

const UString &VAmmoType::getPrefix()
{
	static UString prefix = "VEQUIPMENTAMMOTYPE_";
	return prefix;
}

const UString &VAmmoType::getTypeName()
{
	static UString name = "VAmmoType";
	return name;
}

sp<VAmmoType> VAmmoType::get(const GameState &state, const UString &id)
{
	auto it = state.vehicle_ammo.find(id);
	if (it == state.vehicle_ammo.end())
	{
		LogError("No vammo type matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}
} // namespace OpenApoc
