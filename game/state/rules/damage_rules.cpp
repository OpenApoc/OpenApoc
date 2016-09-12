#include "game/state/rules/damage.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

template <> const UString &StateObject<DamageModifier>::getPrefix()
{
	static UString prefix = "DAMAGEMODIFIER_";
	return prefix;
}

template <> const UString &StateObject<DamageModifier>::getTypeName()
{
	static UString name = "DamageModifier";
	return name;
}

template <>
sp<DamageModifier> StateObject<DamageModifier>::get(const GameState &state, const UString &id)
{
	auto it = state.damage_modifiers.find(id);
	if (it == state.damage_modifiers.end())
	{
		LogError("No damage modifier type matching ID \"%s\"", id.cStr());
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<DamageType>::getPrefix()
{
	static UString prefix = "DAMAGETYPE_";
	return prefix;
}

template <> const UString &StateObject<DamageType>::getTypeName()
{
	static UString name = "DamageType";
	return name;
}

template <> sp<DamageType> StateObject<DamageType>::get(const GameState &state, const UString &id)
{
	auto it = state.damage_types.find(id);
	if (it == state.damage_types.end())
	{
		LogError("No damage type type matching ID \"%s\"", id.cStr());
		return nullptr;
	}
	return it->second;
}
}