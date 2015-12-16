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
	const Vec3<float> &getDirection() { return this->direction; }
	void setDirection(const Vec3<float> &dir) { this->direction = dir; }

  private:
	friend class TileMap;
	std::weak_ptr<Vehicle> vehicle;
	TileObjectVehicle(TileMap &map, sp<Vehicle> vehicle, Vec3<float> initialDirection = {1, 0, 0});
	Vec3<float> direction;
};

} // namespace OpenApoc
