#include "game/state/gameevent.h"
#include "city/vehicle.h"
#include "framework/logger.h"
#include "game/state/agent.h"
#include "game/state/base/base.h"
#include "game/state/battle/battle.h"
#include "game/state/organisation.h"
#include "game/state/rules/vehicle_type.h"
#include "library/strings_format.h"

namespace OpenApoc
{

GameEvent::GameEvent(GameEventType type) : Event(EVENT_GAME_STATE), type(type) {}

GameVehicleEvent::GameVehicleEvent(GameEventType type, StateRef<Vehicle> vehicle,
                                   StateRef<Vehicle> actor)
    : GameEvent(type), vehicle(vehicle), actor(actor)
{
}

UString GameEvent::message()
{
	switch (type)
	{
		case GameEventType::BuildingDisabled:
			return tr("Building has been disabled");
		default:
			LogError("Invalid event type");
			break;
	}
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
			return format("%s %s %s: %s", tr("Vehicle destroyed:"), vehicle->name,
			              tr("destroyed by"), actor->name);
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

UString GameAgentEvent::message()
{
	switch (type)
	{
		case GameEventType::AgentArrived:
			return format("%s %s", tr("New recruit arrived:"), agent->name);
		case GameEventType::HostileSpotted:
			return format("%s", tr("Hostile unit spotted"));
		case GameEventType::AgentBrainsucked:
			return format("%s %s", tr("Unit Brainsucked:"), agent->name);
		case GameEventType::AgentDied:
			return format("%s %s", tr("Unit has died:"), agent->name);
		case GameEventType::HostileDied:
			return format("%s %s", tr("Hostile unit has died"), agent->name);
		case GameEventType::UnknownDied:
			return format("%s", tr("Unknown Unit has died"));
		case GameEventType::AgentCriticallyWounded:
			return format("%s: %s", tr("Unit critically wounded"), agent->name);
		case GameEventType::AgentBadlyInjured:
			return format("%s %s", tr("Unit badly injured:"), agent->name);
		case GameEventType::AgentInjured:
			return format("%s %s", tr("Unit injured:"), agent->name);
		case GameEventType::AgentUnderFire:
			return format("%s %s", tr("Unit under fire:"), agent->name);
		case GameEventType::AgentUnconscious:
			return format("%s %s", tr("Unit has lost consciousness:"), agent->name);
		case GameEventType::AgentLeftCombat:
			return format("%s %s", tr("Unit has left combat zone:"), agent->name);
		case GameEventType::AgentFrozen:
			return format("%s %s", tr("Unit has frozen:"), agent->name);
		case GameEventType::AgentBerserk:
			return format("%s %s", tr("Unit has gone berserk:"), agent->name);
		case GameEventType::AgentPanicked:
			return format("%s %s", tr("Unit has panicked:"), agent->name);
		case GameEventType::AgentPanicOver:
			return format("%s %s", tr("Unit has stopped panicking:"), agent->name);
		case GameEventType::AgentPsiAttacked:
			return format("%s %s", tr("Psionic attack on unit:"), agent->name);
		case GameEventType::AgentPsiControlled:
			return format("%s %s", tr("Unit under Psionic control:"), agent->name);
		case GameEventType::AgentPsiOver:
			return format("%s %s", tr("Unit freed from Psionic control:"), agent->name);
		case GameEventType::NoLOF:
			return format("%s", tr("No line of fire"));
		default:
			LogError("Invalid event type");
			break;
	}
	return "";
}

UString GameBaseEvent::message()
{
	switch (type)
	{
		case GameEventType::AgentRearmed:
			return tr("Agent(s) rearmed:") + " " + base->name;
		case GameEventType::CargoArrived:
			return tr("Cargo arrived:") + " " + base->name;
		case GameEventType::TransferArrived:
			return tr("Transferred goods have arrived:") + " " + base->name;
		case GameEventType::RecoveryArrived:
			return tr("Items from tactical combat zone have arrived:") + " " + base->name;
		case GameEventType::BaseDestroyed:
			return tr("Vehicle destroyed:") + " " + base->name;
		default:
			LogError("Invalid event type");
			break;
	}
	return "";
}

UString GameBattleEvent::message()
{
	switch (type)
	{
		case GameEventType::NewTurn:
			return tr("Turn:") + " " + format("%d", battle->currentTurn) + "   " + tr("Side:") +
			       "  " + tr(battle->currentActiveOrganisation->name);
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

GameBattleEvent::GameBattleEvent(GameEventType type, sp<Battle> battle)
    : GameEvent(type), battle(battle)
{
}
GameLocationEvent::GameLocationEvent(GameEventType type, Vec3<int> location)
    : GameEvent(type), location(location)
{
}
}
