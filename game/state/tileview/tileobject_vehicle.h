#pragma once

#include "game/state/tileview/tileobject.h"
#include "library/sp.h"
#include "library/vec.h"
#include <list>

namespace OpenApoc
{

class Vehicle;
class Image;

class TileObjectVehicle : public TileObject
{
  public:
	void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition, TileViewMode mode,
	          bool visible, int currentLevel, bool friendly, bool hostile) override;
	~TileObjectVehicle() override;

	sp<Vehicle> getVehicle() const;
	const Vec3<float> &getDirection() const;
	void setDirection(const Vec3<float> &dir);

	Vec3<float> getVoxelCentrePosition() const override;
	bool hasVoxelMap() override { return true; }
	sp<VoxelMap> getVoxelMap(Vec3<int> mapIndex, bool los) const override;
	Vec3<float> getPosition() const override;
	void setPosition(Vec3<float> newPosition) override;
	void nextFrame(int ticks);
	void addToDrawnTiles(Tile *tile) override;

  private:
	friend class TileMap;
	wp<Vehicle> vehicle;
	std::list<sp<Image>>::iterator animationFrame;
	int animationDelay;

	TileObjectVehicle(TileMap &map, sp<Vehicle> vehicle);
};

} // namespace OpenApoc
