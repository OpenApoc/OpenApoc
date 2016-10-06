#include "game/state/gameevent.h"
#include "city/vehicle.h"
#include "library/strings_format.h"

namespace OpenApoc
{

GameEvent::GameEvent(GameEventType type) : Event(EVENT_GAME_STATE), type(type) {}

GameVehicleEvent::GameVehicleEvent(GameEventType type, StateRef<Vehicle> vehicle)
    : GameEvent(type), vehicle(vehicle)
{
}

UString GameVehicleEvent::message()
{
	switch (type)
	{
		case GameEventType::UfoSpotted:
			return tr("UFO spotted.");
		case GameEventType::UfoCrashed:
			return format("%s %s", tr("UFO crash landed:"), vehicle->name);
		case GameEventType::UfoRecovered:
			return format("%s %s", tr("Unmanned UFO recovered:"), vehicle->name);
		case GameEventType::VehicleLightDamage:
			return format("%s %s", tr("Vehicle lightly damaged:"), vehicle->name);
		case GameEventType::VehicleModerateDamage:
			return format("%s %s", tr("Vehicle moderately damaged:"), vehicle->name);
		case GameEventType::VehicleHeavyDamage:
			return format("%s %s", tr("Vehicle heavily damaged:"), vehicle->name);
		case GameEventType::VehicleDestroyed:
			return format("%s %s", tr("Vehicle destroyed:"), vehicle->name);
		case GameEventType::VehicleEscaping:
			return format("%s %s", tr("Vehicle returning to base as damaged:"), vehicle->name);
		case GameEventType::VehicleNoAmmo:
			return format("%s %s", vehicle->name, tr(": Weapon out of ammo:"));
		case GameEventType::VehicleLowFuel:
			return format("%s %s", tr("Vehicle low on fuel:"), vehicle->name);
		case GameEventType::VehicleRepaired:
			return format("%s %s", tr("Vehicle Repaired:"), vehicle->name);
		case GameEventType::VehicleRearmed:
			return format("%s %s", tr("Vehicle Rearmed:"), vehicle->name);
		case GameEventType::VehicleRefuelled:
			return format("%s %s", tr("Vehicle Refuelled:"), vehicle->name);
		case GameEventType::VehicleNoEngine:
			return format("%s %s", tr("Vehicle has no engine:"), vehicle->name);
		case GameEventType::UnauthorizedVehicle:
			if (vehicle->type->type == VehicleType::Type::Ground)
			{
				return tr("An illegal road vehicle has been detected.");
			}
			else
			{
				return tr("An illegal flyer has been detected.");
			}
		case GameEventType::NotEnoughAmmo:
			return tr("Not enough ammo to rearm vehicle");
		case GameEventType::NotEnoughFuel:
			return tr("Not enough fuel to refuel vehicle");
		default:
			LogError("Invalid event type");
			break;
	}
	return "";
}

GameBaseEvent::GameBaseEvent(GameEventType type, StateRef<Base> base) : GameEvent(type), base(base)
{
}

GameBuildingEvent::GameBuildingEvent(GameEventType type, StateRef<Building> building)
    : GameEvent(type), building(building)
{
}

GameOrganisationEvent::GameOrganisationEvent(GameEventType type,
                                             StateRef<Organisation> organisation)
    : GameEvent(type), organisation(organisation)
{
}

GameAgentEvent::GameAgentEvent(GameEventType type, StateRef<Agent> agent)
    : GameEvent(type), agent(agent)
{
}

GameResearchEvent::GameResearchEvent(GameEventType type, StateRef<ResearchTopic> topic,
                                     StateRef<Lab> lab)
    : GameEvent(type), topic(topic), lab(lab)
{
}

GameManufactureEvent::GameManufactureEvent(GameEventType type, StateRef<ResearchTopic> topic,
                                           unsigned done, unsigned goal, StateRef<Lab> lab)
    : GameEvent(type), topic(topic), lab(lab), done(done), goal(goal)
{
}

GameFacilityEvent::GameFacilityEvent(GameEventType type, sp<Base> base, sp<Facility> facility)
    : GameEvent(type), base(base), facility(facility)
{
}
}
