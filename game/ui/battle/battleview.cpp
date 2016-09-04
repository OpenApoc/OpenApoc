#include "game/ui/battle/battleview.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/battle.h"
#include "game/state/battlemappart.h"
#include "game/state/battlemappart_type.h"
#include "game/state/battletileobject_mappart.h"
#include "game/state/gameevent.h"
#include "game/state/tileview/voxel.h"
#include "game/ui/base/basescreen.h"
#include "game/ui/battletileview/battletileview.h"
#include "game/ui/general/ingameoptions.h"
#include "library/sp.h"

namespace OpenApoc
{
namespace
{
static const std::vector<UString> TAB_FORM_NAMES = {
    "FORM_BATTLE_UI_RT_1", "FORM_BATTLE_UI_RT_2", "FORM_BATTLE_UI_RT_3",
    "FORM_BATTLE_UI_RT_4", "FORM_BATTLE_UI_TB_1", "FORM_BATTLE_UI_TB_2",
    "FORM_BATTLE_UI_TB_3", "FORM_BATTLE_UI_TB_4", "FORM_BATTLE_UI_TB_5"};

} // anonymous namespace

BattleView::BattleView(sp<GameState> state)
    : BattleTileView(*state->battle.map, Vec3<int>{BATTLE_TILE_X, BATTLE_TILE_Y, BATTLE_TILE_Z},
                     Vec2<int>{BATTLE_STRAT_TILE_X, BATTLE_STRAT_TILE_Y}, TileViewMode::Isometric),
      baseForm(ui().getForm("FORM_BATTLE_UI")), updateSpeed(BattleUpdateSpeed::Speed1),
      state(state), followAgent(false),
      palette(fw().data->loadPalette("xcom3/tacdata/tactical.pal")),
      selectionState(BattleSelectionState::Normal)
{
	baseForm->findControlTyped<RadioButton>("BUTTON_SPEED0")->setChecked(true);
	for (auto &formName : TAB_FORM_NAMES)
	{
		sp<Form> f(ui().getForm(formName));
		if (!f)
		{
			LogError("Failed to load form \"%s\"", formName.cStr());
			return;
		}
		f->takesFocus = false;
		this->uiTabs.push_back(f);
	}
	this->activeTab = this->uiTabs[0];

	// Refresh base views
	resume();

	this->baseForm->findControl("BUTTON_FOLLOW_AGENT")
	    ->addCallback(FormEventType::CheckBoxChange, [this](Event *e) {
		    this->followAgent =
		        std::dynamic_pointer_cast<CheckBox>(e->forms().RaisedBy)->isChecked();
		});
	this->baseForm->findControl("BUTTON_TOGGLE_STRATMAP")
	    ->addCallback(FormEventType::CheckBoxChange, [this](Event *e) {
		    bool strategy = std::dynamic_pointer_cast<CheckBox>(e->forms().RaisedBy)->isChecked();
		    this->setViewMode(strategy ? TileViewMode::Strategy : TileViewMode::Isometric);
		});
	this->baseForm->findControl("BUTTON_LAYERING")
	    ->addCallback(FormEventType::TriStateBoxChange, [this](Event *e) {
		    int state = std::dynamic_pointer_cast<TriStateBox>(e->forms().RaisedBy)->getState();
		    switch (state)
		    {
			    case 1:
				    setLayerDrawingMode(BattleLayerDrawingMode::UpToCurrentLevel);
				    break;
			    case 2:
				    setLayerDrawingMode(BattleLayerDrawingMode::AllLevels);
				    break;
			    case 3:
				    setLayerDrawingMode(BattleLayerDrawingMode::OnlyCurrentLevel);
				    break;
		    }

		});
	this->baseForm->findControl("BUTTON_LAYER_UP")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *e) {
		    this->setZLevel(getZLevel() + 1);
		    updateLayerButtons();
		});
	this->baseForm->findControl("BUTTON_LAYER_DOWN")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *e) {
		    this->setZLevel(getZLevel() - 1);
		    updateLayerButtons();
		});
	this->baseForm->findControl("BUTTON_LAYER_1")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { this->setZLevel(1); });
	this->baseForm->findControl("BUTTON_LAYER_2")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { this->setZLevel(2); });
	this->baseForm->findControl("BUTTON_LAYER_3")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { this->setZLevel(3); });
	this->baseForm->findControl("BUTTON_LAYER_4")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { this->setZLevel(4); });
	this->baseForm->findControl("BUTTON_LAYER_5")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { this->setZLevel(5); });
	this->baseForm->findControl("BUTTON_LAYER_6")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { this->setZLevel(6); });
	this->baseForm->findControl("BUTTON_LAYER_7")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { this->setZLevel(7); });
	this->baseForm->findControl("BUTTON_LAYER_8")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { this->setZLevel(8); });
	this->baseForm->findControl("BUTTON_LAYER_9")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { this->setZLevel(9); });
	this->baseForm->findControl("BUTTON_SPEED0")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->updateSpeed = BattleUpdateSpeed::Pause; });
	this->baseForm->findControl("BUTTON_SPEED1")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->updateSpeed = BattleUpdateSpeed::Speed1; });
	this->baseForm->findControl("BUTTON_SPEED2")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->updateSpeed = BattleUpdateSpeed::Speed2; });
	this->baseForm->findControl("BUTTON_SPEED3")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->updateSpeed = BattleUpdateSpeed::Speed3; });
	this->baseForm->findControl("BUTTON_SHOW_OPTIONS")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<InGameOptions>(this->state)});
		});
	this->baseForm->findControl("BUTTON_SHOW_LOG")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) { LogWarning("Show log"); });
	this->baseForm->findControl("BUTTON_ZOOM_EVENT")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) { LogWarning("Zoom to event"); });
}

