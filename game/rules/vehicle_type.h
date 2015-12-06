#pragma once
#include "library/sp.h"

#include "library/vec.h"
#include "library/rect.h"
#include "library/strings.h"
#include "game/rules/vequipment.h"
#include <vector>
#include <map>
#include <set>
#include <list>

namespace OpenApoc
{

class RulesLoader;
class Image;
class VoxelMap;

class VehicleType
{
  public:
	enum class Type
	{
		Flying,
		Ground,
		UFO,
	};
	enum class Direction
	{
		N,
		NE,
		E,
		SE,
		S,
		SW,
		W,
		NW,
	};
	enum class Banking
	{
		Flat,
		Left,
		Right,
		Ascending,
		Descending,
	};
	enum class ArmourDirection
	{
		Top,
		Bottom,
		Front,
		Rear,
		Left,
		Right,
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

	Type type;
	UString id;

	UString name;
	UString manufacturer;

	Vec3<float> size;
	Vec2<float> image_offset;
	float acceleration;
	float top_speed;
	float health;
	float crash_health;
	float weight;
	std::map<ArmourDirection, float> armour;
	int passengers;
	float aggressiveness;
	int score;
	UString equipment_screen_path;
	sp<Image> equipment_screen;
	UString icon_path;
	sp<Image> icon;

	// All vehicles (flying,ground,ufo) have strategy sprites
	std::map<Direction, UString> strategy_sprite_paths;
	std::vector<std::pair<Vec3<float>, sp<Image>>> directional_strategy_sprites;

	// Flying and ground vehicles have a directional sprite (with optional non-flat banking)
	std::map<Banking, std::map<Direction, UString>> sprite_paths;
	std::vector<std::pair<Vec3<float>, sp<Image>>> directional_sprites;

	// Flying vehicles and UFOs have a shadow
	Vec2<float> shadow_offset;
	std::map<Direction, UString> shadow_sprite_paths;
	std::vector<std::pair<Vec3<float>, sp<Image>>> directional_shadow_sprites;

	// UFOs have a non-directional animated sprite
	std::list<UString> animation_sprite_paths;
	std::list<sp<Image>> animation_sprites;
	// UFOs also have a 'crashed' sprite
	UString crashed_sprite_path;
	sp<Image> crashed_sprite;

	class EquipmentLayoutSlot
	{
	  public:
		VEquipmentType::Type type;
		AlignmentX align_x;
		AlignmentY align_y;
		Rect<int> bounds;
		EquipmentLayoutSlot(VEquipmentType::Type type, AlignmentX align_x, AlignmentY align_y,
		                    Rect<int> bounds)
		    : type(type), align_x(align_x), align_y(align_y), bounds(bounds)
		{
		}
	};
	std::list<EquipmentLayoutSlot> equipment_layout_slots;
	std::list<std::pair<Vec2<int>, UString>> initial_equipment_list;

	virtual ~VehicleType() = default;
	virtual bool isValid(Framework &fw, Rules &rules);

  private:
	VehicleType(Type type, const UString &id);
	friend class RulesLoader;
};
}; // namespace OpenApoc
