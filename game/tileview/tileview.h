#pragma once

#include "framework/stage.h"
#include "framework/includes.h"
#include "framework/palette.h"

namespace OpenApoc {

class TileMap;
class Image;

class TileView : public Stage
{
	protected:
		StageCmd stageCmd;
		TileMap &map;
		Vec3<int> tileSize;

	public:
		int maxZDraw;
		int offsetX, offsetY;
		int cameraScrollX, cameraScrollY;

		Vec3<int> selectedTilePosition;
		std::shared_ptr<Image> selectedTileImageBack, selectedTileImageFront;
		std::shared_ptr<Palette> pal;

		TileView(Framework &fw, TileMap &map, Vec3<int> tileSize);
		~TileView();

		template <typename T>
		Vec2<T> tileToScreenCoords(Vec3<T> c)
		{
			T x = (c.x * tileSize.x / 2) - (c.y * tileSize.x / 2);
			T y = (c.x * tileSize.y / 2) + (c.y * tileSize.y / 2)
				- (c.z * tileSize.z);
			return Vec2<T>{x,y};
		};
		template <typename T>
		Vec3<T> screenToTileCoords(Vec2<T> screenPos, T z)
		{
			screenPos.y += (z * tileSize.z);
			T y = ((screenPos.y / (tileSize.y / 2)) - (screenPos.x / (tileSize.x / 2))) / 2;
			T x = ((screenPos.y / (tileSize.y / 2)) + (screenPos.x / (tileSize.x / 2))) / 2;
			return Vec3<T>{x,y,z};
		};
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