BattleView::~BattleView() = default;

void BattleView::begin()
{
	this->baseForm->findControl("BUTTON_LAYER_1")->Enabled = maxZDraw >= 1;
	this->baseForm->findControl("BUTTON_LAYER_2")->Enabled = maxZDraw >= 2;
	this->baseForm->findControl("BUTTON_LAYER_3")->Enabled = maxZDraw >= 3;
	this->baseForm->findControl("BUTTON_LAYER_4")->Enabled = maxZDraw >= 4;
	this->baseForm->findControl("BUTTON_LAYER_5")->Enabled = maxZDraw >= 5;
	this->baseForm->findControl("BUTTON_LAYER_6")->Enabled = maxZDraw >= 6;
	this->baseForm->findControl("BUTTON_LAYER_7")->Enabled = maxZDraw >= 7;
	this->baseForm->findControl("BUTTON_LAYER_8")->Enabled = maxZDraw >= 8;
	this->baseForm->findControl("BUTTON_LAYER_9")->Enabled = maxZDraw >= 9;
	this->baseForm->findControlTyped<RadioButton>("BUTTON_LAYER_1")->setChecked(true);
	this->setZLevel(1);
}

void BattleView::resume() {}

void BattleView::render()
{
	TRACE_FN;

	BattleTileView::render();
	activeTab->render();
	baseForm->render();
}

void BattleView::setUpdateSpeed(BattleUpdateSpeed updateSpeed)
{
	switch (updateSpeed)
	{
		case BattleUpdateSpeed::Pause:
			baseForm->findControlTyped<RadioButton>("BUTTON_SPEED0")->setChecked(true);
			break;
		case BattleUpdateSpeed::Speed1:
			baseForm->findControlTyped<RadioButton>("BUTTON_SPEED1")->setChecked(true);
			break;
		case BattleUpdateSpeed::Speed2:
			baseForm->findControlTyped<RadioButton>("BUTTON_SPEED2")->setChecked(true);
			break;
		case BattleUpdateSpeed::Speed3:
			baseForm->findControlTyped<RadioButton>("BUTTON_SPEED3")->setChecked(true);
			break;
	}
}

void BattleView::update()
{
	unsigned int ticks = 0;
	switch (this->updateSpeed)
	{
		case BattleUpdateSpeed::Pause:
			ticks = 0;
			break;
		/* POSSIBLE FIXME: 'vanilla' apoc appears to implement Speed1 as 1/2 speed - that is only
		* every other call calls the update loop, meaning that the later update tick counts are
		* halved as well.
		* This effectively means that all openapoc tick counts count for 1/2 the value of vanilla
		* apoc ticks */
		case BattleUpdateSpeed::Speed1:
			ticks = 1;
			break;
		case BattleUpdateSpeed::Speed2:
			ticks = 2;
			break;
		case BattleUpdateSpeed::Speed3:
			ticks = 4;
			break;
	}
	while (ticks > 0)
	{
		this->state->update();
		ticks--;
	}
	auto clockControl = baseForm->findControlTyped<Label>("CLOCK");

	clockControl->setText(state->gameTime.getTimeString());

	this->pal = palette;

	// FIXME: Possibly more efficient ways than re-generating all controls every frame?

	activeTab->update();
	baseForm->update();

	// If we have 'follow vehicle' enabled we clobber any other movement that may have occurred in
	// this frame
	if (this->followAgent)
	{
		// No agents yet!
	}
}

