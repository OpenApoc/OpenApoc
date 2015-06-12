#pragma once

#include "game/tileview/tileview.h"

namespace OpenApoc {

class CityView : public TileView
{
public:
	CityView(Framework &fw);
	virtual ~CityView();
	virtual void Update(StageCmd * const cmd);
	virtual void Render();
	virtual void EventOccurred(Event *e);
};

}; //namespace OpenApoc
