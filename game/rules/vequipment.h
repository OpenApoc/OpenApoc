#pragma once
#include "game/organisation.h"
#include "game/stateobject.h"
#include "library/strings.h"
#include "library/vec.h"
#include <set>

namespace OpenApoc
{

class Rules;
class Image;
class Sample;
class VEquipmentType : public StateObject<VEquipmentType>
{
  public:
	VEquipmentType();
	enum class Type
	{
		Engine,
		Weapon,
		General,
	};
	static const std::map<Type, UString> TypeMap;

	enum class User
	{
		Ground,
		Air,
		Ammo,
	};
	static const std::map<User, UString> UserMap;

	virtual ~VEquipmentType() = default;

	// Shared stuff
	Type type;
	UString id;
	UString name;
	int weight;
	int max_ammo;
	UString ammo_type;
	sp<Image> equipscreen_sprite;
	Vec2<int> equipscreen_size;
	StateRef<Organisation> manufacturer;
	int store_space;
	std::set<User> users;

	// Weapons
	int speed;
	int projectile_image; // FIXME: What is this?
	int damage;
	int accuracy;
	int fire_delay;
	int tail_size;
	bool guided;
	int turn_rate;
	int range;
	int firing_arc_1;
	int firing_arc_2;
	bool point_defence;
	sp<Sample> fire_sfx;
	int explosion_graphic;
	sp<Image> icon;

	// Engine stuff
	int power;
	int top_speed;

	// Other ('general') equipment stuff
	int accuracy_modifier;
	int cargo_space;
	int passengers;
	int alien_space;
	int missile_jamming;
	int shielding;
	bool cloaking;
	bool teleporting;
};

} // namespace OpenApoc
