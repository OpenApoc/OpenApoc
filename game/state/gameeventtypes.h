#pragma once

namespace OpenApoc
{

enum class GameEventType
{
	// Vehicle events
	UfoSpotted,
	UfoCrashed,
	UfoRecoveryUnmanned,
	VehicleLightDamage,
	VehicleModerateDamage,
	VehicleHeavyDamage,
	VehicleEscaping,
	VehicleNoAmmo,
	VehicleLowFuel,
	VehicleNoFuel,
	VehicleRepaired,
	VehicleRearmed,
	VehicleRefuelled,
	VehicleNoEngine,
	VehicleRecovered,
	UnauthorizedVehicle,
	NotEnoughAmmo,
	NotEnoughFuel,
	// Vehicle event that starts recovery mission
	UfoRecoveryBegin,

	// Defense event that starts base defense mission
	DefendTheBase,

	// Base events
	AgentRearmed,
	CargoArrived,
	TransferArrived,
	RecoveryArrived,
	CargoExpired,
	CargoSeized,
	MissionCompletedBase,

	// Something died events
	BaseDestroyed,
	AgentDiedCity,
	VehicleDestroyed,

	// Building events
	AlienSpotted,
	CommenceInvestigation,
	BuildingAttacked,
	CargoExpiresSoon,
	MissionCompletedBuildingRaid,
	MissionCompletedBuildingNormal,

	// Organization events
	AlienTakeover,
	OrganizationAttack,
	OrganizationRaid,
	OrganizationTreaty,
	OrganizationBribe,

	// Agent events
	AgentArrived,
	AgentUnableToReach,
	HostileSpotted,
	AgentBrainsucked,
	AgentDiedBattle,
	HostileDied,
	UnknownDied,
	AgentCriticallyWounded,
	AgentBadlyInjured,
	AgentInjured,
	AgentUnderFire,
	AgentUnconscious,
	AgentLeftCombat,
	AgentFrozen,
	AgentBerserk,
	AgentPanicked,
	AgentPanicOver,
	AgentPsiAttacked,
	AgentPsiProbed,
	AgentPsiControlled,
	AgentPsiOver,
	NoLOF,

	// Battle events

	NewTurn,

	// Research
	ResearchCompleted,

	// Manufacture
	ManufactureCompleted,
	ManufactureHalted,

	// Facility
	FacilityCompleted,

	// LocationEvent
	ZoomView,

	// Misc
	MarketUpdate,
	WeeklyReport,
	EnterAlienDimension,
	LeaveAlienDimension,
	MissionStarted,
	BuildingDisabled,
	MissionCompletedVehicle,
	MissionCompletedBuildingAlien,

	None
};
}
