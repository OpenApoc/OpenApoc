#include "game/ui/city/baseselectscreen.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/tileview/tileobject_scenery.h"
#include "game/state/tileview/voxel.h"
#include "game/ui/city/basebuyscreen.h"

namespace OpenApoc
{

static const Colour PLAYER_BASE_OWNED{188, 212, 88};
static const Colour PLAYER_BASE_AVAILABLE{160, 236, 252};

BaseSelectScreen::BaseSelectScreen(sp<GameState> state, StateRef<City> city, Vec3<float> centerPos)
    : TileView(*city->map, Vec3<int>{CITY_TILE_X, CITY_TILE_Y, CITY_TILE_Z},
               Vec2<int>{CITY_STRAT_TILE_X, CITY_STRAT_TILE_Y}, TileViewMode::Strategy),
      menuform(ui().GetForm("FORM_SELECT_BASE_SCREEN")), state(state), city(city)
{
	this->centerPos = centerPos;
	this->menuform->FindControl("BUTTON_OK")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *e) { this->stageCmd.cmd = StageCmd::Command::POP; });
	Resume();
}

BaseSelectScreen::~BaseSelectScreen() {}

void BaseSelectScreen::Begin() {}

void BaseSelectScreen::Pause() {}

void BaseSelectScreen::Resume()
{
	menuform->FindControlTyped<Label>("TEXT_FUNDS")->SetText(state->getPlayerBalance());
}

void BaseSelectScreen::Finish() {}

void BaseSelectScreen::EventOccurred(Event *e)
{
	menuform->EventOccured(e);

	if (menuform->eventIsWithin(e))
	{
		return;
	}

	if (e->Type() == EVENT_KEY_DOWN && e->Keyboard().KeyCode == SDLK_ESCAPE)
	{
		stageCmd.cmd = StageCmd::Command::POP;
	}
	// Exclude mouse down events that are over the form
	else if (e->Type() == EVENT_MOUSE_DOWN)
	{
		if (e->Mouse().Button == 2)
		{
			Vec2<float> screenOffset = {this->getScreenOffset().x, this->getScreenOffset().y};
			auto clickTile = this->screenToTileCoords(
			    Vec2<float>{e->Mouse().X, e->Mouse().Y} - screenOffset, 0.0f);
			this->setScreenCenterTile({clickTile.x, clickTile.y});
		}
		else if (e->Mouse().Button == 1)
		{
			// If a click has not been handled by a form it's in the map. See if we intersect with
			// anything
			Vec2<float> screenOffset = {this->getScreenOffset().x, this->getScreenOffset().y};
			auto clickTop = this->screenToTileCoords(
			    Vec2<float>{e->Mouse().X, e->Mouse().Y} - screenOffset, 9.99f);
			auto clickBottom = this->screenToTileCoords(
			    Vec2<float>{e->Mouse().X, e->Mouse().Y} - screenOffset, 0.0f);
			auto collision = this->city->map->findCollision(clickTop, clickBottom);
			if (collision)
			{
				if (collision.obj->getType() == TileObject::Type::Scenery)
				{
					auto scenery =
					    std::dynamic_pointer_cast<TileObjectScenery>(collision.obj)->getOwner();
					auto building = scenery->building;
					if (building)
					{
						if (building->base_layout && building->owner.id == "ORG_GOVERNMENT")
						{
							stageCmd.cmd = StageCmd::Command::PUSH;
							stageCmd.nextStage = mksp<BaseBuyScreen>(state, building);
						}
					}
				}
			}
		}
	}
	else
	{
		TileView::EventOccurred(e);
	}
}

void BaseSelectScreen::Update(StageCmd *const cmd)
{
	menuform->Update();
	*cmd = this->stageCmd;
	// Reset the command to default
	this->stageCmd = StageCmd();
}

void BaseSelectScreen::Render()
{
	TileView::Render();
	for (auto b : state->current_city->buildings)
	{
		auto building = b.second;
		if (building->base_layout)
		{
			Vec3<float> posA = {building->bounds.p0.x, building->bounds.p0.y, 0};
			Vec2<float> screenPosA = this->tileToOffsetScreenCoords(posA);
			Vec3<float> posB = {building->bounds.p1.x, building->bounds.p1.y, 0};
			Vec2<float> screenPosB = this->tileToOffsetScreenCoords(posB);

			Colour borderColour;
			if (building->owner == state->getPlayer())
			{
				borderColour = PLAYER_BASE_OWNED;
			}
			else if (building->owner.id == "ORG_GOVERNMENT")
			{
				borderColour = PLAYER_BASE_AVAILABLE;
			}
			fw().renderer->drawRect(screenPosA, screenPosB - screenPosA, borderColour, 2.0f);
		}
	}
	menuform->Render();
}

bool BaseSelectScreen::IsTransition() { return false; }

}; // namespace OpenApoc
