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
#include "library/strings_translate.h"

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
			return tformat("Mission completed in Alien building.");
		case GameEventType::MissionCompletedVehicle:
			return tformat("X-COM returning from UFO mission.");
		case GameEventType::BuildingDisabled:
			return tformat("Building has been disabled");
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
			return tformat("UFO spotted.");
		case GameEventType::UfoCrashed:
			return tformat("UFO crash landed: {1}", vehicle->name);
		case GameEventType::UfoRecoveryUnmanned:
			return tformat("Unmanned UFO recovered: {1}", vehicle->name);
		case GameEventType::VehicleRecovered:
			return tformat("Vehicle successfully recovered: {1}", vehicle->name);
		case GameEventType::VehicleNoFuel:
			return tformat("Vehicle out of fuel: {1}", vehicle->name);
		case GameEventType::UfoRecoveryBegin:
			return "";
		case GameEventType::VehicleLightDamage:
			return tformat("Vehicle lightly damaged: {1}", vehicle->name);
		case GameEventType::VehicleModerateDamage:
			return tformat("Vehicle moderately damaged: {1}", vehicle->name);
		case GameEventType::VehicleHeavyDamage:
			return tformat("Vehicle heavily damaged: {1}", vehicle->name);
		case GameEventType::VehicleEscaping:
			return tformat("Vehicle returning to base as damaged: {1}", vehicle->name);
		case GameEventType::VehicleNoAmmo:
			return tformat("{1}: Weapon out of ammo", vehicle->name);
		case GameEventType::VehicleLowFuel:
			return tformat("Vehicle low on fuel: {1}", vehicle->name);
		case GameEventType::VehicleRepaired:
			return tformat("Vehicle Repaired: {1}", vehicle->name);
		case GameEventType::VehicleRearmed:
			return tformat("Vehicle Rearmed: {1}", vehicle->name);
		case GameEventType::VehicleRefuelled:
			return tformat("Vehicle Refuelled: {1}", vehicle->name);
		case GameEventType::VehicleNoEngine:
			return tformat("Vehicle has no engine: {1}", vehicle->name);
		case GameEventType::UnauthorizedVehicle:
			if (vehicle->type->isGround())
			{
				return tformat("An illegal road vehicle has been detected.");
			}
			else
			{
				return tformat("An illegal flyer has been detected.");
			}
		case GameEventType::NotEnoughAmmo:
			return tformat("Not enough ammo to rearm vehicle: {1}", vehicle->name);
		case GameEventType::NotEnoughFuel:
			return tformat("Not enough fuel to refuel vehicle {1}", vehicle->name);
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
				return tformat("New transfer arrived: {1}", agent->name);
			}
			else
			{
				return tformat("New recruit arrived: {1}", agent->name);
			}
		case GameEventType::AgentUnableToReach:
			return tformat(
			    "{1}: Unable to reach destination due to damaged people tube network and / or "
			    "poor diplomatic relations with Transtellar.",
			    agent->name);
		case GameEventType::HostileSpotted:
			return tformat("Hostile unit spotted");
		case GameEventType::AgentBrainsucked:
			return tformat("Unit Brainsucked: {1}", agent->name);
		case GameEventType::AgentDiedBattle:
			return tformat("Unit has died: {1}", agent->name);
		case GameEventType::HostileDied:
			return tformat("Hostile unit has died {1}", agent->name);
		case GameEventType::UnknownDied:
			return tformat("Unknown Unit has died");
		case GameEventType::AgentCriticallyWounded:
			return tformat("Unit critically wounded {1}", agent->name);
		case GameEventType::AgentBadlyInjured:
			return tformat("Unit badly injured: {1}", agent->name);
		case GameEventType::AgentInjured:
			return tformat("Unit injured: {1}", agent->name);
		case GameEventType::AgentUnderFire:
			return tformat("Unit under fire: {1}", agent->name);
		case GameEventType::AgentUnconscious:
			return tformat("Unit has lost consciousness: {1}", agent->name);
		case GameEventType::AgentLeftCombat:
			return tformat("Unit has left combat zone: {1}", agent->name);
		case GameEventType::AgentFrozen:
			return tformat("Unit has frozen: {1}", agent->name);
		case GameEventType::AgentBerserk:
			return tformat("Unit has gone berserk: {1}", agent->name);
		case GameEventType::AgentPanicked:
			return tformat("Unit has panicked: {1}", agent->name);
		case GameEventType::AgentPanicOver:
			return tformat("Unit has stopped panicking: {1}", agent->name);
		case GameEventType::AgentPsiAttacked:
			return tformat("Psionic attack on unit: {1}", agent->name);
		case GameEventType::AgentPsiControlled:
			return tformat("Unit under Psionic control: {1}", agent->name);
		case GameEventType::AgentPsiOver:
			return tformat("Unit freed from Psionic control: {1}", agent->name);
		case GameEventType::NoLOF:
			return tformat("No line of fire");
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
			return tformat("X-COM returning from mission at: {1}", building->name);
		case GameEventType::MissionCompletedBuildingRaid:
			return tformat("X-COM returning from raid at: {1}", building->name);
		case GameEventType::BuildingAttacked:
			return tformat("Building under attack: {1} Attacked by: {2}", building->name,
			               actor->name);
		case GameEventType::AlienSpotted:
			return tformat("Live Alien spotted.");
		case GameEventType::CargoExpiresSoon:
			return tformat("Cargo expires soon: {1}", building->name);
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
			return tformat("Agent(s) rearmed: {1}", base->name);
		case GameEventType::CargoExpired:
			if (actor)
			{
				if (actor == base->building->owner)
				{
					return tformat("Cargo expired: {1} Returned to base", base->name);
				}
				else
				{
					return tformat("Cargo expired: {1} Refunded by supplier: {2}", base->name,
					               actor->name);
				}
			}
			else
			{
				return tformat("Cargo expired: {1}", base->name);
			}
		case GameEventType::CargoSeized:
		{
			return tformat("Cargo seized: {1} By hostile organisation: {2}", base->name,
			               actor->name);
		}
		case GameEventType::CargoArrived:
			if (actor)
			{
				return tformat("Cargo arrived: {1} Supplier: {2}", base->name, actor->name);
			}
			else
			{
				return tformat("Cargo arrived: {1}", base->name);
			}
		case GameEventType::TransferArrived:
			if (flag)
			{
				return tformat("Transferred Alien specimens have arrived: {1}", base->name);
			}
			else
			{
				return tformat("Transferred goods have arrived: {1}", base->name);
			}
		case GameEventType::RecoveryArrived:
			return tformat("Items from tactical combat zone have arrived: {1}", base->name);
		case GameEventType::MissionCompletedBase:
			return tformat("Base mission completed at: {1}", base->name);

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
			return tformat("Turn: {1}   Side: {2}", battle->currentTurn,
			               battle->currentActiveOrganisation->name);
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
			messageInner = tformat("Agent has died: {1}", name);
			break;
		case GameEventType::BaseDestroyed:
			if (actor.length() > 0)
			{
				messageInner = tformat("X-COM base destroyed by hostile forces.");
			}
			else
			{
				messageInner = tformat("X-COM Base destroyed due to collapsing building.");
			}
			break;
		case GameEventType::VehicleDestroyed:
			if (actor.length() > 0)
			{
				messageInner = tformat("Vehicle destroyed: {1} destroyed by {2}", name, actor);
			}
			else
			{
				messageInner = tformat("Vehicle destroyed: {1}", name);
			}
			break;
		case GameEventType::VehicleRecovered:
			messageInner =
			    tformat("Scrapped vehicle recovered in irreparable condition: {1}", name);
			break;
		case GameEventType::VehicleNoFuel:
			messageInner = tformat("Vehicle out of fuel: {1}", name);
			break;
		default:
			LogWarning("GameSomethingDiedEvent %s called on non-death event %d", name,
			           static_cast<int>(type));
			break;
	}
}
UString GameSomethingDiedEvent::message() { return messageInner; }
} // namespace OpenApoc
