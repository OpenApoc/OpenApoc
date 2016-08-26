#include "game/state/message.h"
#include "city/building.h"
#include "city/vehicle.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

const Vec3<int> EventMessage::NO_LOCATION = {-1, -1, -1};

Vec3<int> EventMessage::getMapLocation(GameState &state) const
{
	if (location.empty())
	{
		return NO_LOCATION;
	}
	StateRef<Vehicle> vehicle = {&state, location};
	if (vehicle != nullptr)
	{
		return vehicle->getPosition();
	}
	StateRef<Building> building = {&state, location};
	if (building != nullptr)
	{
		Vec2<int> position = building->bounds.p0 + building->bounds.size() / 2;
		return {position.x, position.y, 0};
	}
	return NO_LOCATION;
}

}; // namespace OpenApoc
