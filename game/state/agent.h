#pragma once
#include "framework/image.h"
#include "game/state/battle/battleunitimagepack.h"
#include "game/state/rules/damage.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include "library/voxel.h"
#include <map>

namespace OpenApoc
{

class Base;
class Organisation;
class AEquipment;
class AEquipmentType;
class BattleUnitAnimationPack;
class Sample;
class AgentBodyType;

class AgentStats
{
  public:
	AgentStats() = default;
	int health = 0;
	int accuracy = 0;
	int reactions = 0;
	int speed = 0;
	int getActualSpeedValue() { return speed / 8; }
	int getDisplaySpeedValue() { return 8 * getActualSpeedValue(); }
	int time_units = 0;
	void restoreTU() { time_units = speed; }
	int stamina = 0;
	bool canRun() { return stamina > 0; }
	int bravery = 0;
	int strength = 0;
	int morale = 0;
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
	enum class EquipmentSlotType
	{
		General,
		ArmorBody,
		ArmorLegs,
		ArmorHelmet,
		ArmorLeftHand,
		ArmorRightHand,
		LeftHand,
		RightHand
	};
	enum class AlignmentX
	{
		Left,
		Right,
		Centre,
	};
	enum class AlignmentY
	{
		Top,
		Bottom,
		Centre,
	};
	class EquipmentLayoutSlot
	{
	  public:
		EquipmentSlotType type = EquipmentSlotType::General;
		AlignmentX align_x = AlignmentX::Left;
		AlignmentY align_y = AlignmentY::Top;
		Rect<int> bounds;
		EquipmentLayoutSlot() = default;
		EquipmentLayoutSlot(AlignmentX align_x, AlignmentY align_y, Rect<int> bounds)
		    : align_x(align_x), align_y(align_y), bounds(bounds)
		{
		}
	};
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
	enum class BodyPart
	{
		Body,
		Legs,
		Helmet,
		LeftArm,
		RightArm,
	};

	// Enums for animation
	enum class BodyState
	{
		Standing,
		Flying,
		Kneeling,
		Prone,
		Jumping,
		Throwing,
		Downed,
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
		Strafing
	};

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

	AgentEquipmentLayout::EquipmentLayoutSlot *
	getFirstSlot(AgentEquipmentLayout::EquipmentSlotType type);

	bool can_improve = false;
	// Can this be generated for the player
	bool playable = false;

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
};

class AgentBodyType : public StateObject<AgentBodyType>
{
  public:
	// This, among others, determines wether unit has built-in hover capability, can can be
	// overriden by use of certain armor
	std::set<AgentType::BodyState> allowed_body_states;
	std::set<AgentType::MovementState> allowed_movement_states;
	std::set<Vec2<int>> allowed_facing;

	// Unit is large and will be treated accordingly
	bool large = false;
	// Unit's height, used in pathfinding
	int maxHeight = 0;
	// Unit's height in each body state, used when displaying unit selection arrows
	std::map<AgentType::BodyState, int> height;

	// Voxel maps (x,y,z) for each body state and facing of the agent
	std::map<AgentType::BodyState, std::map<Vec2<int>, Vec3<int>>> size;
	// Voxel maps for each body state and facing of the agent
	std::map<AgentType::BodyState, std::map<Vec2<int>, std::vector<sp<VoxelMap>>>> voxelMaps;
};

class Agent : public StateObject<Agent>, public std::enable_shared_from_this<Agent>
{
  public:
	Agent() = default;

	StateRef<AgentType> type;

	UString name;

	// Appearance that this specific agent chose from available list of its type
	int appearance = 0;
	int portrait = 0;
	AgentPortrait getPortrait() { return type->portraits[gender][portrait]; }
	AgentType::Gender gender = AgentType::Gender::Male;

	AgentStats initial_stats;  // Stats at agent creatrion
	AgentStats current_stats;  // Stats after agent training/improvement
	AgentStats modified_stats; // Stats after 'temporary' modification (health damage, slowdown due
	                           // to equipment weight, used stamina etc)

	int getArmorValue(AgentType::BodyPart bodyPart) const;
	bool isBodyStateAllowed(AgentType::BodyState bodyState) const;
	bool isMovementStateAllowed(AgentType::MovementState movementState) const;
	bool isFacingAllowed(Vec2<int> facing) const;

	StateRef<Base> home_base;
	StateRef<Organisation> owner;

	bool assigned_to_lab = false;

	std::list<sp<AEquipment>> equipment;
	bool canAddEquipment(Vec2<int> pos, StateRef<AEquipmentType> type) const;
	// Add equipment to the first available slot of any type
	void addEquipment(GameState &state, StateRef<AEquipmentType> type);
	// Add equipment to the first available slot of a specific type
	void addEquipment(GameState &state, StateRef<AEquipmentType> type,
	                  AgentEquipmentLayout::EquipmentSlotType slotType);
	void addEquipment(GameState &state, Vec2<int> pos, StateRef<AEquipmentType> type);
	void addEquipment(GameState &state, Vec2<int> pos, sp<AEquipment> object);
	void removeEquipment(sp<AEquipment> object);
	void updateSpeed();
	bool canRun() { return modified_stats.canRun(); }

	StateRef<BattleUnitAnimationPack> getAnimationPack() const;
	StateRef<AEquipmentType> getItemInHands() const;
	sp<AEquipment> getFirstItemInSlot(AgentEquipmentLayout::EquipmentSlotType type) const;
	StateRef<BattleUnitImagePack> getImagePack(AgentType::BodyPart bodyPart) const;
};

class AgentGenerator
{
  public:
	AgentGenerator() = default;
	// Magic number to make unique agent IDs
	mutable unsigned int num_created = 0;
	// FIXME: I think there should be some kind of 'nationality' stuff going on here
	std::map<AgentType::Gender, std::list<UString>> first_names;
	std::list<UString> second_names;

	// Create an agent of specified role
	StateRef<Agent> createAgent(GameState &state, StateRef<Organisation> org,
	                            AgentType::Role role) const;
	// Create an agent of specified type
	StateRef<Agent> createAgent(GameState &state, StateRef<Organisation> org,
	                            StateRef<AgentType> type) const;
};

} // namespace OpenApoc
