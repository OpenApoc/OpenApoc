#pragma once
#include "library/sp.h"
#include "game/tileview/projectile.h"

namespace OpenApoc
{

class BeamProjectile : virtual public Projectile
{
  protected:
	Colour colour;
	float beamLength;
	float beamWidth;

  public:
	BeamProjectile(TileMap &map, sp<Vehicle> firer, Vec3<float> position, Vec3<float> velocity,
	               unsigned int lifetime, const Colour &colour, float beamLength, float beamWidth);

	virtual void drawProjectile(TileView &v, Renderer &r, Vec2<int> screenPosition) override;
};

}; // namespace OpenApoc
