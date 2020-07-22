#include "game/state/gameevent.h"
#include "city/vehicle.h"
#include "framework/logger.h"
#include "game/state/battle/battle.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/rules/city/vehicletype.h"
#include "game/state/shared/agent.h"
#include "game/state/shared/organisation.h"
#include "library/strings_format.h"

namespace OpenApoc
{

const std::map<GameEventType, UString> GameEvent::optionsMap = {
    {GameEventType::UfoSpotted, "Notifications.City.UfoSpotted"},
    {GameEventType::VehicleLightDamage, "Notifications.City.VehicleLightDamage"},
    {GameEventType::VehicleModerateDamage, "Notifications.City.VehicleModerateDamage"},
    {GameEventType::VehicleHeavyDamage, "Notifications.City.VehicleHeavyDamage"},
    {GameEventType::VehicleDestroyed, "Notifications.City.VehicleDestroyed"},
    {GameEventType::VehicleEscaping, "Notifications.City.VehicleEscaping"},
    {GameEventType::VehicleNoAmmo, "Notifications.City.VehicleNoAmmo"},
    {GameEventType::VehicleLowFuel, "Notifications.City.VehicleLowFuel"},
    {GameEventType::AgentDiedCity, "Notifications.City.AgentDiedCity"},
    {GameEventType::AgentArrived, "Notifications.City.AgentArrived"},
    {GameEventType::CargoArrived, "Notifications.City.CargoArrived"},
    {GameEventType::TransferArrived, "Notifications.City.TransferArrived"},
    {GameEventType::RecoveryArrived, "Notifications.City.RecoveryArrived"},
    {GameEventType::VehicleRepaired, "Notifications.City.VehicleRepaired"},
    {GameEventType::VehicleRearmed, "Notifications.City.VehicleRearmed"},
    {GameEventType::NotEnoughAmmo, "Notifications.City.NotEnoughAmmo"},
    {GameEventType::VehicleRefuelled, "Notifications.City.VehicleRefuelled"},
    {GameEventType::NotEnoughFuel, "Notifications.City.NotEnoughFuel"},
    {GameEventType::CommenceInvestigation, "Notifications.City.CommenceInvestigation"},
    {GameEventType::UnauthorizedVehicle, "Notifications.City.UnauthorizedVehicle"},

    {GameEventType::HostileSpotted, "Notifications.Battle.HostileSpotted"},
    {GameEventType::HostileDied, "Notifications.Battle.HostileDied"},
    {GameEventType::UnknownDied, "Notifications.Battle.UnknownDied"},
    {GameEventType::AgentDiedBattle, "Notifications.Battle.AgentDiedBattle"},
    {GameEventType::AgentBrainsucked, "Notifications.Battle.AgentBrainsucked"},
    {GameEventType::AgentCriticallyWounded, "Notifications.Battle.AgentCriticallyWounded"},
    {GameEventType::AgentBadlyInjured, "Notifications.Battle.AgentBadlyInjured"},
    {GameEventType::AgentInjured, "Notifications.Battle.AgentInjured"},
    {GameEventType::AgentUnderFire, "Notifications.Battle.AgentUnderFire"},
    {GameEventType::AgentUnconscious, "Notifications.Battle.AgentUnconscious"},
    {GameEventType::AgentLeftCombat, "Notifications.Battle.AgentLeftCombat"},
    {GameEventType::AgentFrozen, "Notifications.Battle.AgentFrozen"},
    {GameEventType::AgentBerserk, "Notifications.Battle.AgentBerserk"},
    {GameEventType::AgentPanicked, "Notifications.Battle.AgentPanicked"},
    {GameEventType::AgentPanicOver, "Notifications.Battle.AgentPanicOver"},
    {GameEventType::AgentPsiAttacked, "Notifications.Battle.AgentPsiAttacked"},
    {GameEventType::AgentPsiControlled, "Notifications.Battle.AgentPsiControlled"},
    {GameEventType::AgentPsiOver, "Notifications.Battle.AgentPsiOver"},

};

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
		case GameEventType::MissionCompletedBuildingAlien:
			return tr("Mission completed in Alien building.");
		case GameEventType::MissionCompletedVehicle:
			return tr("X-COM returning from UFO mission.");
		case GameEventType::BuildingDisabled:
			return tr("Building has been disabled");
		default:
			break;
	}
	return "";
}

