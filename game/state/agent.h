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

class AgentStats
{
  public:
	AgentStats() = default;
	int health = 0;
	int accuracy = 0;
	int reactions = 0;
	int speed = 0;
	int stamina = 0;
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
	std::map<AgentType::Gender, float> gender_chance;
	std::map<Gender, std::map<int, AgentPortrait>> portraits;

	AgentStats min_stats;
	AgentStats max_stats;

	// This, among others, determines wether unit has built-in hover capability, can can be overriden by use of certain armor
	std::set<BodyState> allowed_body_states;
	std::set<MovementState> allowed_movement_states;
	// Unit is large and will be treated accordingly
	bool large = false;

	sp<VoxelMap> voxelMap;

	StateRef<BattleUnitImagePack> shadow_pack;
	std::map<BodyPart, StateRef<BattleUnitImagePack>> image_packs;
	StateRef<BattleUnitAnimationPack> animation_pack;

	std::map<BodyPart, int> armor;
	StateRef<DamageModifier> damage_modifier;

	bool inventory = false;
	// Only used if no inventory
	StateRef<AEquipmentType> built_in_weapon_left;
	StateRef<AEquipmentType> built_in_weapon_right;

	class EquipmentLayoutSlot
	{
	  public:
		AlignmentX align_x = AlignmentX::Left;
		AlignmentY align_y = AlignmentY::Top;
		Rect<int> bounds;
		EquipmentLayoutSlot() = default;
		EquipmentLayoutSlot(AlignmentX align_x, AlignmentY align_y, Rect<int> bounds)
		    : align_x(align_x), align_y(align_y), bounds(bounds)
		{
		}
	};
	std::list<EquipmentLayoutSlot> equipment_layout_slots;

	bool can_improve = false;
	// Can this be generated for the player
	bool playable = false;

	int score = 0;
};

class Agent : public StateObject<Agent>
{
  public:
	Agent() = default;

	StateRef<AgentType> type;

	UString name;

	int portrait;
	AgentPortrait get_portrait() { return type->portraits[gender][portrait]; }
	AgentType::Gender gender = AgentType::Gender::Male;
	
	// Stats when agent was recruited
	AgentStats initial_stats;
	// Current max stats of the agent, before taking into account damage, exhaustion etc.
	AgentStats max_stats;
	// Current stats of the agent, tracking expenditure such as movement, psi attacks and so on.
	AgentStats current_stats;

	StateRef<Base> home_base;
	StateRef<Organisation> owner;

	bool assigned_to_lab = false;

	std::list<sp<AEquipment>> equipment;
	bool canAddEquipment(Vec2<int> pos, StateRef<AEquipmentType> type) const;
	void addEquipment(GameState &state, Vec2<int> pos, StateRef<AEquipmentType> type);
	void addEquipment(GameState &state, Vec2<int> pos, sp<AEquipment> object);
	void removeEquipment(sp<AEquipment> object);

	void updateSpeed();
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
	StateRef<Agent> createAgent(GameState &state, AgentType::Role role) const;
	// Create an agent of specified type
	StateRef<Agent> createAgent(GameState &state, StateRef<AgentType> type) const;
};

} // namespace OpenApoc
