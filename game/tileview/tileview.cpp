#include "game/tileview/tileview.h"
#include "game/tileview/tile.h"

#include "framework/includes.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "game/resources/gamecore.h"


namespace OpenApoc {

TileView::TileView(Framework &fw, TileMap &map, Vec3<int> tileSize)
	: Stage(fw), map(map), tileSize(tileSize), maxZDraw(10), offsetX(0), offsetY(0),
	  cameraScrollX(0), cameraScrollY(0), selectedTilePosition(0,0,0),
	  selectedTileImageBack(fw.data->load_image("CITY/SELECTED-CITYTILE-BACK.PNG")),
	  selectedTileImageFront(fw.data->load_image("CITY/SELECTED-CITYTILE-FRONT.PNG")),
	  pal(fw.data->load_palette("xcom3/ufodata/PAL_01.DAT"))
{
}

TileView::~TileView()
{
}

void TileView::Begin()
{
}

void TileView::Pause()
{
}

void TileView::Resume()
{
}

void TileView::Finish()
{
}


void TileView::EventOccurred(Event *e)
{
	bool selectionChanged = false;

	if( e->Type == EVENT_KEY_DOWN )
	{
		if( e->Data.Keyboard.KeyCode == ALLEGRO_KEY_ESCAPE )
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}

		switch (e->Data.Keyboard.KeyCode)
		{
			case ALLEGRO_KEY_UP:
				//offsetY += tileSize.y;
				cameraScrollY = tileSize.y / 8;
				break;
			case ALLEGRO_KEY_DOWN:
				//offsetY -= tileSize.y;
				cameraScrollY = -tileSize.y / 8;
				break;
			case ALLEGRO_KEY_LEFT:
				//offsetX += tileSize.x;
				cameraScrollX = tileSize.x / 8;
				break;
			case ALLEGRO_KEY_RIGHT:
				//offsetX -= tileSize.x;
				cameraScrollX = -tileSize.x / 8;
				break;

			case ALLEGRO_KEY_PGDN:
				if( fw.gamecore->DebugModeEnabled && maxZDraw > 1)
				{
					maxZDraw--;
				}
				break;
			case ALLEGRO_KEY_PGUP:
				if( fw.gamecore->DebugModeEnabled && maxZDraw < map.size.z)
				{
					maxZDraw++;
				}
				break;
			case ALLEGRO_KEY_S:
				selectionChanged = true;
				if (selectedTilePosition.y < (map.size.y-1))
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
				if (selectedTilePosition.x < (map.size.x-1))
					selectedTilePosition.x++;
				break;
			case ALLEGRO_KEY_R:
				selectionChanged = true;
				if (selectedTilePosition.z < (map.size.z-1))
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
		auto selected = screenToTileCoords(Vec2<float>{(float)ev.X - offsetX, (float)ev.Y - offsetY}, (float)maxZDraw-1);

		if (selected.x < 0) selected.x = 0;
		if (selected.y < 0) selected.y = 0;
		if (selected.x > 99) selected.x = 99;
		if (selected.y > 99) selected.y = 99;
		selectedTilePosition = Vec3<int>{(int)selected.x, (int)selected.y, (int)selected.z};
		selectionChanged = true;
	} else if( e->Type == EVENT_KEY_UP )
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
	if (fw.gamecore->DebugModeEnabled &&
	    selectionChanged)
	{
		LogInfo("Selected tile {%d,%d,%d}", selectedTilePosition.x, selectedTilePosition.y, selectedTilePosition.z);
	}
}

void TileView::Update(StageCmd * const cmd)
{
	*cmd = stageCmd;
	stageCmd = StageCmd();

	offsetX += cameraScrollX;
	offsetY += cameraScrollY;

	//TODO: Map wall-time to ticks?
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
	auto topLeft = screenToTileCoords(Vec2<int>{-offsetX - tileSize.x, -offsetY - tileSize.y}, 0);
	auto topRight = screenToTileCoords(Vec2<int>{-offsetX + dpyWidth, -offsetY - tileSize.y}, 0);
	auto bottomLeft = screenToTileCoords(Vec2<int>{-offsetX - tileSize.x, -offsetY + dpyHeight}, map.size.z);
	auto bottomRight = screenToTileCoords(Vec2<int>{-offsetX + dpyWidth, -offsetY + dpyHeight}, map.size.z);

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
				bool showSelected =
					(fw.gamecore->DebugModeEnabled &&
					 z == selectedTilePosition.z &&
					 y == selectedTilePosition.y &&
					 x == selectedTilePosition.x);
				auto tile = map.getTile(x, y, z);
				auto screenPos = tileToScreenCoords(Vec3<float>{(float)x,(float)y,(float)z});
				screenPos.x += offsetX;
				screenPos.y += offsetY;

				if (showSelected)
					r.draw(selectedTileImageBack, screenPos);
				for (auto obj : tile->visibleObjects)
				{
					assert(obj->isVisible());
					auto img = obj->getSprite();
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
							x, y, z, obj->getOwningTile()->position.x, obj->getOwningTile()->position.y, obj->getOwningTile()->position.z);
					}
					if (showOrigin)
					{
						Vec2<float> offset{offsetX, offsetY};
						auto linePos0 = tileToScreenCoords(obj->getPosition() + Vec3<float>{0,0,0.5});
						auto linePos1 = tileToScreenCoords(obj->getPosition() + Vec3<float>{0,0,-0.5});
						linePos1 += offset;
						linePos0 += offset;
						r.drawLine(linePos0, linePos1, Colour{255,0,0,255});
						linePos0 = tileToScreenCoords(obj->getPosition() + Vec3<float>{0,0.5,0});
						linePos1 = tileToScreenCoords(obj->getPosition() + Vec3<float>{0,-0.5,0});
						linePos1 += offset;
						linePos0 += offset;
						r.drawLine(linePos0, linePos1, Colour{255,0,0,255});
						linePos0 = tileToScreenCoords(obj->getPosition() + Vec3<float>{0.5,0,0});
						linePos1 = tileToScreenCoords(obj->getPosition() + Vec3<float>{-0.5,0,0});
						linePos1 += offset;
						linePos0 += offset;
						r.drawLine(linePos0, linePos1, Colour{255,0,0,255});

						linePos0 = tileToScreenCoords(Vec3<float>{obj->getOwningTile()->position} + Vec3<float>{0,0,0.5});
						linePos1 = tileToScreenCoords(Vec3<float>{obj->getOwningTile()->position} + Vec3<float>{0,0,-0.5});
						linePos1 += offset;
						linePos0 += offset;
						r.drawLine(linePos0, linePos1, Colour{255,255,0,255});
						linePos0 = tileToScreenCoords(Vec3<float>{obj->getOwningTile()->position} + Vec3<float>{0,0.5,0});
						linePos1 = tileToScreenCoords(Vec3<float>{obj->getOwningTile()->position} + Vec3<float>{0,-0.5,0});
						linePos1 += offset;
						linePos0 += offset;
						r.drawLine(linePos0, linePos1, Colour{255,255,0,255});
						linePos0 = tileToScreenCoords(Vec3<float>{obj->getOwningTile()->position} + Vec3<float>{0.5,0,0});
						linePos1 = tileToScreenCoords(Vec3<float>{obj->getOwningTile()->position} + Vec3<float>{-0.5,0,0});
						linePos1 += offset;
						linePos0 += offset;
						r.drawLine(linePos0, linePos1, Colour{255,255,0,255});
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

bool TileView::IsTransition()
{
	return false;
}

}; //namespace OpenApoc
