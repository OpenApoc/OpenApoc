#pragma once
#include "game/state/agent.h"
#include "game/state/battle/battleunitimagepack.h"
#include "game/state/organisation.h"
#include "game/state/research.h"
#include "game/state/rules/damage.h"
#include "game/state/stateobject.h"
#include "library/strings.h"
#include "library/vec.h"
#include <map>
#include <set>

namespace OpenApoc
{
class Rules;
class Image;
class Sample;
class AEquipmentType : public StateObject<AEquipmentType>
{
  public:
	enum class Type
	{
		Armor,
		Weapon,
		Grenade,
		Ammo,
		MotionScanner,
		// This is in the game files, but no item info is present at all
		StructureProbe,
		// This is in the game files, but no item info is present at all
		VortexAnalyzer,
		MultiTracker,
		MindShield,
		MindBender,
		AlienDetector,
		DisruptorShield,
		Teleporter,
		CloakingField,
		DimensionForceField,
		MediKit,
		// For Psi-clones and stuff
		Loot
	};
	enum class TriggerType
	{
		Normal,
		Proximity,
		Boomeroid
	};

	~AEquipmentType() override = default;

	// Shared stuff
	Type type = Type::Loot;
	UString id;
	UString name;
	int weight = 0;
	std::vector<sp<Image>> held_sprites;
	sp<Image> dropped_sprite;
	sp<Image> equipscreen_sprite;
	Vec2<int> equipscreen_size;
	StateRef<Organisation> manufacturer;
	int store_space = 0;
	int armor = 0;
	int score = 0;
	ResearchDependency research_dependency;

	// Armor only
	StateRef<DamageModifier> damage_modifier;
	AgentType::BodyPart body_part = AgentType::BodyPart::Body;
	StateRef<BattleUnitImagePack> image_pack;
	bool provides_flight = false;

	// Weapon & Grenade only
	// For weapons with built-in ammo and for grenades leave this empty
	std::set<StateRef<AEquipmentType>> ammo_types;

	// Ammo, Weapons & Grenades with built-in ammo, General with charge
	int max_ammo = 0;
	int recharge = 0;

	// Ammo, Weapons & Grenades with built-in ammo only
	int speed = 0;
	// A list of sprites forming the projectile 'bullet'/'beam' - 'nullptr' gaps are expected
	std::list<sp<Image>> projectile_sprites;
	int damage = 0;
	int accuracy = 0;
	int fire_delay = 0;
	int tail_size = 0;
	bool guided = false;
	int turn_rate = 0;
	int range = 0;
	int ttl = 0;
	int explosion_graphic = 0;
	sp<Sample> fire_sfx;
	sp<Sample> impact_sfx;
	StateRef<DamageType> damage_type;
	TriggerType trigger_type = TriggerType::Normal;
	int explosion_depletion_rate = 0;
};

class EquipmentSet : public StateObject<EquipmentSet>
{
  public:
	class WeaponData
	{
	  public:
		StateRef<AEquipmentType> weapon;
		StateRef<AEquipmentType> clip;
		int clip_amount = 0;

		WeaponData() = default;
		WeaponData(StateRef<AEquipmentType> weapon) : WeaponData(weapon, nullptr, 0) {}
		WeaponData(StateRef<AEquipmentType> weapon, StateRef<AEquipmentType> clip, int clip_amount)
		    : weapon(weapon), clip(clip), clip_amount(clip_amount)
		{
		}
	};
	class GrenadeData
	{
	  public:
		StateRef<AEquipmentType> grenade;
		int grenade_amount = 0;

		GrenadeData() = default;
		GrenadeData(StateRef<AEquipmentType> grenade, int grenade_amount)
		    : grenade(grenade), grenade_amount(grenade_amount)
		{
		}
	};
	class EquipmentData
	{
	  public:
		std::list<StateRef<AEquipmentType>> equipment;
		EquipmentData() = default;
		EquipmentData(StateRef<AEquipmentType> item) : EquipmentData(item, nullptr) {}
		EquipmentData(StateRef<AEquipmentType> item1, StateRef<AEquipmentType> item2)
		{
			if (item1)
				equipment.push_back(item1);
			if (item2)
				equipment.push_back(item2);
		}
	};
	UString id;

	int min_score = INT_MIN;
	int max_score = INT_MAX;
	bool is_appropriate(int score) { return score >= min_score && score < max_score; };

	std::vector<WeaponData> weapons;
	std::vector<GrenadeData> grenades;
	std::vector<EquipmentData> equipment;

	std::list<sp<AEquipmentType>> generateEquipmentList(GameState &state);

	static sp<EquipmentSet> getByScore(const GameState &state, const int score);
	static sp<EquipmentSet> getByLevel(const GameState &state, const int level);
};

} // namespace OpenApoc