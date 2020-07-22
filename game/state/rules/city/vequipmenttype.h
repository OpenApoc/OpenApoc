#pragma once

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

class Rules;
class Image;
class Sample;
class DoodadType;
class Organisation;
class VAmmoType;

class VEquipmentType : public StateObject<VEquipmentType>
{
  public:
	VEquipmentType() = default;

	enum class User
	{
		Ground,
		Air,
		Ammo,
	};

	~VEquipmentType() override = default;

	// Shared stuff
	EquipmentSlotType type = EquipmentSlotType::VehicleGeneral;
	UString id;
	UString name;
	int weight = 0;
	int max_ammo = 0;
	StateRef<VAmmoType> ammo_type;
	sp<Image> equipscreen_sprite;
	Vec2<int> equipscreen_size = {0, 0};
	StateRef<Organisation> manufacturer;
	int store_space = 0;
	std::set<User> users;

	// Weapons
	int speed = 0;
	std::list<sp<Image>> projectile_sprites; // A list of sprites forming the projectile
	                                         // 'bullet'/'beam' - 'nullptr' gaps are expected
	int damage = 0;
	int accuracy = 0;
	// Fire delay, in ticks, to fire a shot
	int fire_delay = 0;
	int tail_size = 0;
	bool guided = false;
	// How much can it turn per tick.
	// Based on the fact that retribution (tr = 10) turns 90 degrees (PI/2) per second
	// One point of turn rate is equal to PI/20 turned per second
	int turn_rate = 0;
	// Divide by 32 to get range in tiles
	int range = 0;
	// Projectile's ttl, in voxels travelled
	float ttl = 0.0f;
	int firing_arc_1 = 0;
	int firing_arc_2 = 0;
	bool point_defence = false;
	sp<Sample> fire_sfx;
	sp<Sample> impact_sfx;
	StateRef<DoodadType> explosion_graphic;
	sp<Image> icon;

	// Special weapons
	int stunTicks = 0;
	std::list<StateRef<VEquipmentType>> splitIntoTypes;

	// Engine stuff
	int power = 0;
	int top_speed = 0;

	// Other ('general') equipment stuff
	int accuracy_modifier = 0;
	int cargo_space = 0;
	int passengers = 0;
	int alien_space = 0;
	int missile_jamming = 0;
	int shielding = 0;
	bool cloaking = false;
	bool teleporting = false;
	bool dimensionShifting = false;

	// Score requirement
	int scoreRequirement = 0;

	int getRangeInTiles() const;
	int getRangeInMetres() const;
};

} // namespace OpenApoc
