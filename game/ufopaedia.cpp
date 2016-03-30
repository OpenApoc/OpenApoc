#include "game/ufopaedia.h"
#include "game/research.h"

namespace OpenApoc
{

const bool UfopaediaEntry::isVisible() const
{
	// No required research = always visible
	if (!this->required_research)
		return true;
	// Otherwise only visible if the research is complete
	return (this->required_research->isComplete());
}

} // namespace OpenApoc
