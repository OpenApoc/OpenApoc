#pragma once
#include "library/sp.h"

#include "framework/stage.h"
#include "framework/includes.h"
#include "framework/palette.h"
#include "framework/logger.h"

namespace OpenApoc
{

class TileMap;
class Image;
class TileObject;

enum class TileViewMode
{
	Isometric,
	Strategy,
};

class TileView : public Stage
{
  protected:
	StageCmd stageCmd;
	TileMap &map;
	Vec3<int> isoTileSize;
	Vec2<int> stratTileSize;
	TileViewMode viewMode;

  public:
	int maxZDraw;
	int offsetX, offsetY;
	int cameraScrollX, cameraScrollY;

	Vec3<int> selectedTilePosition;
	sp<Image> selectedTileImageBack, selectedTileImageFront;
	sp<Palette> pal;

	sp<TileObject> selectedTileObject;

	TileView(Framework &fw, TileMap &map, Vec3<int> isoTileSize, Vec2<int> stratTileSize,
	         TileViewMode initialMode);
	~TileView();

	template <typename T> Vec2<T> tileToScreenCoords(Vec3<T> c)
	{
		switch (viewMode)
		{
			case TileViewMode::Isometric:
			{
				T x = (c.x * isoTileSize.x / 2) - (c.y * isoTileSize.x / 2);
				T y = (c.x * isoTileSize.y / 2) + (c.y * isoTileSize.y / 2) - (c.z * isoTileSize.z);
				return Vec2<T>{x, y};
			}
			case TileViewMode::Strategy:
			{
				return Vec2<T>{c.x * stratTileSize.x, c.y * stratTileSize.y};
			}
		}
		LogError("Invalid view mode");
		return {0, 0};
	}
	template <typename T> Vec3<T> screenToTileCoords(Vec2<T> screenPos, T z)
	{
		switch (viewMode)
		{
			case TileViewMode::Isometric:
			{
				screenPos.y += (z * isoTileSize.z);
				T y =
				    ((screenPos.y / (isoTileSize.y / 2)) - (screenPos.x / (isoTileSize.x / 2))) / 2;
				T x =
				    ((screenPos.y / (isoTileSize.y / 2)) + (screenPos.x / (isoTileSize.x / 2))) / 2;
				return Vec3<T>{x, y, z};
			}
			case TileViewMode::Strategy:
			{
				return Vec3<T>{screenPos.x / stratTileSize.x, screenPos.y / stratTileSize.y, z};
			}
		}
		LogError("Invalid view mode");
		return {0, 0, z};
	}
	// Stage control
	virtual void Begin() override;
	virtual void Pause() override;
	virtual void Resume() override;
	virtual void Finish() override;
	virtual void EventOccurred(Event *e) override;
	virtual void Render() override;
	virtual bool IsTransition() override;

	virtual void update(unsigned int ticks);

	virtual void setViewMode(TileViewMode newMode);
	virtual TileViewMode getViewMode() const;
};
}; // namespace OpenApoc
