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
	AgentPsiControlled,
	AgentPsiOver,
	NoLOF,

	// Battle events

	NewTurn,

	// Misc
	ResearchCompleted,
	ManufactureCompleted,
	ManufactureHalted,
	FacilityCompleted,
	MarketUpdate,
	WeeklyReport,
	EnterAlienDimension,
	LeaveAlienDimension,
	MissionStarted,
	MissionCompleted
};
}