UString GameVehicleEvent::message()
{
	switch (type)
	{
		case GameEventType::UfoSpotted:
			return tr("UFO spotted.");
		case GameEventType::UfoCrashed:
			return format("%s %s", tr("UFO crash landed:"), vehicle->name);
		case GameEventType::UfoRecoveryUnmanned:
			return format("%s %s", tr("Unmanned UFO recovered:"), vehicle->name);
		case GameEventType::VehicleRecovered:
			return format("%s %s", tr("Vehicle successfully recovered:"), vehicle->name);
		case GameEventType::VehicleNoFuel:
			return format("%s %s", tr("Vehicle out of fuel:"), vehicle->name);
		case GameEventType::UfoRecoveryBegin:
			return "";
		case GameEventType::VehicleLightDamage:
			return format("%s %s", tr("Vehicle lightly damaged:"), vehicle->name);
		case GameEventType::VehicleModerateDamage:
			return format("%s %s", tr("Vehicle moderately damaged:"), vehicle->name);
		case GameEventType::VehicleHeavyDamage:
			return format("%s %s", tr("Vehicle heavily damaged:"), vehicle->name);
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
			if (vehicle->type->isGround())
			{
				return tr("An illegal road vehicle has been detected.");
			}
			else
			{
				return tr("An illegal flyer has been detected.");
			}
		case GameEventType::NotEnoughAmmo:
			return format("%s %s", tr("Not enough ammo to rearm vehicle:"), vehicle->name);
		case GameEventType::NotEnoughFuel:
			return format("%s %s", tr("Not enough fuel to refuel vehicle"), vehicle->name);
		default:
			LogError("Invalid vehicle event type");
			break;
	}
	return "";
}

UString GameAgentEvent::message()
{
	switch (type)
	{
		case GameEventType::AgentArrived:
			if (flag)
			{
				return format("%s %s", tr("New transfer arrived:"), agent->name);
			}
			else
			{
				return format("%s %s", tr("New recruit arrived:"), agent->name);
			}
		case GameEventType::AgentUnableToReach:
			return format(
			    "%s%s", agent->name,
			    tr(": Unable to reach destination due to damaged people tube network and / or "
			       "poor diplomatic relations with Transtellar."));
		case GameEventType::HostileSpotted:
			return format("%s", tr("Hostile unit spotted"));
		case GameEventType::AgentBrainsucked:
			return format("%s %s", tr("Unit Brainsucked:"), agent->name);
		case GameEventType::AgentDiedBattle:
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
		case GameEventType::AgentPsiProbed:
			return "";
		default:
			LogError("Invalid agent event type");
			break;
	}
	return "";
}

