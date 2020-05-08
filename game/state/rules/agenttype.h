#pragma once

#include "framework/image.h"
#include "game/state/gametime.h"
#include "game/state/shared/equipment.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <list>
#include <map>
#include <set>

namespace OpenApoc
{

class Organisation;
class AEquipment;
class BattleUnitAnimationPack;
class BattleUnitImagePack;
class Sample;
class AgentBodyType;
class BattleUnit;
class DamageModifier;
class DamageType;
class Building;
class Vehicle;
class AgentMission;
class VoxelMap;
class City;
enum class AIType;
class AEquipmentType;

enum class BodyPart
{
	Body,
	Legs,
	Helmet,
	LeftArm,
	RightArm,
};
enum class BodyState
{
	Standing,
	Flying,
	Kneeling,
	Prone,
	Jumping,
	Throwing,
	Downed,
	Dead
};
enum class HandState
{
	AtEase,
	Aiming,
	Firing
};
enum class MovementState
{
	None,
	Normal,
	Running,
	Strafing,
	Reverse,
	Brainsuck,
};

class AgentStats
{
  public:
	AgentStats() = default;
	int health = 0;
	int accuracy = 0;
	int reactions = 0;
	int speed = 0;
	void setSpeed(int value)
	{
		speed = value;
		restoreTU();
	}
	int getActualSpeedValue() const { return (speed + 4) / 8; }
	int getMovementSpeed() const { return clamp(getActualSpeedValue(), 5, 14); }
	int getDisplaySpeedValue() const { return 8 * getActualSpeedValue(); }
	int time_units = 0;
	void restoreTU() { time_units = speed; }
	int stamina = 0;
	int getDisplayStaminaValue() const { return stamina / 20; }
	bool canRun() const { return stamina > 5; }
	int bravery = 0;
	int strength = 0;
	int morale = 0;
	void loseMorale(int value)
	{
		morale -= value;
		if (morale < 0)
			morale = 0;
	}
	void gainMorale(int value)
	{
		morale += value;
		if (morale > 100)
			morale = 100;
	}
	int psi_energy = 0;
	int psi_attack = 0;
	int psi_defence = 0;

	int physics_skill = 0;
	int biochem_skill = 0;
	int engineering_skill = 0;
};

class AgentPortrait
{
  public:
	sp<Image> photo;
	sp<Image> icon;
};

class AgentEquipmentLayout : public StateObject<AgentEquipmentLayout>
{
  public:
	std::list<EquipmentLayoutSlot> slots;
};

class AgentType : public StateObject<AgentType>
{
  public:
	enum class Role
	{
		Soldier,
		Physicist,
		BioChemist,
		Engineer,
	};
	enum class Gender
	{
		Male,
		Female,
	};
	static EquipmentSlotType getArmorSlotType(BodyPart bodyPart);
	// Enums for animation

	AgentType();

	UString id;
	UString name;
	Role role = Role::Soldier;

	std::set<Gender> possible_genders;
	std::map<Gender, float> gender_chance;
	std::map<Gender, std::map<int, AgentPortrait>> portraits;

	AgentStats min_stats;
	AgentStats max_stats;

	// Defines size, voxel maps, allowed states and facings
	StateRef<AgentBodyType> bodyType;

	StateRef<BattleUnitImagePack> shadow_pack;
	// A unit can have more than one appearance.
	// Examples are:
	// - Gang members, who have two attires - red and pink
	// - Androids, who can have 4 different heads
	// - Also Chrysalises and Multiworm eggs work this way since their bodies are immobile,
	// but can face in several directions
	int appearance_count = 0;
	std::vector<std::map<BodyPart, StateRef<BattleUnitImagePack>>> image_packs;
	std::vector<StateRef<BattleUnitAnimationPack>> animation_packs;

	std::map<BodyPart, int> armor;
	StateRef<DamageModifier> damage_modifier;

