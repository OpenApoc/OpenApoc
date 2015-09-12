#include "game/tileview/beam_projectile.h"
#include "framework/logger.h"
#include "game/tileview/tileview.h"

namespace OpenApoc
{

BeamProjectile::BeamProjectile(TileMap &map, std::shared_ptr<Vehicle> firer, Vec3<float> position,
                               Vec3<float> velocity, unsigned int lifetime, const Colour &colour,
                               float beamLength, float beamWidth)
    : TileObject(map, position, false, false, true),
      Projectile(map, firer, position, velocity, lifetime), colour(colour), beamLength(beamLength),
      beamWidth(beamWidth)
{
}

void BeamProjectile::drawProjectile(TileView &v, Renderer &r, Vec2<int> screenOffset)
{
	Vec2<float> headScreenCoords = v.tileToScreenCoords(this->getPosition());
	headScreenCoords += screenOffset;
	Vec3<float> tailPosition =
	    this->getPosition() + (beamLength * (glm::normalize(this->velocity)));
	Vec2<float> tailScreenCoords = v.tileToScreenCoords(tailPosition);
	tailScreenCoords += screenOffset;
	r.drawLine(headScreenCoords, tailScreenCoords, colour, beamWidth);
}

}; // namespace OpenApoc