UString GameBuildingEvent::message()
{
	switch (type)
	{
		case GameEventType::MissionCompletedBuildingNormal:
			return format("%s %s", tr("X-COM returning from mission at:"), building->name);
		case GameEventType::MissionCompletedBuildingRaid:
			return format("%s %s", tr("X-COM returning from raid at:"), building->name);
		case GameEventType::BuildingAttacked:
			return format("%s %s %s %s", tr("Building under attack :"), building->name,
			              tr("Attacked by:"), actor->name);
		case GameEventType::AlienSpotted:
			return tr("Live Alien spotted.");
		case GameEventType::CargoExpiresSoon:
			return format("%s %s", tr("Cargo expires soon:"), building->name);
		case GameEventType::CommenceInvestigation:
			return "";
		default:
			LogError("Invalid building event type");
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
		case GameEventType::CargoExpired:
			if (actor)
			{
				if (actor == base->building->owner)
				{
					return tr("Cargo expired:") + " " + base->name + " " + tr("Returned to base");
				}
				else
				{
					return tr("Cargo expired:") + " " + base->name + " " +
					       tr("Refunded by supplier: ") + actor->name;
				}
			}
			else
			{
				return tr("Cargo expired:") + " " + base->name;
			}
		case GameEventType::CargoSeized:
		{
			return tr("Cargo seized:") + " " + base->name + " " + tr("By hostile organisation: ") +
			       actor->name;
		}
		case GameEventType::CargoArrived:
			if (actor)
			{
				return tr("Cargo arrived:") + " " + base->name + " " + tr("Supplier: ") +
				       actor->name;
			}
			else
			{
				return tr("Cargo arrived:") + " " + base->name;
			}
		case GameEventType::TransferArrived:
			if (flag)
			{
				return tr("Transferred Alien specimens have arrived:") + " " + base->name;
			}
			else
			{
				return tr("Transferred goods have arrived:") + " " + base->name;
			}
		case GameEventType::RecoveryArrived:
			return tr("Items from tactical combat zone have arrived:") + " " + base->name;
		case GameEventType::MissionCompletedBase:
			return tr("Base mission completed at:") + " " + base->name;

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
			LogError("Invalid battle event type");
			break;
	}
	return "";
}

GameBaseEvent::GameBaseEvent(GameEventType type, StateRef<Base> base, StateRef<Organisation> actor,
                             bool flag)
    : GameEvent(type), base(base), actor(actor), flag(flag)
{
}

GameBuildingEvent::GameBuildingEvent(GameEventType type, StateRef<Building> building,
                                     StateRef<Organisation> actor)
    : GameEvent(type), building(building), actor(actor)
{
}

GameOrganisationEvent::GameOrganisationEvent(GameEventType type,
                                             StateRef<Organisation> organisation)
    : GameEvent(type), organisation(organisation)
{
}

GameAgentEvent::GameAgentEvent(GameEventType type, StateRef<Agent> agent, bool flag)
    : GameEvent(type), agent(agent), flag(flag)
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
GameDefenseEvent::GameDefenseEvent(GameEventType type, StateRef<Base> base,
                                   StateRef<Organisation> organisation)
    : GameEvent(type), base(base), organisation(organisation)
{
}

GameSomethingDiedEvent::GameSomethingDiedEvent(GameEventType type, UString name, Vec3<int> location)
    : GameSomethingDiedEvent(type, name, "", location)
{
}

GameSomethingDiedEvent::GameSomethingDiedEvent(GameEventType type, UString name, UString actor,
                                               Vec3<int> location)
    : GameEvent(type), location(location)
{
	switch (type)
	{
		case GameEventType::AgentDiedCity:
			messageInner = format("%s %s", tr("Agent has died:"), name);
			break;
		case GameEventType::BaseDestroyed:
			if (actor.length() > 0)
			{
				messageInner = tr("X-COM base destroyed by hostile forces.");
			}
			else
			{
				messageInner = tr("X-COM Base destroyed due to collapsing building.");
			}
			break;
		case GameEventType::VehicleDestroyed:
			if (actor.length() > 0)
			{
				messageInner = format("%s %s %s: %s", tr("Vehicle destroyed:"), name,
				                      tr("destroyed by"), actor);
			}
			else
			{
				messageInner = format("%s %s", tr("Vehicle destroyed:"), name);
			}
			break;
		case GameEventType::VehicleRecovered:
			messageInner =
			    format("%s %s", tr("Scrapped vehicle recovered in irreparable condition:"), name);
			break;
		case GameEventType::VehicleNoFuel:
			messageInner = format("%s %s", tr("Vehicle out of fuel:"), name);
			break;
		default:
			LogWarning("GameSomethingDiedEvent %s called on non-death event %d", name,
			           static_cast<int>(type));
			break;
	}
}
UString GameSomethingDiedEvent::message() { return messageInner; }
} // namespace OpenApoc