void BattleView::eventOccurred(Event *e)
{
	static const std::set<int> processedKeystrokes = {SDLK_ESCAPE, SDLK_PAGEUP, SDLK_PAGEDOWN,
	                                                  SDLK_TAB};

	activeTab->eventOccured(e);
	baseForm->eventOccured(e);

	if (activeTab->eventIsWithin(e) || baseForm->eventIsWithin(e))
	{
		return;
	}

	if (e->type() == EVENT_KEY_DOWN &&
	    processedKeystrokes.find(e->keyboard().KeyCode) != processedKeystrokes.end())
	{
		switch (e->keyboard().KeyCode)
		{
			case SDLK_ESCAPE:
				fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<InGameOptions>(state)});
				return;
			case SDLK_PAGEUP:
				this->setZLevel(getZLevel() + 1);
				updateLayerButtons();
				break;
			case SDLK_PAGEDOWN:
				this->setZLevel(getZLevel() - 1);
				updateLayerButtons();
				break;
			case SDLK_TAB:
				this->baseForm->findControlTyped<CheckBox>("BUTTON_TOGGLE_STRATMAP")
				    ->setChecked(
				        !this->baseForm->findControlTyped<CheckBox>("BUTTON_TOGGLE_STRATMAP")
				             ->isChecked());
				break;
		}
	}
	// Exclude mouse down events that are over the form
	else if (e->type() == EVENT_MOUSE_DOWN)
	{
		if (this->getViewMode() == TileViewMode::Strategy && e->type() == EVENT_MOUSE_DOWN &&
		    e->mouse().Button == 2)
		{
			Vec2<float> screenOffset = {this->getScreenOffset().x, this->getScreenOffset().y};
			auto clickTile = this->screenToTileCoords(
			    Vec2<float>{e->mouse().X, e->mouse().Y} - screenOffset, 0.0f);
			this->setScreenCenterTile({clickTile.x, clickTile.y});
		}
		else if (e->type() == EVENT_MOUSE_DOWN && e->mouse().Button == 1)
		{
			// If a click has not been handled by a form it's in the map. See if we intersect with
			// anything
			Vec2<float> screenOffset = {this->getScreenOffset().x, this->getScreenOffset().y};
			auto clickTop = this->screenToTileCoords(
			    Vec2<float>{e->mouse().X, e->mouse().Y} - screenOffset, (float)maxZDraw - 0.01f);
			auto clickBottom = this->screenToTileCoords(
			    Vec2<float>{e->mouse().X, e->mouse().Y} - screenOffset, 0.0f);
			auto collision = state->battle.map->findCollision(clickTop, clickBottom);
			if (collision)
			{
				if (collision.obj->getType() == BattleTileObject::Type::Ground ||
				    collision.obj->getType() == BattleTileObject::Type::LeftWall ||
				    collision.obj->getType() == BattleTileObject::Type::RightWall ||
				    collision.obj->getType() == BattleTileObject::Type::Scenery)
				{
					auto map_part =
					    std::dynamic_pointer_cast<BattleTileObjectMapPart>(collision.obj)
					        ->getOwner();
					LogInfo("Clicked on map part %s at {%f,%f,%f}",
					        BattleMapPartType::TypeMap.at(map_part->type->type).cStr(),
					        map_part->currentPosition.x, map_part->currentPosition.y,
					        map_part->currentPosition.z);
				}
			}
		}
	}
	else if (e->type() == EVENT_GAME_STATE)
	{
		auto gameEvent = dynamic_cast<GameEvent *>(e);
		if (!gameEvent)
		{
			LogError("Invalid game state event");
			return;
		}
		/*if (!gameEvent->message().empty())
		{
		    state->logEvent(gameEvent);
		    baseForm->findControlTyped<Ticker>("NEWS_TICKER")->addMessage(gameEvent->message());
		    auto notification = mksp<NotificationScreen>(state, *this, gameEvent->message());
		    stageCmd.cmd = StageCmd::Command::PUSH;
		    stageCmd.nextStage = notification;
		}*/
		switch (gameEvent->type)
		{
			default:
				break;
		}
	}
	else
	{
		BattleTileView::eventOccurred(e);
	}
}

void BattleView::updateLayerButtons()
{
	switch (this->getZLevel())
	{
		case 1:
			this->baseForm->findControlTyped<RadioButton>("BUTTON_LAYER_1")->setChecked(true);
			break;
		case 2:
			this->baseForm->findControlTyped<RadioButton>("BUTTON_LAYER_2")->setChecked(true);
			break;
		case 3:
			this->baseForm->findControlTyped<RadioButton>("BUTTON_LAYER_3")->setChecked(true);
			break;
		case 4:
			this->baseForm->findControlTyped<RadioButton>("BUTTON_LAYER_4")->setChecked(true);
			break;
		case 5:
			this->baseForm->findControlTyped<RadioButton>("BUTTON_LAYER_5")->setChecked(true);
			break;
		case 6:
			this->baseForm->findControlTyped<RadioButton>("BUTTON_LAYER_6")->setChecked(true);
			break;
		case 7:
			this->baseForm->findControlTyped<RadioButton>("BUTTON_LAYER_7")->setChecked(true);
			break;
		case 8:
			this->baseForm->findControlTyped<RadioButton>("BUTTON_LAYER_8")->setChecked(true);
			break;
		case 9:
			this->baseForm->findControlTyped<RadioButton>("BUTTON_LAYER_9")->setChecked(true);
			break;
	}
}

}; // namespace OpenApoc
