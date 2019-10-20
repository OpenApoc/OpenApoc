#include "game/state/rules/city/ufopaedia.h"
#include "game/state/city/research.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

UfopaediaEntry::UfopaediaEntry() : data_type(Data::Nothing) {}

bool UfopaediaEntry::isVisible() const { return this->dependency.satisfied(); }

sp<UfopaediaEntry> UfopaediaEntry::get(const GameState &state, const UString &id)
{
	for (auto &cat : state.ufopaedia)
	{
		auto entry = cat.second->entries.find(id);
		if (entry != cat.second->entries.end())
			return entry->second;
	}
	LogError("No UFOPaedia entry matching ID \"%s\"", id);
	return nullptr;
}

const UString &UfopaediaEntry::getPrefix()
{
	static UString prefix = "PAEDIAENTRY_";
	return prefix;
}
const UString &UfopaediaEntry::getTypeName()
{
	static UString name = "UfopaediaEntry";
	return name;
}

} // namespace OpenApoc
