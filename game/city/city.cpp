#include "game/city/city.h"
#include "game/city/building.h"
#include "game/organisation.h"
#include "game/city/buildingtile.h"
#include "framework/framework.h"
#include "game/resources/gamecore.h"

namespace OpenApoc
{

City::City(Framework &fw, GameState &state) : TileMap(fw, fw.rules->getCitySize())
{
	for (auto &def : fw.rules->getBuildingDefs()) {
		Organisation *owner = nullptr;
		for (auto &org : state.organisations) {
			if (org.def.getName() == def.getOwnerName()) {
				owner = &org;
			}
		}
		if (!owner) {
			LogError("No organisation found matching building \"%s\" owner \"%s\"",
			         def.getName().str().c_str(), def.getOwnerName().str().c_str());
			return;
		}
		this->buildings.emplace_back(def, *owner);
	}

	for (int z = 0; z < this->size.z; z++) {
		for (int y = 0; y < this->size.y; y++) {
			for (int x = 0; x < this->size.x; x++) {
				auto tileID = fw.rules->getBuildingTileAt(Vec3<int>{x, y, z});
				if (tileID == "")
					continue;
				Building *bld = nullptr;

				for (auto &b : this->buildings) {
					if (b.def.getBounds().withinInclusive(Vec2<int>{x, y})) {
						if (bld) {
							LogError("Multiple buildings on tile at %d,%d,%d", x, y, z);
						}
						bld = &b;
						for (auto &padID : fw.rules->getLandingPadTiles()) {
							if (padID == tileID) {
								LogInfo("Building %s has landing pad at {%d,%d,%d}",
								        b.def.getName().str().c_str(), x, y, z);
								b.landingPadLocations.emplace_back(x, y, z);
								break;
							}
						}
					}
				}

				auto &cityTileDef = fw.rules->getBuildingTileDef(tileID);
				auto tile =
				    std::make_shared<BuildingTile>(*this, cityTileDef, Vec3<int>{x, y, z}, bld);
				this->addObject(std::dynamic_pointer_cast<TileObject>(tile));
				tile->setPosition(Vec3<int>{x, y, z});
			}
		}
	}
	/* Sanity check - all buildings should at have least one landing pad */
	for (auto &b : this->buildings) {
		if (b.landingPadLocations.empty()) {
			LogError("Building \"%s\" has no landing pads", b.def.getName().str().c_str());
		}
	}
}

City::~City() {}

} // namespace OpenApoc
