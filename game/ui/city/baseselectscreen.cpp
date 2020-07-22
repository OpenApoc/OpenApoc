#include "game/ui/city/baseselectscreen.h"
#include "forms/form.h"
#include "forms/label.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "framework/renderer.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/scenery.h"
#include "game/state/gamestate.h"
#include "game/state/rules/city/baselayout.h"
#include "game/state/tilemap/collision.h"
#include "game/state/tilemap/tilemap.h"
#include "game/state/tilemap/tileobject_scenery.h"
#include "game/ui/city/basebuyscreen.h"

namespace OpenApoc
{

BaseSelectScreen::BaseSelectScreen(sp<GameState> state, Vec3<float> centerPos)
    : CityTileView(*state->current_city->map, Vec3<int>{TILE_X_CITY, TILE_Y_CITY, TILE_Z_CITY},
                   Vec2<int>{STRAT_TILE_X, STRAT_TILE_Y}, TileViewMode::Strategy,
                   state->current_city->cityViewScreenCenter, *state),
      menuform(ui().getForm("city/baseselect")), state(state)
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

	if (e->type() == EVENT_KEY_DOWN &&
	    (e->keyboard().KeyCode == SDLK_ESCAPE || e->keyboard().KeyCode == SDLK_RETURN ||
	     e->keyboard().KeyCode == SDLK_KP_ENTER))
	{
		menuform->findControl("BUTTON_OK")->click();
	}
	// Exclude mouse down events that are over the form
	else if (e->type() == EVENT_MOUSE_DOWN)
	{
		if (Event::isPressed(e->mouse().Button, Event::MouseButton::Middle))
		{
			Vec2<float> screenOffset = {this->getScreenOffset().x, this->getScreenOffset().y};
			auto clickTile = this->screenToTileCoords(
			    Vec2<float>{e->mouse().X, e->mouse().Y} - screenOffset, 0.0f);
			this->setScreenCenterTile(Vec2<float>{clickTile.x, clickTile.y});
		}
		else if (Event::isPressed(e->mouse().Button, Event::MouseButton::Left))
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
						if (building->base_layout && building->owner == state->getGovernment())
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
	CityTileView::update();
}

void BaseSelectScreen::render()
{
	CityTileView::render();

	// Draw bases
	static const Colour PLAYER_BASE_AVAILABLE{160, 236, 252};
	for (auto &b : state->current_city->buildings)
	{
		auto building = b.second;
		if (building->base_layout && building->owner != state->getPlayer())
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

			fw().renderer->drawRect(screenPosA, screenPosB - screenPosA, PLAYER_BASE_AVAILABLE,
			                        2.0f);
		}
	}
	menuform->render();

	// If there's a modal dialog, darken the screen
	if (fw().stageGetCurrent() != this->shared_from_this())
	{
		fw().renderer->drawFilledRect({0, 0}, fw().displayGetSize(), Colour{0, 0, 0, 128});
	}
}

bool BaseSelectScreen::isTransition() { return false; }

}; // namespace OpenApoc
