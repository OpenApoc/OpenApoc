#pragma once

namespace OpenApoc
{

enum class GameEventType
{
	// Vehicle events
	UfoSpotted,
	UfoCrashed,
	UfoRecovered,
	VehicleLightDamage,
	VehicleModerateDamage,
	VehicleHeavyDamage,
	VehicleDestroyed,
	VehicleEscaping,
	VehicleNoAmmo,
	VehicleLowFuel,
	VehicleRepaired,
	VehicleRearmed,
	VehicleRefuelled,
	VehicleNoEngine,
	UnauthorizedVehicle,
	NotEnoughAmmo,
	NotEnoughFuel,

	// Base events
	AgentRearmed,
	CargoArrived,
	TransferArrived,
	RecoveryArrived,
	BaseDestroyed,

	// Building events
	AlienSpotted,
	BuildingAttacked,

	// Organization events
	AlienTakeover,
	OrganizationAttack,
	OrganizationRaid,
	OrganizationTreaty,
	OrganizationBribe,

	// Agent events
	AgentArrived,
	HostileSpotted,
	AgentBrainsucked,
	AgentDied,
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
	MissionCompleted,
	BuildingDisabled,
};
}
