#pragma once

#include "framework/stage.h"
#include "framework/includes.h"
#include "framework/palette.h"

namespace OpenApoc {

class TileMap;
class Image;

class TileView : public Stage
{
	private:
		StageCmd stageCmd;
		TileMap &map;
		Vec3<float> tileSize;

	public:
		int maxZDraw;
		int offsetX, offsetY;
		int cameraScrollX, cameraScrollY;

		Vec3<int> selectedTilePosition;
		std::shared_ptr<Image> selectedTileImageBack, selectedTileImageFront;
		std::shared_ptr<Palette> pal;

		TileView(Framework &fw, TileMap &map, Vec3<float> tileSize);
		~TileView();

		Vec2<float> tileToScreenCoords(Vec3<float> c);
		Vec3<float> screenToTileCoords(Vec2<float> screenPos, float z);
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
