#pragma once

#include "game/state/tileview/tileobject.h"

namespace OpenApoc
{

class Vehicle;

class TileObjectShadow : public TileObject
{
  public:
	void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
	          TileViewMode mode) override;
	~TileObjectShadow() override;
	void setPosition(Vec3<float> newPosition) override;
	Vec3<float> getPosition() const override;

  private:
	friend class TileMap;
	std::weak_ptr<Vehicle> owner;
	TileObjectShadow(TileMap &map, sp<Vehicle> owner);
	Vec3<float> shadowPosition;
	bool fellOffTheBottomOfTheMap;
};

} // namespace OpenApoc
