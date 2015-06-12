#include "game/city/city.h"
#include "game/city/cityview.h"
#include "framework/framework.h"

namespace OpenApoc {

CityView::CityView(Framework &fw)
	: TileView(fw, *fw.state.city, Vec3<int>{CITY_TILE_X, CITY_TILE_Y, CITY_TILE_Z})
{

}

CityView::~CityView()
{

}

void
CityView::Render()
{
	TileView::Render();
}

void
CityView::Update(StageCmd * const cmd)
{
	TileView::Update(cmd);
}

void
CityView::EventOccurred(Event *e)
{
	TileView::EventOccurred(e);
}

}; //namespace OpenApoc
