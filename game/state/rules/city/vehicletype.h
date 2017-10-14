#pragma once

#include "game/state/rules/city/vequipmenttype.h"
#include "game/state/shared/equipment.h"
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
class BattleMap;
class ResearchTopic;
class AgentType;
class UfopaediaEntry;

class VehicleType : public StateObject
{
	STATE_OBJECT(VehicleType)
  public:
	enum class Type
	{
		Flying,
		UFO,
		Road,
		ATV,
	};
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
	static const Vec3<float> &directionToVector(Direction);
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
	enum class MapIconType
	{
		Arrow = 0,
		SmallCircle = 1,
		LargeCircle = 2
	};

	// This is explictly mutable it can be used through a const ref
	// FIXME: Should this go somewhere else in the state? If the rules are meant to be immutable
	// this may be lost after serialisation?
	mutable unsigned numCreated = 0;

	Type type = Type::Flying;
	bool isGround() const;

	UString name;
	StateRef<Organisation> manufacturer;

	Vec2<float> image_offset = {0, 0};
	float acceleration = 0;
	float top_speed = 0;
	int health = 0;
	int crash_health = 0;
	int weight = 0;
	std::map<ArmourDirection, float> armour;
	int passengers = 0;
	float aggressiveness = 0;
	int score = 0;
	sp<Image> icon;

	// Battle data

	StateRef<BattleMap> battle_map;
	std::map<StateRef<AgentType>, int> crew_downed;
	std::map<StateRef<AgentType>, int> crew_deposit;

	// The following (equip screen, equip icon big and small) are only required
	// for vehicles able to be used by the player
	sp<Image> equipment_screen;

	sp<Image> equip_icon_big;

	sp<Image> equip_icon_small;

	MapIconType mapIconType;

	// Flying and ground vehicles have a directional sprite (with optional non-flat banking)
	std::map<Banking, std::map<Direction, sp<Image>>> directional_sprites;

	// Flying vehicles and UFOs have a shadow
	Vec2<float> shadow_offset = {0, 0};
	std::map<Direction, sp<Image>> directional_shadow_sprites;

	// UFOs have a non-directional animated sprite
	std::list<sp<Image>> animation_sprites;
	// UFOs also have a 'crashed' sprite
	sp<Image> crashed_sprite;

	int height = 0;
	// Vehicle size, depending on facing
	std::map<float, Vec3<int>> size;
	// Vehicle voxel map vector, depending on facing
	// This set of voxelmaps is for projectile collision
	std::map<float, std::vector<sp<VoxelMap>>> voxelMaps;
	// Vehicle voxel map vector, depending on facing
	// This set of voxelmaps is for selecting vehicle with mouseclick
	std::map<float, std::vector<sp<VoxelMap>>> voxelMapsLOS;

	// Gets current facing for purpose of determining size and voxel map
	float getVoxelMapFacing(float direction) const;

	std::list<EquipmentLayoutSlot> equipment_layout_slots;
	std::list<std::pair<Vec2<int>, StateRef<VEquipmentType>>> initial_equipment_list;

	StateRef<UfopaediaEntry> ufopaedia_entry;
	// Unlocks when successful at recovering this
	std::list<StateRef<ResearchTopic>> researchUnlock;

	bool provideFreightAgent = false;
	bool provideFreightCargo = false;
	bool provideFreightBio = false;
	bool canRescueCrashed = false;

	~VehicleType() override = default;
	VehicleType() = default;
};
}; // namespace OpenApoc
