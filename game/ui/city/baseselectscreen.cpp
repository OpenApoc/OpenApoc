#include "game/ui/city/baseselectscreen.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/tileview/collision.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject_scenery.h"
#include "game/ui/city/basebuyscreen.h"

namespace OpenApoc
{

static const Colour PLAYER_BASE_OWNED{188, 212, 88};
static const Colour PLAYER_BASE_AVAILABLE{160, 236, 252};

BaseSelectScreen::BaseSelectScreen(sp<GameState> state, Vec3<float> centerPos)
    : CityTileView(*state->current_city->map, Vec3<int>{CITY_TILE_X, CITY_TILE_Y, CITY_TILE_Z},
               Vec2<int>{CITY_STRAT_TILE_X, CITY_STRAT_TILE_Y}, TileViewMode::Strategy),
      menuform(ui().getForm("FORM_SELECT_BASE_SCREEN")), state(state), counter(0)
{
	this->centerPos = centerPos;
	this->menuform->findControl("BUTTON_OK")->addCallback(FormEventType::ButtonClick, [](Event *) {
		fw().stageQueueCommand({StageCmd::Command::POP});
	});
}

BaseSelectScreen::~BaseSelectScreen() = default;

void BaseSelectScreen::begin()
{
	menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
}

void BaseSelectScreen::pause() {}

void BaseSelectScreen::resume() {}

void BaseSelectScreen::finish() {}

void BaseSelectScreen::eventOccurred(Event *e)
{
	menuform->eventOccured(e);

	if (menuform->eventIsWithin(e))
	{
		return;
	}

	if (e->type() == EVENT_KEY_DOWN && e->keyboard().KeyCode == SDLK_ESCAPE)
	{
		fw().stageQueueCommand({StageCmd::Command::POP});
	}
	// Exclude mouse down events that are over the form
	else if (e->type() == EVENT_MOUSE_DOWN)
	{
		if (e->mouse().Button == 2)
		{
			Vec2<float> screenOffset = {this->getScreenOffset().x, this->getScreenOffset().y};
			auto clickTile = this->screenToTileCoords(
			    Vec2<float>{e->mouse().X, e->mouse().Y} - screenOffset, 0.0f);
			this->setScreenCenterTile({clickTile.x, clickTile.y});
		}
		else if (e->mouse().Button == 1)
		{
			// If a click has not been handled by a form it's in the map. See if we intersect with
			// anything
			Vec2<float> screenOffset = {this->getScreenOffset().x, this->getScreenOffset().y};
			auto clickTop = this->screenToTileCoords(
			    Vec2<float>{e->mouse().X, e->mouse().Y} - screenOffset, 9.99f);
			auto clickBottom = this->screenToTileCoords(
			    Vec2<float>{e->mouse().X, e->mouse().Y} - screenOffset, 0.0f);
			auto collision = state->current_city->map->findCollision(clickTop, clickBottom);
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
							fw().stageQueueCommand(
							    {StageCmd::Command::PUSH, mksp<BaseBuyScreen>(state, building)});
						}
					}
				}
			}
		}
	}
	else
	{
		CityTileView::eventOccurred(e);
	}
}

void BaseSelectScreen::update()
{
	menuform->update();
	counter = (counter + 1) % COUNTER_MAX;
}

void BaseSelectScreen::render()
{
	CityTileView::render();
	for (auto b : state->current_city->buildings)
	{
		auto building = b.second;
		if (building->base_layout)
		{
			Vec3<float> posA = {building->bounds.p0.x, building->bounds.p0.y, 0};
			Vec2<float> screenPosA = this->tileToOffsetScreenCoords(posA);
			Vec3<float> posB = {building->bounds.p1.x, building->bounds.p1.y, 0};
			Vec2<float> screenPosB = this->tileToOffsetScreenCoords(posB);

			// Apply offset to borders every half-second
			if (counter >= COUNTER_MAX / 2)
			{
				screenPosA -= Vec2<float>{2.0f, 2.0f};
				screenPosB += Vec2<float>{2.0f, 2.0f};
			}

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
	menuform->render();
}

bool BaseSelectScreen::isTransition() { return false; }

}; // namespace OpenApoc
