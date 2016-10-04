#pragma once

#include "game/state/city/doodad.h"
#include "game/state/tileview/tileobject.h"

namespace OpenApoc
{

class TileObjectDoodad : public TileObject
{
  public:
	void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition, TileViewMode mode,
	          int, bool, bool) override;
	~TileObjectDoodad() override;

	std::weak_ptr<Doodad> doodad;

	Vec3<float> getPosition() const override;
	float getZOrder() const override;

  private:
	friend class TileMap;
	TileObjectDoodad(TileMap &map, sp<Doodad> doodad);
};

} // namespace OpenApoc
