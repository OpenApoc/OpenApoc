#pragma once

#include "framework/logger.h"
#include "framework/stage.h"
#include "game/state/tilemap/tilemap.h"
#include "library/colour.h"
#include "library/sp.h"
#include "library/vec.h"

#define STRAT_TILE_X 8
#define STRAT_TILE_Y 8

namespace OpenApoc
{

class TileMap;
class Image;
class Palette;

class TileView : public Stage, public TileTransform
{
  protected:
	// Formula: FPS / DESIRED_ANIMATIONS_PER_SECOND

	static const int SELECTION_FRAME_ANIMATION_DELAY = 60 / 5;
	static const int PORTAL_FRAME_ANIMATION_DELAY = 60 / 15;
	// How many pixels from edge trigger scroll
	static const int MOUSE_SCROLL_MARGIN = 1;

  protected:
	TileMap &map;
	Vec3<int> isoTileSize;
	Vec2<int> stratTileSize;
	TileViewMode viewMode;

	bool scrollUpKB = false;
	bool scrollDownKB = false;
	bool scrollLeftKB = false;
	bool scrollRightKB = false;

	bool scrollUpM = false;
	bool scrollDownM = false;
	bool scrollLeftM = false;
	bool scrollRightM = false;

	bool autoScroll = false;

	Vec2<int> dpySize;

	Colour strategyViewBoxColour;
	float strategyViewBoxThickness;

	Vec3<int> selectedTilePosition;

	bool debugHotkeyMode = false;

  public:
	int maxZDraw;
	Vec3<float> centerPos;
	Vec2<float> isoScrollSpeed;
	Vec2<float> stratScrollSpeed;

	sp<Palette> pal;

	TileView(TileMap &map, Vec3<int> isoTileSize, Vec2<int> stratTileSize,
	         TileViewMode initialMode);
	~TileView() override;

	Vec2<int> getScreenOffset() const;
	virtual void setScreenCenterTile(Vec2<float> center);
	virtual void setScreenCenterTile(Vec3<float> center);
	virtual void setScreenCenterTile(Vec2<int> center)
	{
		this->setScreenCenterTile(Vec2<float>{center.x, center.y});
	}
	virtual void setScreenCenterTile(Vec3<int> center)
	{

		this->setScreenCenterTile(Vec3<float>{center.x, center.y, center.z});
	}

	Vec3<int> getSelectedTilePosition();
	virtual void setSelectedTilePosition(Vec3<int> newPosition);

	template <typename T> Vec2<T> tileToScreenCoords(Vec3<T> c, TileViewMode v) const
	{
		switch (v)
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
	template <typename T> Vec2<T> tileToScreenCoords(Vec3<T> c) const
	{
		return this->tileToScreenCoords(c, this->viewMode);
	}

	Vec2<float> tileToScreenCoords(Vec3<float> coords) const override
	{
		return this->tileToScreenCoords(coords, this->viewMode);
	}

	template <typename T> Vec2<T> tileToOffsetScreenCoords(Vec3<T> c, TileViewMode v) const
	{
		return this->tileToScreenCoords(c, v) + Vec2<T>{this->getScreenOffset()};
	}
	template <typename T> Vec2<T> tileToOffsetScreenCoords(Vec3<T> c) const
	{
		return this->tileToScreenCoords(c, this->viewMode) + Vec2<T>{this->getScreenOffset()};
	}

	template <typename T> Vec3<T> screenToTileCoords(Vec2<T> screenPos, T z, TileViewMode v) const
	{
		switch (v)
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
	template <typename T> Vec3<T> screenToTileCoords(Vec2<T> screenPos, T z) const
	{
		return this->screenToTileCoords(screenPos, z, this->viewMode);
	}

	Vec3<float> screenToTileCoords(Vec2<float> screenPos, float z) const override
	{
		return this->screenToTileCoords(screenPos, z, this->viewMode);
	}

	template <typename T>
	Vec3<T> offsetScreenToTileCoords(Vec2<T> screenPos, T z, TileViewMode v) const
	{
		return this->screenToTileCoords(screenPos - Vec2<T>{this->getScreenOffset()}, z, v);
	}
	template <typename T> Vec3<T> offsetScreenToTileCoords(Vec2<T> screenPos, T z) const
	{
		return this->screenToTileCoords(screenPos - Vec2<T>{this->getScreenOffset()}, z,
		                                this->viewMode);
	}

	// Stage control
	void begin() override;
	void pause() override;
	void resume() override;
	void finish() override;
	void update() override;
	void eventOccurred(Event *e) override;
	bool isTransition() override;

	virtual void setViewMode(TileViewMode newMode);
	virtual TileViewMode getViewMode() const;

	void applyScrolling();
	void renderStrategyOverlay(Renderer &r);
};
}; // namespace OpenApoc
