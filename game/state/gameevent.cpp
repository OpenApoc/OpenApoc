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
			return fmt::format("{} {}", tr("UFO crash landed:"), vehicle->name);
		case GameEventType::UfoRecoveryUnmanned:
			return fmt::format("{} {}", tr("Unmanned UFO recovered:"), vehicle->name);
		case GameEventType::VehicleRecovered:
			return fmt::format("{} {}", tr("Vehicle successfully recovered:"), vehicle->name);
		case GameEventType::VehicleNoFuel:
			return fmt::format("{} {}", tr("Vehicle out of fuel:"), vehicle->name);
		case GameEventType::UfoRecoveryBegin:
			return "";
		case GameEventType::VehicleLightDamage:
			return fmt::format("{} {}", tr("Vehicle lightly damaged:"), vehicle->name);
		case GameEventType::VehicleModerateDamage:
			return fmt::format("{} {}", tr("Vehicle moderately damaged:"), vehicle->name);
		case GameEventType::VehicleHeavyDamage:
			return fmt::format("{} {}", tr("Vehicle heavily damaged:"), vehicle->name);
		case GameEventType::VehicleEscaping:
			return fmt::format("{} {}", tr("Vehicle returning to base as damaged:"), vehicle->name);
		case GameEventType::VehicleNoAmmo:
			return fmt::format("{} {}", vehicle->name, tr(": Weapon out of ammo:"));
		case GameEventType::VehicleLowFuel:
			return fmt::format("{} {}", tr("Vehicle low on fuel:"), vehicle->name);
		case GameEventType::VehicleRepaired:
			return fmt::format("{} {}", tr("Vehicle Repaired:"), vehicle->name);
		case GameEventType::VehicleRearmed:
			return fmt::format("{} {}", tr("Vehicle Rearmed:"), vehicle->name);
		case GameEventType::VehicleRefuelled:
			return fmt::format("{} {}", tr("Vehicle Refuelled:"), vehicle->name);
		case GameEventType::VehicleNoEngine:
			return fmt::format("{} {}", tr("Vehicle has no engine:"), vehicle->name);
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
			return fmt::format("{} {}", tr("Not enough ammo to rearm vehicle:"), vehicle->name);
		case GameEventType::NotEnoughFuel:
			return fmt::format("{} {}", tr("Not enough fuel to refuel vehicle"), vehicle->name);
		case GameEventType::VehicleWithAlienLootInBaseWithNoContainment:
			return fmt::format(
			    "{} {}", tr("Vehicle landed with alien loot in base with no alien containment"),
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
				return fmt::format("{} {}", tr("New transfer arrived:"), agent->name);
			}
			else
			{
				return fmt::format("{} {}", tr("New recruit arrived:"), agent->name);
			}
		case GameEventType::AgentUnableToReach:
			return fmt::format(
			    "{}{}", agent->name,
			    tr(": Unable to reach destination due to damaged people tube network and / or "
			       "poor diplomatic relations with Transtellar."));
		case GameEventType::HostileSpotted:
			return fmt::format("{}", tr("Hostile unit spotted"));
		case GameEventType::AgentBrainsucked:
			return fmt::format("{} {}", tr("Unit Brainsucked:"), agent->name);
		case GameEventType::AgentDiedBattle:
			return fmt::format("{} {}", tr("Unit has died:"), agent->name);
		case GameEventType::HostileDied:
			return fmt::format("{} {}", tr("Hostile unit has died"), agent->name);
		case GameEventType::UnknownDied:
			return fmt::format("{}", tr("Unknown Unit has died"));
		case GameEventType::AgentCriticallyWounded:
			return fmt::format("{}: {}", tr("Unit critically wounded"), agent->name);
		case GameEventType::AgentBadlyInjured:
			return fmt::format("{} {}", tr("Unit badly injured:"), agent->name);
		case GameEventType::AgentInjured:
			return fmt::format("{} {}", tr("Unit injured:"), agent->name);
		case GameEventType::AgentUnderFire:
			return fmt::format("{} {}", tr("Unit under fire:"), agent->name);
		case GameEventType::AgentUnconscious:
			return fmt::format("{} {}", tr("Unit has lost consciousness:"), agent->name);
		case GameEventType::AgentLeftCombat:
			return fmt::format("{} {}", tr("Unit has left combat zone:"), agent->name);
		case GameEventType::AgentFrozen:
			return fmt::format("{} {}", tr("Unit has frozen:"), agent->name);
		case GameEventType::AgentBerserk:
			return fmt::format("{} {}", tr("Unit has gone berserk:"), agent->name);
		case GameEventType::AgentPanicked:
			return fmt::format("{} {}", tr("Unit has panicked:"), agent->name);
		case GameEventType::AgentPanicOver:
			return fmt::format("{} {}", tr("Unit has stopped panicking:"), agent->name);
		case GameEventType::AgentPsiAttacked:
			return fmt::format("{} {}", tr("Psionic attack on unit:"), agent->name);
		case GameEventType::AgentPsiControlled:
			return fmt::format("{} {}", tr("Unit under Psionic control:"), agent->name);
		case GameEventType::AgentPsiOver:
			return fmt::format("{} {}", tr("Unit freed from Psionic control:"), agent->name);
		case GameEventType::NoLOF:
			return fmt::format("{}", tr("No line of fire"));
		case GameEventType::AgentPsiProbed:
			return "";
		case GameEventType::AgentOutOfAmmo:
			return fmt::format("{} {}", agent->name, tr(": Out of ammo"));
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
			return fmt::format("{} {}", tr("X-COM returning from mission at:"), building->name);
		case GameEventType::MissionCompletedBuildingRaid:
			return fmt::format("{} {}", tr("X-COM returning from raid at:"), building->name);
		case GameEventType::BuildingAttacked:
			return fmt::format("{} {} {} {}", tr("Building under attack :"), building->name,
			                   tr("Attacked by:"), actor->name);
		case GameEventType::OrganisationAttackBuilding:
			return fmt::format("{} {} {} {}", tr("Organization attacked:"), building->owner->name,
			                   tr("Attacked by:"), actor->name);
		case GameEventType::OrganisationRaidBuilding:
			return fmt::format("{} {} {} {}", tr("Organization raided:"), building->owner->name,
			                   tr("Raided by:"), actor->name);
		case GameEventType::OrganisationStormBuilding:
			return fmt::format("{} {} {} {}", tr("Organization stormed:"), building->owner->name,
			                   tr("Stormed by:"), actor->name);
		case GameEventType::OrganisationTreatySigned:
			return fmt::format("{} {}, {}", tr("Treaty signed:"), building->owner->name,
			                   actor->name);
		case GameEventType::AlienSpotted:
			return tr("Live Alien spotted.");
		case GameEventType::CargoExpiresSoon:
			return fmt::format("{} {}", tr("Cargo expires soon:"), building->name);
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
			return fmt::format("Agent(s) rearmed: {}", base->name);
		case GameEventType::CargoExpired:
			if (actor)
			{
				if (actor == base->building->owner)
				{
					return fmt::format("Cargo expired: {} Returned to base", base->name);
				}
				else
				{
					return fmt::format("Cargo expired:{} Refunded by supplier: {}", base->name,
					                   actor->name);
				}
			}
			else
			{
				return fmt::format("Cargo expired: {}", base->name);
			}
		case GameEventType::CargoSeized:
		{
			return fmt::format("Cargo seized: {} By hostile organisation: {}", base->name,
			                   actor->name);
		}
		case GameEventType::CargoArrived:
			if (actor)
			{
				return fmt::format("Cargo arrived: {} Supplier: {}", base->name, actor->name);
			}
			else
			{
				return fmt::format("Cargo arrived: {}", base->name);
			}
		case GameEventType::TransferArrived:
			if (flag)
			{
				return fmt::format("Transferred Alien specimens have arrived: {}", base->name);
			}
			else
			{
				return fmt::format("Transferred goods have arrived: {}", base->name);
			}
		case GameEventType::RecoveryArrived:
			return fmt::format("Items from tactical combat zone have arrived: {}", base->name);
		case GameEventType::MissionCompletedBase:
			return fmt::format("Base mission completed at: {}", base->name);

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
			return fmt::format("Turn: {}   Side: {}", battle->currentTurn,
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
			messageInner = fmt::format("{} {}", tr("Agent has died:"), name);
			break;
		case GameEventType::BaseDestroyed:
			if (actor.length() > 0)
			{
				messageInner = fmt::format("X-COM {} destroyed by hostile forces", name);
			}
			else
			{
				messageInner = fmt::format("X-COM {} destroyed due to collapsing building.", name);
			}
			break;
		case GameEventType::VehicleDestroyed:
			if (actor.length() > 0)
			{
				messageInner = fmt::format("{} {}: {}", name, tr("destroyed by"), actor);
			}
			else
			{
				messageInner = fmt::format("{} {}", tr("Vehicle destroyed:"), name);
			}
			break;
		case GameEventType::VehicleRecovered:
			messageInner = fmt::format(
			    "{} {}", tr("Scrapped vehicle recovered in irreparable condition:"), name);
			break;
		case GameEventType::VehicleNoFuel:
			messageInner = fmt::format("{} {}", tr("Vehicle out of fuel:"), name);
			break;
		case GameEventType::VehicleModuleScrapped:
			messageInner = fmt::format("{} {}", tr("Module lost during recovery:"), name);
			break;
		default:
			LogWarning("GameSomethingDiedEvent {} called on non-death event {}", name,
			           static_cast<int>(type));
			break;
	}
}
UString GameSomethingDiedEvent::message() { return messageInner; }
} // namespace OpenApoc
