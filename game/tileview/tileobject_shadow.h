#pragma once

#include "game/tileview/tileobject.h"

namespace OpenApoc
{

class Vehicle;

class TileObjectShadow : public TileObject
{
  public:
	virtual void draw(Renderer &r, TileView &view, Vec2<float> screenPosition, TileViewMode mode);
	virtual ~TileObjectShadow();
	virtual void setPosition(Vec3<float> newPosition) override;

  private:
	friend class TileMap;
	std::weak_ptr<Vehicle> owner;
	TileObjectShadow(TileMap &map, sp<Vehicle> owner);
};

} // namespace OpenApoc
