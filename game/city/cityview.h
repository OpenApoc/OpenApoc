#pragma once

#include "framework/stage.h"
#include "framework/includes.h"

#include "game/resources/gamecore.h"
#include "game/apocresources/apocresource.h"
#include "forms/forms.h"


#define CITY_TILE_HEIGHT (32)
#define CITY_TILE_WIDTH (64)
#define CITY_TILE_ZOFFSET (-16)

namespace OpenApoc {

class CityView : public Stage
{
	private:
		StageCmd stageCmd;
	public:
		int maxZDraw;

		int offsetX, offsetY;
		std::unique_ptr<Palette> pal;
		std::unique_ptr<PCK> cityPck;

		Vec3<int> selectedTilePosition;
		std::shared_ptr<Image> selectedTileImageBack, selectedTileImageFront;

		CityView();
		~CityView();
		// Stage control
		virtual void Begin();
		virtual void Pause();
		virtual void Resume();
		virtual void Finish();
		virtual void EventOccurred(Event *e);
		virtual void Update(StageCmd * const cmd);
		virtual void Render();
		virtual bool IsTransition();
};
}; //namespace OpenApoc
