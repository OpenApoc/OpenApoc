#pragma once

#include "library/sp.h"
#include "library/vec.h"

namespace OpenApoc
{

class Base;
class Building;
class Facility;
class GameState;
class RGBImage;

namespace BaseGraphics
{
const int TILE_SIZE = 32;
const int MINI_SIZE = 4;

enum class FacilityHighlight
{
	None,
	Construction,
	Quarters,
	Stores,
	Labs,
	Aliens
};

int getCorridorSprite(const Base &base, Vec2<int> pos);
void renderBase(Vec2<int> renderPos, const Base &base);
sp<RGBImage> drawMiniBase(const Base &base, FacilityHighlight highlight = FacilityHighlight::None,
                          sp<Facility> selected = nullptr);
sp<RGBImage> drawMinimap(sp<GameState> state, const Building &selected);
} // namespace BaseGraphics

}; // namespace OpenApoc
