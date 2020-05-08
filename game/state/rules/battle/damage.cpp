#include "game/state/rules/battle/damage.h"
#include "game/state/gamestate.h"
#include "game/state/rules/doodadtype.h"

namespace OpenApoc
{

template <> const UString &StateObject<HazardType>::getPrefix()
{
	static UString prefix = "HAZARD_";
	return prefix;
}

template <> const UString &StateObject<HazardType>::getTypeName()
{
	static UString name = "HazardType";
	return name;
}

template <> sp<HazardType> StateObject<HazardType>::get(const GameState &state, const UString &id)
{
	auto it = state.hazard_types.find(id);
	if (it == state.hazard_types.end())
	{
		LogError("No hazard type matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

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
		LogError("No damage modifier type matching ID \"%s\"", id);
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
		LogError("No damage type type matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

int DamageType::dealDamage(int damage, StateRef<DamageModifier> modifier) const
{
	if (modifiers.find(modifier) == modifiers.end())
	{
		LogError("Do not know how to deal damage type to modifier %s!", modifier.id);
		return damage;
	}
	else
		return damage * modifiers.at(modifier) / 100;
}

sp<Image> HazardType::getFrame(unsigned age, int offset)
{
	if (fire)
	{
		// Explanation how fire frames work is at the end of battlehazard.h
		// Round stage to nearest 0,5
		int stage = (age + 2) / 5 * 5;
		// Get min and max frames for this stage
		int minFrame = clamp((stage - 5) / 10, 0, 11);
		int maxFrame = clamp((stage + 5 + 5) / 10, 0, 11);
		// Trunc offset if it's too big
		if (minFrame + offset > maxFrame)
		{
			offset = 0;
		}
		int frame = minFrame + offset;
		// Scale to the actual frames of the doodad (crude support for more/less frames)
		return doodadType->frames[frame * doodadType->frames.size() / 12].image;
	}
	else
	{
		if (age >= 2 * maxLifetime)
		{
			LogError("Age must be lower than max lifetime");
			return nullptr;
		}
		int frame = age * doodadType->frames.size() / (2 * maxLifetime);
		while (frame + offset >= (int)doodadType->frames.size())
			offset -= (doodadType->frames.size() - frame);
		return doodadType->frames[frame + offset].image;
	}
}

int HazardType::getLifetime(GameState &state)
{
	return randBoundsInclusive(state.rng, 2 * minLifetime, 2 * maxLifetime);
}
} // namespace OpenApoc
