#include "game/state/rules/city/ufopaedia.h"
#include "game/state/city/research.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

UfopaediaEntry::UfopaediaEntry() : data_type(Data::Nothing) {}

bool UfopaediaEntry::isVisible() const { return this->dependency.satisfied(); }

template <>
sp<UfopaediaEntry> StateObject<UfopaediaEntry>::get(const GameState &state, const UString &id)
{
	auto it = state.ufopaedia_entries.find(id);
	if (it == state.ufopaedia_entries.end())
	{
		LogError("No UFOPaedia entry matching ID \"{0}\"", id);
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<UfopaediaEntry>::getPrefix()
{
	static UString prefix = "PAEDIAENTRY_";
	return prefix;
}
template <> const UString &StateObject<UfopaediaEntry>::getTypeName()
{
	static UString name = "UfopaediaEntry";
	return name;
}

} // namespace OpenApoc
