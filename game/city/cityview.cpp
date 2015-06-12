#include "game/city/city.h"
#include "game/city/cityview.h"
#include "framework/framework.h"
#include "game/resources/gamecore.h"

namespace OpenApoc {

CityView::CityView(Framework &fw)
	: TileView(fw, *fw.state.city, Vec3<int>{CITY_TILE_X, CITY_TILE_Y, CITY_TILE_Z}),
		ui(fw.gamecore->GetForm("FORM_CITY_UI"))
{

}

CityView::~CityView()
{

}

void
CityView::Render()
{
	TileView::Render();
	ui->Render();
	fw.gamecore->MouseCursor->Render();
}

void
CityView::Update(StageCmd * const cmd)
{
	TileView::Update(cmd);
	ui->Update();
}

void
CityView::EventOccurred(Event *e)
{
	fw.gamecore->MouseCursor->EventOccured( e );
	TileView::EventOccurred(e);
}

}; //namespace OpenApoc
