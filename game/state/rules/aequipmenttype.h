#pragma once

#include "game/state/city/research.h"
#include "game/state/rules/agenttype.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <limits>
#include <map>
#include <set>

namespace OpenApoc
{
class Rules;
class Image;
class Sample;
class DoodadType;
class BattleUnitImagePack;
class Organisation;
class DamageType;
class DamageModifier;
class AgentType;
class UfopaediaEntry;

enum class TriggerType
{
	None,
	Timed,
	Contact,
	Proximity,
	Boomeroid
};

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
		// This is not in the game files, but we need it to function
		Brainsucker,
		Popper,
		Spawner,
		// For Psi-clones and stuff
		Loot
	};

	AEquipmentType() = default;
	~AEquipmentType() override = default;

	// Shared stuff
	Type type = Type::Loot;
	UString id;
	UString name;
	int weight = 0;
	StateRef<BattleUnitImagePack> held_image_pack;
	sp<Image> dropped_sprite;
	Vec2<float> dropped_offset = {0, 0};
	sp<Image> dropped_shadow_sprite;
	Vec2<float> shadow_offset = {0, 0};
	sp<Image> equipscreen_sprite;
	Vec2<int> equipscreen_size = {0, 0};
	StateRef<Organisation> manufacturer;
	int store_space = 0;
	int armor = 0;
	int score = 0;
	ResearchDependency research_dependency;
	// Whether item is carried two-handed (for display purposes)
	bool two_handed = false;

	// Item goes to alien containment
	bool bioStorage = false;
	// Item that remains after this is researched
	StateRef<AEquipmentType> bioRemains;

	// True = when fired from weapon will throw ammunition to target location instead of firing
	// properly
	bool launcher = false;

	// Alien artifact flag (does not show in economy unless you own it)
	bool artifact = false;

	// Armor only
	sp<Image> body_sprite;
	StateRef<DamageModifier> damage_modifier;
	BodyPart body_part = BodyPart::Body;
	StateRef<BattleUnitImagePack> body_image_pack;
	bool provides_flight = false;

	// Weapon & Grenade only
	// For weapons with built-in ammo and for grenades leave this empty
	// This is not stored in files, but rather filled in gamestate init
	// In files we only store ammo's link to the weapon
	// This way, modders can introduce ammunition for existing weapons without having mod conflicts
	std::set<StateRef<AEquipmentType>> ammo_types;

	// Ammo only
	std::list<StateRef<AEquipmentType>> weapon_types;

	// Ammo, Weapons & Grenades with built-in ammo, General with charge
	int max_ammo = 0;
	int burst = 1;
	int recharge = 0;
	int rechargeTB = 0;

	// Ammo, Weapons & Grenades with built-in ammo only
	int speed = 0;
	// A list of sprites forming the projectile 'bullet'/'beam' - 'nullptr' gaps are expected
	std::list<sp<Image>> projectile_sprites;
	int damage = 0;
	int accuracy = 0;

	/* This is how many ticks it takes to fire a single shot.
	 *
	 * For fire rate displayed ingame (equipment screen), formula is 1000/FEDL
	 * For example:
	 * Ingame Toxigun has 125, in file it has 8, 125*8=1000.
	 * Ingame Minigun has 83, in file it has 12, 83*12=996. (rounding 1000/12 down makes it 83)
	 *
	 * For fire rate displayed ingame (ufopaedia screen), formula is 36/FEDL
	 * For example:
	 * Ingame Toxigun has 4.50r/s, in file it has 8, 36/8 = 4.5
	 * Ingame Minigun has 3.00r/s, in file it has 12, 36/12 = 3
	 *
	 * Fire rate expects game ticks to be 36 per second
	 * since we have 144 ticks per second, when using this multiply it by 4 to get actual amount
	 * of ticks required to fire
	 *
	 * Since playable alpha 0.1 this is already in OpenApoc ticks
	 */
	int fire_delay = 0;
	float getRoundsPerSecond() const;

	// This is used for aliens with firing animations that have pre-fire frames
	// Like spitter that first throws his tube forward and only then emits a shot
	// This will
	int projectile_delay = 0;
	int tail_size = 0;
	bool guided = false;
	int turn_rate = 0;
	// Ingame displayed value is this divided by 16 rounded down. Range in tiles
	// Since it's range in tiles, it must be divided by 24 to get the actual range!
	int range = 0;
	/* returns tiles */
	int getRangeInTiles() const;
	int getRangeInMetres() const;

	// Projectile's Time To Live, in voxels travelled
	float ttl = 0.0f;
	StateRef<DoodadType> explosion_graphic;
	StateRef<DoodadType> shield_graphic;
	sp<Sample> fire_sfx;
	sp<Sample> impact_sfx;
	StateRef<DamageType> damage_type;
	TriggerType trigger_type = TriggerType::None;
	int explosion_depletion_rate = 0;

	std::map<StateRef<AgentType>, int> spawnList;

	bool canBeUsed(GameState &state, StateRef<Organisation> user) const;
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
			if (item1.id.length() > 0)
				equipment.push_back(item1);
			if (item2.id.length() > 0)
				equipment.push_back(item2);
		}
	};
	UString id;

	int min_score = std::numeric_limits<int>::min();
	int max_score = std::numeric_limits<int>::max();
	bool isAppropriate(int score) { return score >= min_score && score < max_score; };

	std::vector<WeaponData> weapons;
	std::vector<GrenadeData> grenades;
	std::vector<EquipmentData> equipment;

	std::list<const AEquipmentType *> generateEquipmentList(GameState &state);

	static sp<EquipmentSet> getByScore(const GameState &state, const int score);
	static sp<EquipmentSet> getByLevel(const GameState &state, const int level);
};

class EquipmentTemplate
{
  public:
	class EquipmentTemplateEntry
	{
	  public:
		Vec2<int> pos = {0, 0};
		StateRef<AEquipmentType> type;
		StateRef<AEquipmentType> payloadType;

		EquipmentTemplateEntry() = default;
		EquipmentTemplateEntry(Vec2<int> pos, StateRef<AEquipmentType> type,
		                       StateRef<AEquipmentType> payloadType)
		    : pos(pos), type(type), payloadType(payloadType)
		{
		}
	};

	std::list<EquipmentTemplateEntry> equipment;
};

} // namespace OpenApoc
