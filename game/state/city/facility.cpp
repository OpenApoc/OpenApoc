#include "game/state/city/facility.h"

namespace OpenApoc
{

Facility::Facility(StateRef<FacilityType> type) : type(type), pos(0, 0), buildTime(0) {}

}; // namespace OpenApoc
