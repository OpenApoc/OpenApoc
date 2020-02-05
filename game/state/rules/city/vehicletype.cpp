#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "game/state/rules/city/vehicletype.h"
#include "game/state/gamestate.h"
#include "library/sp.h"
#include "library/strings.h"
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <map>

namespace OpenApoc
{

namespace
{
static const std::map<VehicleType::Direction, Vec3<float>> DirectionVectors = {
    {VehicleType::Direction::N, glm::normalize(Vec3<float>{0, -1, 0})},
    {VehicleType::Direction::NNE, glm::normalize(Vec3<float>{1, -2, 0})},
    {VehicleType::Direction::NE, glm::normalize(Vec3<float>{1, -1, 0})},
    {VehicleType::Direction::NEE, glm::normalize(Vec3<float>{2, -1, 0})},
    {VehicleType::Direction::E, glm::normalize(Vec3<float>{1, 0, 0})},
    {VehicleType::Direction::SEE, glm::normalize(Vec3<float>{2, 1, 0})},
    {VehicleType::Direction::SE, glm::normalize(Vec3<float>{1, 1, 0})},
    {VehicleType::Direction::SSE, glm::normalize(Vec3<float>{1, 2, 0})},
    {VehicleType::Direction::S, glm::normalize(Vec3<float>{0, 1, 0})},
    {VehicleType::Direction::SSW, glm::normalize(Vec3<float>{-1, 2, 0})},
    {VehicleType::Direction::SW, glm::normalize(Vec3<float>{-1, 1, 0})},
    {VehicleType::Direction::SWW, glm::normalize(Vec3<float>{-2, 1, 0})},
    {VehicleType::Direction::W, glm::normalize(Vec3<float>{-1, 0, 0})},
    {VehicleType::Direction::NWW, glm::normalize(Vec3<float>{-2, -1, 0})},
    {VehicleType::Direction::NW, glm::normalize(Vec3<float>{-1, -1, 0})},
    {VehicleType::Direction::NNW, glm::normalize(Vec3<float>{-1, -2, 0})}};
}

const Vec3<float> &VehicleType::directionToVector(Direction d)
{
	static Vec3<float> fallback = {1, 0, 0};
	auto it = DirectionVectors.find(d);
	if (it == DirectionVectors.end())
	{
		LogError("Failed to find a direction vector for %d", (int)d);
		return fallback;
	}
	return it->second;
}

bool VehicleType::isGround() const { return type == Type::Road || type == Type::ATV; }

template <> sp<VehicleType> StateObject<VehicleType>::get(const GameState &state, const UString &id)
{
	auto it = state.vehicle_types.find(id);
	if (it == state.vehicle_types.end())
	{
		LogError("No vehicle type matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<VehicleType>::getPrefix()
{
	static UString prefix = "VEHICLETYPE_";
	return prefix;
}
template <> const UString &StateObject<VehicleType>::getTypeName()
{
	static UString name = "VehicleType";
	return name;
}
template <>
const UString &StateObject<VehicleType>::getId(const GameState &state, const sp<VehicleType> ptr)
{
	static const UString emptyString = "";
	for (auto &v : state.vehicle_types)
	{
		if (v.second == ptr)
			return v.first;
	}
	LogError("No vehicle type matching pointer %p", ptr.get());
	return emptyString;
}

float VehicleType::getVoxelMapFacing(float facing) const
{
	float closestDiff = FLT_MAX;
	float closestAngle = 0.0f;
	for (auto &p : voxelMaps)
	{
		float d1 = p.first - facing;
		if (d1 < 0.0f)
		{
			d1 += 2.0f * (float)M_PI;
		}
		float d2 = facing - p.first;
		if (d2 < 0.0f)
		{
			d2 += 2.0f * (float)M_PI;
		}
		float diff = std::min(d1, d2);
		if (diff < closestDiff)
		{
			closestDiff = diff;
			closestAngle = p.first;
		}
	}
	return closestAngle;
}

namespace
{
static const float M_2xPI = 2.0f * M_PI;
}

VehicleType::Direction VehicleType::getDirectionLarge(float facing)
{
	static std::map<float, VehicleType::Direction> DirectionMap = {
	    {0.0f * (float)M_PI, VehicleType::Direction::N},
	    {0.125f * (float)M_PI, VehicleType::Direction::NNE},
	    {0.25f * (float)M_PI, VehicleType::Direction::NE},
	    {0.375f * (float)M_PI, VehicleType::Direction::NEE},
	    {0.5f * (float)M_PI, VehicleType::Direction::E},
	    {0.625f * (float)M_PI, VehicleType::Direction::SEE},
	    {0.75f * (float)M_PI, VehicleType::Direction::SE},
	    {0.875f * (float)M_PI, VehicleType::Direction::SSE},
	    {1.0f * (float)M_PI, VehicleType::Direction::S},
	    {1.125f * (float)M_PI, VehicleType::Direction::SSW},
	    {1.25f * (float)M_PI, VehicleType::Direction::SW},
	    {1.375f * (float)M_PI, VehicleType::Direction::SWW},
	    {1.5f * (float)M_PI, VehicleType::Direction::W},
	    {1.625f * (float)M_PI, VehicleType::Direction::NWW},
	    {1.75f * (float)M_PI, VehicleType::Direction::NW},
	    {1.875f * (float)M_PI, VehicleType::Direction::NNW},
	};

	float closestDiff = FLT_MAX;
	VehicleType::Direction closestDir = VehicleType::Direction::N;
	for (auto &p : DirectionMap)
	{
		float d1 = p.first - facing;
		if (d1 < 0.0f)
		{
			d1 += M_2xPI;
		}
		float d2 = facing - p.first;
		if (d2 < 0.0f)
		{
			d2 += M_2xPI;
		}
		float diff = std::min(d1, d2);
		if (diff < closestDiff)
		{
			closestDiff = diff;
			closestDir = p.second;
		}
	}
	return closestDir;
}

VehicleType::Direction VehicleType::getDirectionSmall(float facing)
{
	static std::map<float, VehicleType::Direction> DirectionMap = {
	    {0.0f * (float)M_PI, VehicleType::Direction::N},
	    {0.25f * (float)M_PI, VehicleType::Direction::NE},
	    {0.5f * (float)M_PI, VehicleType::Direction::E},
	    {0.75f * (float)M_PI, VehicleType::Direction::SE},
	    {1.0f * (float)M_PI, VehicleType::Direction::S},
	    {1.25f * (float)M_PI, VehicleType::Direction::SW},
	    {1.5f * (float)M_PI, VehicleType::Direction::W},
	    {1.75f * (float)M_PI, VehicleType::Direction::NW},
	};

	float closestDiff = FLT_MAX;
	VehicleType::Direction closestDir = VehicleType::Direction::N;
	for (auto &p : DirectionMap)
	{
		float d1 = p.first - facing;
		if (d1 < 0.0f)
		{
			d1 += M_2xPI;
		}
		float d2 = facing - p.first;
		if (d2 < 0.0f)
		{
			d2 += M_2xPI;
		}
		float diff = std::min(d1, d2);
		if (diff < closestDiff)
		{
			closestDiff = diff;
			closestDir = p.second;
		}
	}
	return closestDir;
}

}; // namespace OpenApoc