	bool inventory = false;
	// Only used if no inventory
	StateRef<AEquipmentType> built_in_weapon_left;
	StateRef<AEquipmentType> built_in_weapon_right;

	StateRef<AgentEquipmentLayout> equipment_layout;

	EquipmentLayoutSlot *getFirstSlot(EquipmentSlotType type);

	// Percentage improvement rate
	int improvementPercentagePhysical = 0;
	// Percentage improvement rate
	int improvementPercentagePsi = 0;
	// Can be assigned to training
	bool canTrain = false;
	// Is agent immune to brainsuckers
	bool immuneToBrainsuckers = false;
	// Can this be generated for the player
	bool playable = false;
	// Can this be controlled by a player (if false, even when control is gained, AI will act)
	// For example, X-Com base turrets behave this way (yours, give you sight, but can't control)
	bool allowsDirectControl = false;
	// AI type used by this
	AIType aiType;
	// Fatal woulds immunity
	bool immuneToFatalWounds = false;

	int infiltrationSpeed = 0;
	int growthChance = 0;
	std::list<std::pair<int, std::pair<StateRef<AgentType>, int>>> growthOptions;
	// Apply this infiltration when alien grows
	int growthInfiltration = 0;
	int detectionWeight = 0;
	int movementPercent = 0;

	StateRef<DamageType> spreadHazardDamageType;
	int spreadHazardMinPower = 0;
	int spreadHazardMaxPower = 0;
	int spreadHazardTTLDivizor = 0;

	// Background used for agent equipment screen
	sp<Image> inventoryBackground;
	// Whether agent's rank should be displayed in the equipment screen
	bool displayRank = false;

	// This agent must be killed to disable the building it's in
	bool missionObjective = false;

	StateRef<AEquipmentType> liveSpeciesItem;
	StateRef<AEquipmentType> deadSpeciesItem;

	// Sounds unit makes when walking, overrides terrain's walk sounds if present
	std::vector<sp<Sample>> walkSfx;
	// Sounds unit randomly makes when acting, used by aliens
	std::list<sp<Sample>> crySfx;
	// Sounds unit emits when taking non-fatal damage
	std::map<Gender, std::list<sp<Sample>>> damageSfx;
	// Sounds unit emits when taking fatal damage
	std::map<Gender, std::list<sp<Sample>>> fatalWoundSfx;
	std::map<Gender, std::list<sp<Sample>>> dieSfx;

	int score = 0;

	// Following members are not serialized, but rather are set in initState method

	sp<Sample> gravLiftSfx;
};

class AgentBodyType : public StateObject<AgentBodyType>
{
  public:
	// This, among others, determines whether unit has built-in hover capability, can can be
	// overridden by use of certain armor
	std::set<BodyState> allowed_body_states;
	// Allowed movement states for the unit
	// If unit is to be allowed to move at all, it should have at least Normal or Running movement
	// state allowed
	std::set<MovementState> allowed_movement_states;
	// Allowed movement stats from which unit can fire
	std::set<MovementState> allowed_fire_movement_states;
	// Allowed facings, for every appearance. Empty means every facing is allowed
	std::vector<std::set<Vec2<int>>> allowed_facing;

	// Unit is large and will be treated accordingly
	bool large = false;
	// Unit's height, used in pathfinding
	int maxHeight = 0;
	// Unit's height in each body state, used when displaying unit selection arrows
	// as well as for determining collision model height
	std::map<BodyState, int> height;
	// Unit's muzzle location in each body state, used when firing
	std::map<BodyState, int> muzzleZPosition;

	// Voxel maps (x,y,z) for each body state and facing of the agent
	std::map<BodyState, std::map<Vec2<int>, Vec3<int>>> size;
	// Voxel maps for each body state and facing of the agent
	std::map<BodyState, std::map<Vec2<int>, std::vector<sp<VoxelMap>>>> voxelMaps;

	BodyState getFirstAllowedState();
};

} // namespace OpenApoc
