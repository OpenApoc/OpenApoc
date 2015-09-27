#include "game/tileview/tileview.h"
#include "game/tileview/tile.h"

#include "framework/includes.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "game/resources/gamecore.h"
#include "game/city/vehicle.h"

namespace OpenApoc
{

TileView::TileView(Framework &fw, TileMap &map, Vec3<int> isoTileSize, Vec2<int> stratTileSize,
                   TileViewMode initialMode)
    : Stage(fw), map(map), isoTileSize(isoTileSize), stratTileSize(stratTileSize),
      viewMode(initialMode), maxZDraw(10), offsetX(0), offsetY(0), cameraScrollX(0),
      cameraScrollY(0), selectedTilePosition(0, 0, 0),
      selectedTileImageBack(fw.data->load_image("CITY/SELECTED-CITYTILE-BACK.PNG")),
      selectedTileImageFront(fw.data->load_image("CITY/SELECTED-CITYTILE-FRONT.PNG")),
      pal(fw.data->load_palette("xcom3/ufodata/PAL_01.DAT"))
{
}

TileView::~TileView() {}

void TileView::Begin() {}

void TileView::Pause() {}

void TileView::Resume() {}

void TileView::Finish() {}

void TileView::EventOccurred(Event *e)
{
	bool selectionChanged = false;

	if (e->Type == EVENT_KEY_DOWN)
	{
		switch (e->Data.Keyboard.KeyCode)
		{
			case ALLEGRO_KEY_UP:
				// offsetY += tileSize.y;
				cameraScrollY = isoTileSize.y / 8;
				break;
			case ALLEGRO_KEY_DOWN:
				// offsetY -= tileSize.y;
				cameraScrollY = -isoTileSize.y / 8;
				break;
			case ALLEGRO_KEY_LEFT:
				// offsetX += tileSize.x;
				cameraScrollX = isoTileSize.x / 8;
				break;
			case ALLEGRO_KEY_RIGHT:
				// offsetX -= tileSize.x;
				cameraScrollX = -isoTileSize.x / 8;
				break;

			case ALLEGRO_KEY_PGDN:
				if (fw.gamecore->DebugModeEnabled && maxZDraw > 1)
				{
					maxZDraw--;
				}
				break;
			case ALLEGRO_KEY_PGUP:
				if (fw.gamecore->DebugModeEnabled && maxZDraw < map.size.z)
				{
					maxZDraw++;
				}
				break;
			case ALLEGRO_KEY_S:
				selectionChanged = true;
				if (selectedTilePosition.y < (map.size.y - 1))
					selectedTilePosition.y++;
				break;
			case ALLEGRO_KEY_W:
				selectionChanged = true;
				if (selectedTilePosition.y > 0)
					selectedTilePosition.y--;
				break;
			case ALLEGRO_KEY_A:
				selectionChanged = true;
				if (selectedTilePosition.x > 0)
					selectedTilePosition.x--;
				break;
			case ALLEGRO_KEY_D:
				selectionChanged = true;
				if (selectedTilePosition.x < (map.size.x - 1))
					selectedTilePosition.x++;
				break;
			case ALLEGRO_KEY_R:
				selectionChanged = true;
				if (selectedTilePosition.z < (map.size.z - 1))
					selectedTilePosition.z++;
				break;
			case ALLEGRO_KEY_F:
				selectionChanged = true;
				if (selectedTilePosition.z > 0)
					selectedTilePosition.z--;
				break;
			case ALLEGRO_KEY_1:
				pal = fw.data->load_palette("xcom3/ufodata/PAL_01.DAT");
				break;
			case ALLEGRO_KEY_2:
				pal = fw.data->load_palette("xcom3/ufodata/PAL_02.DAT");
				break;
			case ALLEGRO_KEY_3:
				pal = fw.data->load_palette("xcom3/ufodata/PAL_03.DAT");
				break;
		}
	}
	else if (e->Type == EVENT_MOUSE_DOWN)
	{
		auto &ev = e->Data.Mouse;
		auto selectedPos =
		    Vec2<float>{static_cast<float>(ev.X) - offsetX, static_cast<float>(ev.Y) - offsetY};
		std::shared_ptr<TileObject> newSelectedTileObject;

		for (auto &obj : this->map.selectableObjects)
		{
			Rect<float> bounds;
			if (this->viewMode == TileViewMode::Strategy)
			{
				bounds = {0, 0, 8, 8};
			}
			else
			{
				bounds = obj->getSelectableBounds();
			}
			auto tileOffset = tileToScreenCoords(obj->getDrawPosition());
			bounds.p0 += tileOffset;
			bounds.p1 += tileOffset;
			if (bounds.within(selectedPos))
			{
				if (newSelectedTileObject)
				{
					if (newSelectedTileObject->getDrawPosition().z < obj->getDrawPosition().z)
						newSelectedTileObject = obj;
				}
				else
					newSelectedTileObject = obj;
			}
			if (newSelectedTileObject != this->selectedTileObject)
			{
				/* Either could be nullptr if there's nothing currently selected
				 * or if you've clicked somewhere with no selectable objects
				 * (IE deselected) */
				if (this->selectedTileObject)
					this->selectedTileObject->setSelected(false);
				if (newSelectedTileObject)
					newSelectedTileObject->setSelected(true);
				this->selectedTileObject = newSelectedTileObject;
			}
		}
	}
	else if (e->Type == EVENT_KEY_UP)
	{
		switch (e->Data.Keyboard.KeyCode)
		{
			case ALLEGRO_KEY_UP:
			case ALLEGRO_KEY_DOWN:
				cameraScrollY = 0;
				break;
			case ALLEGRO_KEY_LEFT:
			case ALLEGRO_KEY_RIGHT:
				cameraScrollX = 0;
				break;
		}
	}
	if (fw.gamecore->DebugModeEnabled && selectionChanged)
	{
		LogInfo("Selected tile {%d,%d,%d}", selectedTilePosition.x, selectedTilePosition.y,
		        selectedTilePosition.z);
	}
}

void TileView::update(unsigned int ticks)
{
	offsetX += cameraScrollX;
	offsetY += cameraScrollY;

	/* TODO: MAke non-'1' update ticks work (e.g. projectile paths & vehicle movement intersection)
	 */
	while (ticks--)
		this->map.update(1);
}

void TileView::Render()
{
	int dpyWidth = fw.Display_GetWidth();
	int dpyHeight = fw.Display_GetHeight();
	Renderer &r = *fw.renderer;
	r.clear();
	r.setPalette(this->pal);

	// offsetX/offsetY is the 'amount added to the tile coords' - so we want
	// the inverse to tell which tiles are at the screen bounds
	auto topLeft =
	    screenToTileCoords(Vec2<int>{-offsetX - isoTileSize.x, -offsetY - isoTileSize.y}, 0);
	auto topRight = screenToTileCoords(Vec2<int>{-offsetX + dpyWidth, -offsetY - isoTileSize.y}, 0);
	auto bottomLeft =
	    screenToTileCoords(Vec2<int>{-offsetX - isoTileSize.x, -offsetY + dpyHeight}, map.size.z);
	auto bottomRight =
	    screenToTileCoords(Vec2<int>{-offsetX + dpyWidth, -offsetY + dpyHeight}, map.size.z);

	int minX = std::max(0, topLeft.x);
	int maxX = std::min(map.size.x, bottomRight.x);

	int minY = std::max(0, topRight.y);
	int maxY = std::min(map.size.y, bottomLeft.y);

	for (int z = 0; z < maxZDraw; z++)
	{
		for (int y = minY; y < maxY; y++)
		{
			for (int x = minX; x < maxX; x++)
			{
				bool showOrigin = fw.state->showTileOrigin;
				bool showSelected = (fw.gamecore->DebugModeEnabled && z == selectedTilePosition.z &&
				                     y == selectedTilePosition.y && x == selectedTilePosition.x);
				auto tile = map.getTile(x, y, z);
				auto screenPos = tileToScreenCoords(Vec3<float>{
				    static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)});
				screenPos.x += offsetX;
				screenPos.y += offsetY;

				if (showSelected)
					r.draw(selectedTileImageBack, screenPos);
				for (auto obj : tile->visibleObjects)
				{
					assert(obj->isVisible());
					bool showBounds = obj == this->selectedTileObject ||
					                  (fw.state->showSelectableBounds && obj->isSelectable());
					std::shared_ptr<Image> img;
					switch (this->viewMode)
					{
						case TileViewMode::Strategy:
							img = obj->getStrategySprite();
							break;
						case TileViewMode::Isometric:
							img = obj->getSprite();
							break;
						default:
							LogError("Invalid view mode");
					}
					if (!img)
					{
						LogWarning("Visible object has no sprite");
						continue;
					}
					auto pos = obj->getDrawPosition();
					auto objScreenPos = tileToScreenCoords(pos);
					objScreenPos.x += offsetX;
					objScreenPos.y += offsetY;
					r.draw(img, objScreenPos);
					if (x != obj->getOwningTile()->position.x ||
					    y != obj->getOwningTile()->position.y ||
					    z != obj->getOwningTile()->position.z)
					{
						LogError("Object has mismatches owning tile / visible object link"
						         " visible in {%d,%d,%d} owned by {%d,%d,%d}",
						         x, y, z, obj->getOwningTile()->position.x,
						         obj->getOwningTile()->position.y,
						         obj->getOwningTile()->position.z);
					}
					if (showOrigin)
					{
						Vec2<float> offset{offsetX, offsetY};
						auto linePos0 =
						    tileToScreenCoords(obj->getPosition() + Vec3<float>{0, 0, 0.5});
						auto linePos1 =
						    tileToScreenCoords(obj->getPosition() + Vec3<float>{0, 0, -0.5});
						linePos1 += offset;
						linePos0 += offset;
						r.drawLine(linePos0, linePos1, Colour{255, 0, 0, 255});
						linePos0 = tileToScreenCoords(obj->getPosition() + Vec3<float>{0, 0.5, 0});
						linePos1 = tileToScreenCoords(obj->getPosition() + Vec3<float>{0, -0.5, 0});
						linePos1 += offset;
						linePos0 += offset;
						r.drawLine(linePos0, linePos1, Colour{255, 0, 0, 255});
						linePos0 = tileToScreenCoords(obj->getPosition() + Vec3<float>{0.5, 0, 0});
						linePos1 = tileToScreenCoords(obj->getPosition() + Vec3<float>{-0.5, 0, 0});
						linePos1 += offset;
						linePos0 += offset;
						r.drawLine(linePos0, linePos1, Colour{255, 0, 0, 255});

						linePos0 = tileToScreenCoords(Vec3<float>{obj->getOwningTile()->position} +
						                              Vec3<float>{0, 0, 0.5});
						linePos1 = tileToScreenCoords(Vec3<float>{obj->getOwningTile()->position} +
						                              Vec3<float>{0, 0, -0.5});
						linePos1 += offset;
						linePos0 += offset;
						r.drawLine(linePos0, linePos1, Colour{255, 255, 0, 255});
						linePos0 = tileToScreenCoords(Vec3<float>{obj->getOwningTile()->position} +
						                              Vec3<float>{0, 0.5, 0});
						linePos1 = tileToScreenCoords(Vec3<float>{obj->getOwningTile()->position} +
						                              Vec3<float>{0, -0.5, 0});
						linePos1 += offset;
						linePos0 += offset;
						r.drawLine(linePos0, linePos1, Colour{255, 255, 0, 255});
						linePos0 = tileToScreenCoords(Vec3<float>{obj->getOwningTile()->position} +
						                              Vec3<float>{0.5, 0, 0});
						linePos1 = tileToScreenCoords(Vec3<float>{obj->getOwningTile()->position} +
						                              Vec3<float>{-0.5, 0, 0});
						linePos1 += offset;
						linePos0 += offset;
						r.drawLine(linePos0, linePos1, Colour{255, 255, 0, 255});
					}
					if (showBounds)
					{
						Vec2<float> offset{offsetX, offsetY};
						Rect<float> bounds;
						if (this->viewMode == TileViewMode::Strategy)
						{
							bounds = {0, 0, 8, 8};
						}
						else
						{
							bounds = obj->getSelectableBounds();
						}

						auto p00 = tileToScreenCoords(obj->getDrawPosition());
						p00 += offset;
						p00 += bounds.p0;
						auto p11 = tileToScreenCoords(obj->getDrawPosition());
						p11 += offset;
						p11 += bounds.p1;
						auto p10 = Vec2<float>{p11.x, p00.y};
						auto p01 = Vec2<float>{p00.x, p11.y};

						r.drawLine(p00, p00 + Vec2<float>{0, 5}, Colour{255, 0, 0, 255});
						r.drawLine(p00, p00 + Vec2<float>{5, 0}, Colour{255, 0, 0, 255});

						r.drawLine(p01, p01 + Vec2<float>{0, -5}, Colour{255, 0, 0, 255});
						r.drawLine(p01, p01 + Vec2<float>{5, 0}, Colour{255, 0, 0, 255});

						r.drawLine(p10, p10 + Vec2<float>{0, 5}, Colour{255, 0, 0, 255});
						r.drawLine(p10, p10 + Vec2<float>{-5, 0}, Colour{255, 0, 0, 255});

						r.drawLine(p11, p11 + Vec2<float>{0, -5}, Colour{255, 0, 0, 255});
						r.drawLine(p11, p11 + Vec2<float>{-5, 0}, Colour{255, 0, 0, 255});
					}
				}
				for (auto &p : tile->ownedProjectiles)
					p->drawProjectile(*this, r, Vec2<int>{offsetX, offsetY});

				if (showSelected)
					r.draw(selectedTileImageFront, screenPos);
			}
		}
	}
}

bool TileView::IsTransition() { return false; }

void TileView::setViewMode(TileViewMode newMode) { this->viewMode = newMode; }

TileViewMode TileView::getViewMode() const { return this->viewMode; }

}; // namespace OpenApoc
