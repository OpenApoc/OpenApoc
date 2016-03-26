#include "framework/data.h"
#include "framework/framework.h"
#include "tools/extractors/extractors.h"

#include "game/city/building.h"

namespace OpenApoc
{

#pragma pack(push, 1)
struct bld_file_entry
{
	uint16_t name_idx;
	uint16_t x0;
	uint16_t x1;
	uint16_t y0;
	uint16_t y1;
	uint16_t unknown1[95];
	uint16_t owner_idx;
	uint16_t unknown2[12];
};
#pragma pack(pop)

static_assert(sizeof(struct bld_file_entry) == 226, "Unexpected bld_file_entry size");

void InitialGameStateExtractor::extractBuildings(GameState &state, UString bldFileName,
                                                 sp<City> city)
{
	auto &data = this->ufo2p;

	auto fileName = "xcom3/ufodata/" + bldFileName + ".bld";

	auto inFile = fw().data->fs.open(fileName);
	if (!inFile)
	{
		LogError("Failed to open \"%s\"", fileName.c_str());
	}
	auto fileSize = inFile.size();
	auto bldCount = fileSize / sizeof(struct bld_file_entry);

	LogInfo("Loading %lu buildings from %s", (unsigned long)bldCount, fileName.c_str());

	for (unsigned i = 0; i < bldCount; i++)
	{
		struct bld_file_entry entry;
		inFile.read((char *)&entry, sizeof(entry));

		auto b = mksp<Building>();
		b->name = data.building_names->get(entry.name_idx);
		b->owner = {&state, data.get_org_id(entry.owner_idx)};
		// Our rects are exclusive of p2
		b->bounds = {entry.x0, entry.y0, entry.x1 + 1, entry.y1 + 1};
		auto id =
		    UString::format("%s%s", Building::getPrefix().c_str(), canon_string(b->name).c_str());

		city->buildings[id] = b;
	}
}

} // namespace OpenApoc
