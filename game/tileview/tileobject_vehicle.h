#pragma once

#include "game/city/vehicle.h"
#include "game/tileview/tileobject.h"

namespace OpenApoc
{

class TileObjectVehicle : public TileObject
{
  public:
	void draw(Renderer &r, TileView &view, Vec2<float> screenPosition, TileViewMode mode) override;
	virtual ~TileObjectVehicle();

	sp<Vehicle> getVehicle();
	const Vec3<float> &getDirection() { return this->getVehicle()->velocity; }
	void setDirection(const Vec3<float> &dir) { this->getVehicle()->facing = dir; this->getVehicle()->velocity = dir; }

	sp<VoxelMap> getVoxelMap() override;

  private:
	friend class TileMap;
	std::weak_ptr<Vehicle> vehicle;
	TileObjectVehicle(TileMap &map, sp<Vehicle> vehicle);
};

} // namespace OpenApoc
