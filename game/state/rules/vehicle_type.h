#pragma once
#include "game/state/rules/vequipment_type.h"
#include "game/state/stateobject.h"
#include "library/rect.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <list>
#include <map>
#include <vector>

namespace OpenApoc
{

class RulesLoader;
class Image;
class VoxelMap;

class VehicleType : public StateObject<VehicleType>
{
  public:
	enum class Type
	{
		Flying,
		Ground,
		UFO,
	};
	static const std::map<Type, UString> TypeMap;
	enum class Direction
	{
		N,
		NNE,
		NE,
		NEE,
		E,
		SEE,
		SE,
		SSE,
		S,
		SSW,
		SW,
		SWW,
		W,
		NWW,
		NW,
		NNW
	};
	static const std::map<Direction, UString> DirectionMap;
	static const Vec3<float> &directionToVector(Direction);
	enum class Banking
	{
		Flat,
		Left,
		Right,
		Ascending,
		Descending,
	};
	static const std::map<Banking, UString> BankingMap;
	enum class ArmourDirection
	{
		Top,
		Bottom,
		Front,
		Rear,
		Left,
		Right,
	};
	static const std::map<ArmourDirection, UString> ArmourDirectionMap;
	enum class AlignmentX
	{
		Left,
		Right,
		Centre,
	};
	static const std::map<AlignmentX, UString> AlignmentXMap;
	enum class AlignmentY
	{
		Top,
		Bottom,
		Centre,
	};
	static const std::map<AlignmentY, UString> AlignmentYMap;

	// This is explictly mutable it can be used through a const ref
	// FIXME: Should this go somewhere else in the state? If the rules are meant to be immutable
	// this may be lost after serialisation?
	mutable unsigned numCreated;

	Type type;

	UString name;
	StateRef<Organisation> manufacturer;

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
	sp<Image> icon;

	// The following (equip screen, equip icon big and small) are only required
	// for vehicles able to be used by the player
	sp<Image> equipment_screen;

	sp<Image> equip_icon_big;

	sp<Image> equip_icon_small;

	// All vehicles (flying,ground,ufo) have strategy sprites
	std::map<Vec3<float>, sp<Image>> directional_strategy_sprites;

	// Flying and ground vehicles have a directional sprite (with optional non-flat banking)
	std::map<Banking, std::map<Vec3<float>, sp<Image>>> directional_sprites;

	// Flying vehicles and UFOs have a shadow
	Vec2<float> shadow_offset;
	std::map<Vec3<float>, sp<Image>> directional_shadow_sprites;

	// UFOs have a non-directional animated sprite
	std::list<sp<Image>> animation_sprites;
	// UFOs also have a 'crashed' sprite
	sp<Image> crashed_sprite;

	sp<VoxelMap> voxelMap;

	class EquipmentLayoutSlot
	{
	  public:
		VEquipmentType::Type type;
		AlignmentX align_x;
		AlignmentY align_y;
		Rect<int> bounds;
		EquipmentLayoutSlot() = default;
		EquipmentLayoutSlot(VEquipmentType::Type type, AlignmentX align_x, AlignmentY align_y,
		                    Rect<int> bounds)
		    : type(type), align_x(align_x), align_y(align_y), bounds(bounds)
		{
		}
	};
	std::list<EquipmentLayoutSlot> equipment_layout_slots;
	std::list<std::pair<Vec2<int>, StateRef<VEquipmentType>>> initial_equipment_list;

	virtual ~VehicleType() = default;
	VehicleType();
};
}; // namespace OpenApoc
