#pragma once

namespace OpenApoc
{

enum class GameEventType
{
	// Vehicle events
	UfoSpotted,
	UfoCrashed,
	UfoRecoverySuccess,
	UfoRecoveryUnmanned,
	UfoRecoveryFailure,
	UfoRecoveryBegin,
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

	// Defense events
	DefendTheBase,

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
	AgentDiedCity,
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
	MissionCompleted,
	BuildingDisabled,
};
}
