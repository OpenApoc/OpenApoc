#include "framework/data.h"
#include "framework/framework.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/battlemapsector.h"
#include "library/strings_format.h"
#include "tools/extractors/common/ufo2p.h"
#include "tools/extractors/extractors.h"

namespace OpenApoc
{

void InitialGameStateExtractor::extractFacilities(GameState &state) const
{
	auto &data = this->ufo2p;
	LogInfo("Number of facility strings: %u", (unsigned)data.facility_names->count());
	LogInfo("Number of facility data chunks: %u", (unsigned)data.facility_data->count());

	// Start at 2, as 'earth' and 'corridor' are handled specially, this aren't really 'facilities'
	// in openapoc terms
	for (unsigned i = 2; i < data.facility_names->count(); i++)
	{
		UString id = data.getFacilityId(i);
		auto f = data.facility_data->get(i);

		LogInfo(
		    "Facility %d: %s cost %d image_offset %d size %d build_time %d maint %d capacity %d", i,
		    id, (int)f.cost, (int)f.image_offset, (int)f.size, (int)f.build_time,
		    (int)f.maintainance_cost, (int)f.capacity);
		LogInfo("u1 0x%04x u2 0x%04x", (unsigned)f.unknown1, (unsigned)f.unknown2);

		auto facilityType = mksp<FacilityType>();
		facilityType->name = data.facility_names->get(i);
		facilityType->buildCost = f.cost;
		facilityType->buildTime = f.build_time;
		facilityType->weeklyCost = f.maintainance_cost;
		// 12 constitution points repair for vehicles
		facilityType->capacityAmount = f.capacity == 1 ? 12 : f.capacity;
		facilityType->size = f.size;
		facilityType->sector = i - 2 + 16 + 15;
		facilityType->sprite = fw().data->loadImage(
		    format("PCK:xcom3/ufodata/base.pck:xcom3/ufodata/base.tab:%d:xcom3/ufodata/base.pcx",
		           (int)f.image_offset));

		state.facility_types[id] = facilityType;
	}
}

} // namespace OpenApoc
