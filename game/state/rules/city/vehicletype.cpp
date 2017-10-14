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

sp<VehicleType> VehicleType::get(const GameState &state, const UString &id)
{
	auto it = state.vehicle_types.find(id);
	if (it == state.vehicle_types.end())
	{
		LogError("No vehicle type matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

const UString &VehicleType::getPrefix()
{
	static UString prefix = "VEHICLETYPE_";
	return prefix;
}
const UString &VehicleType::getTypeName()
{
	static UString name = "VehicleType";
	return name;
}
const UString &VehicleType::getId(const GameState &state, const sp<VehicleType> ptr)
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

}; // namespace OpenApoc
