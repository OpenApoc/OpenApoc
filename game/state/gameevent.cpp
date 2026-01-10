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
    {GameEventType::BaseDestroyed, "Notifications.City.BaseDestroyed"},

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
			return format(tr("UFO crash landed: {0}"), vehicle->name);
		case GameEventType::UfoRecoveryUnmanned:
			return format(tr("Unmanned UFO recovered: {0}"), vehicle->name);
		case GameEventType::VehicleRecovered:
			return format(tr("Vehicle successfully recovered: {0}"), vehicle->name);
		case GameEventType::VehicleNoFuel:
			return format(tr("Vehicle out of fuel: {0}"), vehicle->name);
		case GameEventType::UfoRecoveryBegin:
			return "";
		case GameEventType::VehicleLightDamage:
			return format(tr("Vehicle lightly damaged: {0}"), vehicle->name);
		case GameEventType::VehicleModerateDamage:
			return format(tr("Vehicle moderately damaged: {0}"), vehicle->name);
		case GameEventType::VehicleHeavyDamage:
			return format(tr("Vehicle heavily damaged: {0}"), vehicle->name);
		case GameEventType::VehicleEscaping:
			return format(tr("Vehicle returning to base as damaged: {0}"), vehicle->name);
		case GameEventType::VehicleNoAmmo:
			return format(tr("{0}: Weapon out of ammo"), vehicle->name);
		case GameEventType::VehicleLowFuel:
			return format(tr("Vehicle low on fuel: {0}"), vehicle->name);
		case GameEventType::VehicleRepaired:
			return format(tr("Vehicle repaired: {0}"), vehicle->name);
		case GameEventType::VehicleRearmed:
			return format(tr("Vehicle rearmed: {0}"), vehicle->name);
		case GameEventType::VehicleRefuelled:
			return format(tr("Vehicle refuelled: {0}"), vehicle->name);
		case GameEventType::VehicleNoEngine:
			return format(tr("Vehicle has no engine: {0}"), vehicle->name);
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
			return format(tr("Not enough ammo to rearm vehicle: {0}"), vehicle->name);
		case GameEventType::NotEnoughFuel:
			return format(tr("Not enough fuel to refuel vehicle: {0}"), vehicle->name);
		case GameEventType::VehicleWithAlienLootInBaseWithNoContainment:
			return format(
			    tr("Vehicle landed with alien loot in base with no alien containment: {0}"),
			    vehicle->name);
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
				return format(tr("New transfer arrived: {0}"), agent->name);
			}
			else
			{
				return format(tr("New recruit arrived: {0}"), agent->name);
			}
		case GameEventType::AgentUnableToReach:
			return format(
			    tr("{0}: Unable to reach destination due to damaged people tube network and / or "
			       "poor diplomatic relations with Transtellar."),
			    agent->name);
		case GameEventType::HostileSpotted:
			return tr("Hostile unit spotted");
		case GameEventType::AgentBrainsucked:
			return format(tr("Unit brainsucked: {0}"), agent->name);
		case GameEventType::AgentDiedBattle:
			return format(tr("Unit has died: {0}"), agent->name);
		case GameEventType::HostileDied:
			return format(tr("Hostile unit has died: {0}"), agent->name);
		case GameEventType::UnknownDied:
			return tr("Unknown unit has died");
		case GameEventType::AgentCriticallyWounded:
			return format(tr("Unit critically wounded: {0}"), agent->name);
		case GameEventType::AgentBadlyInjured:
			return format(tr("Unit badly injured: {0}"), agent->name);
		case GameEventType::AgentInjured:
			return format(tr("Unit injured: {0}"), agent->name);
		case GameEventType::AgentUnderFire:
			return format(tr("Unit under fire: {0}"), agent->name);
		case GameEventType::AgentUnconscious:
			return format(tr("Unit has lost consciousness: {0}"), agent->name);
		case GameEventType::AgentLeftCombat:
			return format(tr("Unit has left combat zone: {0}"), agent->name);
		case GameEventType::AgentFrozen:
			return format(tr("Unit has frozen: {0}"), agent->name);
		case GameEventType::AgentBerserk:
			return format(tr("Unit has gone berserk: {0}"), agent->name);
		case GameEventType::AgentPanicked:
			return format(tr("Unit has panicked: {0}"), agent->name);
		case GameEventType::AgentPanicOver:
			return format(tr("Unit has stopped panicking: {0}"), agent->name);
		case GameEventType::AgentPsiAttacked:
			return format(tr("Psionic attack on unit: {0}"), agent->name);
		case GameEventType::AgentPsiControlled:
			return format(tr("Unit under psionic control: {0}"), agent->name);
		case GameEventType::AgentPsiOver:
			return format(tr("Unit freed from psionic control: {0}"), agent->name);
		case GameEventType::NoLOF:
			return tr("No line of fire");
		case GameEventType::AgentPsiProbed:
			return "";
		case GameEventType::AgentOutOfAmmo:
			return format(tr("{0}: Out of ammo"), agent->name);
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
			return format(tr("X-COM returning from mission at: {0}"), building->name);
		case GameEventType::MissionCompletedBuildingRaid:
			return format(tr("X-COM returning from raid at: {0}"), building->name);
		case GameEventType::BuildingAttacked:
			return format(tr("Building under attack: {0}. Attacked by: {1}"), building->name,
			              actor->name);
		case GameEventType::OrganisationAttackBuilding:
			return format(tr("Organization attacked: {0}. Attacked by: {1}"), building->owner->name,
			              actor->name);
		case GameEventType::OrganisationRaidBuilding:
			return format(tr("Organization raided: {0}. Raided by: {1}"), building->owner->name,
			              actor->name);
		case GameEventType::OrganisationStormBuilding:
			return format(tr("Organization stormed: {0}. Stormed by: {1}"), building->owner->name,
			              actor->name);
		case GameEventType::OrganisationTreatySigned:
			return format(tr("Treaty signed: {0}, {1}"), building->owner->name, actor->name);
		case GameEventType::AlienSpotted:
			return tr("Live Alien spotted.");
		case GameEventType::CargoExpiresSoon:
			return format(tr("Cargo expires soon: {0}"), building->name);
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
			return format(tr("Agent(s) rearmed: {0}"), base->name);
		case GameEventType::CargoExpired:
			if (actor)
			{
				if (actor == base->building->owner)
				{
					return format(tr("Cargo expired: {0}. Returned to base"), base->name);
				}
				else
				{
					return format(tr("Cargo expired: {0}. Refunded by supplier: {1}"), base->name,
					              actor->name);
				}
			}
			else
			{
				return format(tr("Cargo expired: {0}"), base->name);
			}
		case GameEventType::CargoSeized:
		{
			return format(tr("Cargo seized: {0}. By hostile organisation: {1}"), base->name,
			              actor->name);
		}
		case GameEventType::CargoArrived:
			if (actor)
			{
				return format(tr("Cargo arrived: {0}. Supplier: {1}"), base->name, actor->name);
			}
			else
			{
				return format(tr("Cargo arrived: {0}"), base->name);
			}
		case GameEventType::TransferArrived:
			if (flag)
			{
				return format(tr("Transferred Alien specimens have arrived: {0}"), base->name);
			}
			else
			{
				return format(tr("Transferred goods have arrived: {0}"), base->name);
			}
		case GameEventType::RecoveryArrived:
			return format(tr("Items from tactical combat zone have arrived: {0}"), base->name);
		case GameEventType::MissionCompletedBase:
			return format(tr("Base mission completed at: {0}"), base->name);

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
			return format(tr("Turn: {0}   Side: {1}"), battle->currentTurn,
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
			messageInner = format(tr("Agent has died: {0}"), name);
			break;
		case GameEventType::BaseDestroyed:
			if (actor.length() > 0)
			{
				messageInner = format(tr("X-COM {0} destroyed by hostile forces"), name);
			}
			else
			{
				messageInner = format(tr("X-COM {0} destroyed due to collapsing building."), name);
			}
			break;
		case GameEventType::VehicleDestroyed:
			if (actor.length() > 0)
			{
				messageInner = format(tr("{0} destroyed by: {1}"), name, actor);
			}
			else
			{
				messageInner = format(tr("Vehicle destroyed: {0}"), name);
			}
			break;
		case GameEventType::VehicleRecovered:
			messageInner =
			    format(tr("Scrapped vehicle recovered in irreparable condition: {0}"), name);
			break;
		case GameEventType::VehicleNoFuel:
			messageInner = format(tr("Vehicle out of fuel: {0}"), name);
			break;
		case GameEventType::VehicleModuleScrapped:
			messageInner = format(tr("Module lost during recovery: {0}"), name);
			break;
		default:
			LogWarning("GameSomethingDiedEvent {0} called on non-death event {1}", name,
			           static_cast<int>(type));
			break;
	}
}
UString GameSomethingDiedEvent::message() { return messageInner; }
} // namespace OpenApoc
