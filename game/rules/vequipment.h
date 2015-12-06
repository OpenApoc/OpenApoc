#pragma once
#include "library/vec.h"
#include "library/strings.h"
#include <set>

namespace OpenApoc
{

class Rules;
class Image;
class Framework;
class Sample;
class VEquipmentType
{
  public:
	enum class Type
	{
		Engine,
		Weapon,
		General,
	};

	enum class User
	{
		Ground,
		Air,
	};

	virtual ~VEquipmentType() = default;
	virtual bool isValid(Framework &fw, Rules &rules);

	Type type;
	UString id;
	UString name;
	int weight;
	int max_ammo;
	UString ammo_type;
	UString equipscreen_sprite_name;
	sp<Image> equipscreen_sprite;
	Vec2<int> equipscreen_size;
	UString manufacturer;
	int store_space;
	std::set<User> users;

  protected:
	VEquipmentType(Type type, const UString &id);
};

class VWeaponType : public VEquipmentType
{
  private:
	friend class RulesLoader;
	VWeaponType(const UString &id);

  public:
	virtual ~VWeaponType() = default;
	bool isValid(Framework &fw, Rules &rules) override;

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
	UString fire_sfx_path;
	sp<Sample> fire_sfx;
	UString explosion_graphic;
	UString icon_path;
	sp<Image> icon;
};

class VEngineType : public VEquipmentType
{
  private:
	friend class RulesLoader;
	VEngineType(const UString &id);

  public:
	virtual ~VEngineType() = default;
	bool isValid(Framework &fw, Rules &rules) override;

	int power;
	int top_speed;
};

class VGeneralEquipmentType : public VEquipmentType
{
  private:
	friend class RulesLoader;
	VGeneralEquipmentType(const UString &id);

  public:
	virtual ~VGeneralEquipmentType() = default;
	bool isValid(Framework &fw, Rules &rules) override;

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
