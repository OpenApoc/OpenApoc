#pragma once

#include "framework/logger.h"
#include "game/state/rules/city/vequipmenttype.h"
#include "game/state/shared/equipment.h"
#include "game/state/stateobject.h"
#include "library/rect.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <list>
#include <map>
#include <queue>
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

class VehicleType : public StateObject<VehicleType>
{
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

	static VehicleType::Direction getDirectionLarge(float facing);
	static VehicleType::Direction getDirectionSmall(float facing);

	template <class IterT> int getMaxConstitution(IterT first, IterT last) const
	{
		return getMaxHealth(first, last) + getMaxShield(first, last);
	}
	template <class IterT>
	int getMaxHealth(IterT first [[maybe_unused]], IterT last [[maybe_unused]]) const
	{
		static_assert(std::is_same<typename std::iterator_traits<IterT>::value_type,
		                           sp<VEquipmentType>>::value,
		              "iterator must return sp<VehicleEquipmentType>");
		return this->health;
	}
	template <class IterT> int getMaxShield(IterT first, IterT last) const
	{
		static_assert(std::is_same<typename std::iterator_traits<IterT>::value_type,
		                           sp<VEquipmentType>>::value,
		              "iterator must return sp<VehicleEquipmentType>");
		int maxShield = 0;

		while (first != last)
		{
			sp<VEquipmentType> vet = *first;
			if (vet->type == EquipmentSlotType::VehicleGeneral)
			{
				maxShield += vet->shielding;
			}
			++first;
		}
		return maxShield;
	}
	// This is the 'sum' of all armors?
	template <class IterT>
	int getArmor(IterT first [[maybe_unused]], IterT last [[maybe_unused]]) const
	{
		static_assert(std::is_same<typename std::iterator_traits<IterT>::value_type,
		                           sp<VEquipmentType>>::value,
		              "iterator must return sp<VehicleEquipmentType>");
		int armor = 0;
		for (auto &armorDirection : this->armour)
		{
			armor += armorDirection.second;
		}
		return armor;
	}
	template <class IterT> int getAccuracy(IterT first, IterT last) const
	{
		static_assert(std::is_same<typename std::iterator_traits<IterT>::value_type,
		                           sp<VEquipmentType>>::value,
		              "iterator must return sp<VehicleEquipmentType>");
		int accuracy = 0;
		std::priority_queue<int> accModifiers;

		while (first != last)
		{
			sp<VEquipmentType> vet = *first;
			if (vet->type == EquipmentSlotType::VehicleGeneral && vet->accuracy_modifier > 0)
			{
				// accuracy percentages are inverted in the data (e.g. 10% module gives 90)
				accModifiers.push(100 - vet->accuracy_modifier);
			}
			++first;
		}

		int moduleEfficiency = 1;
		while (!accModifiers.empty())
		{
			accuracy += accModifiers.top() / moduleEfficiency;
			accModifiers.pop();
			moduleEfficiency *= 2;
		}
		return accuracy;
	}
	template <class IterT> int getTopSpeed(IterT first, IterT last) const
	{
		static_assert(std::is_same<typename std::iterator_traits<IterT>::value_type,
		                           sp<VEquipmentType>>::value,
		              "iterator must return sp<VehicleEquipmentType>");
		return (int)getSpeed(first, last);
	}
	template <class IterT> int getAcceleration(IterT first, IterT last) const
	{
		static_assert(std::is_same<typename std::iterator_traits<IterT>::value_type,
		                           sp<VEquipmentType>>::value,
		              "iterator must return sp<VehicleEquipmentType>");
		int weight = this->getWeight(first, last);
		int power = 0;
		while (first != last)
		{
			sp<VEquipmentType> vet = *first;
			if (vet->type == EquipmentSlotType::VehicleEngine)
			{
				power += vet->power;
			}
			++first;
		}
		if (weight == 0)
		{
			return 0;
		}
		int acceleration = this->acceleration + std::max(1, power / weight);
		if (power == 0 && acceleration == 0)
		{
			// No engine shows a '0' acceleration in the stats ui
			return 0;
		}
		return acceleration;
	}
	template <class IterT> int getWeight(IterT first, IterT last) const
	{
		static_assert(std::is_same<typename std::iterator_traits<IterT>::value_type,
		                           sp<VEquipmentType>>::value,
		              "iterator must return sp<VehicleEquipmentType>");
		int weight = this->weight;
		while (first != last)
		{
			weight += (*first)->weight;
			++first;
		}
		if (weight == 0)
		{
			LogError("Vehicle with no weight");
		}
		return weight;
	}
	template <class IterT> int getMaxFuel(IterT first, IterT last) const
	{
		static_assert(std::is_same<typename std::iterator_traits<IterT>::value_type,
		                           sp<VEquipmentType>>::value,
		              "iterator must return sp<VehicleEquipmentType>");
		// Zero fuel is normal on some vehicles (IE ufos/'dimension-capable' xcom)
		int fuel = 0;

		while (first != last)
		{
			sp<VEquipmentType> vet = *first;
			if (vet->type == EquipmentSlotType::VehicleEngine)
			{
				fuel += vet->max_ammo;
			}
			++first;
		}
		return fuel;
	}
	template <class IterT> int getMaxPassengers(IterT first, IterT last) const
	{
		static_assert(std::is_same<typename std::iterator_traits<IterT>::value_type,
		                           sp<VEquipmentType>>::value,
		              "iterator must return sp<VehicleEquipmentType>");
		int passengers = this->passengers;
		while (first != last)
		{
			if ((*first)->type == EquipmentSlotType::VehicleGeneral)
			{
				passengers += (*first)->passengers;
			}
			++first;
		}
		return passengers;
	}
	template <class IterT> int getMaxCargo(IterT first, IterT last) const
	{
		static_assert(std::is_same<typename std::iterator_traits<IterT>::value_type,
		                           sp<VEquipmentType>>::value,
		              "iterator must return sp<VehicleEquipmentType>");

		int cargoMax = 0;
		while (first != last)
		{
			if ((*first)->type == EquipmentSlotType::VehicleGeneral)
			{
				cargoMax += (*first)->cargo_space;
			}
			++first;
		}
		return cargoMax;
	}
	template <class IterT> int getMaxBio(IterT first, IterT last) const
	{
		static_assert(std::is_same<typename std::iterator_traits<IterT>::value_type,
		                           sp<VEquipmentType>>::value,
		              "iterator must return sp<VehicleEquipmentType>");

		int cargoMax = 0;
		while (first != last)
		{
			if ((*first)->type == EquipmentSlotType::VehicleGeneral)
			{
				cargoMax += (*first)->alien_space;
			}
			++first;
		}
		return cargoMax;
	}
	template <class IterT> float getSpeed(IterT first, IterT last) const
	{
		static_assert(std::is_same<typename std::iterator_traits<IterT>::value_type,
		                           sp<VEquipmentType>>::value,
		              "iterator must return sp<VehicleEquipmentType>");

		// FIXME: This is somehow modulated by weight?
		float speed = this->top_speed;
		while (first != last)
		{
			if ((*first)->type == EquipmentSlotType::VehicleEngine)
			{
				speed += (*first)->top_speed;
			}
			++first;
		}
		return speed;
	}

	// This is explicitly mutable it can be used through a const ref
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

	MapIconType mapIconType = MapIconType::Arrow;

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
	bool canEnterDimensionGate = false;

	~VehicleType() override = default;
	VehicleType() = default;
};
}; // namespace OpenApoc
