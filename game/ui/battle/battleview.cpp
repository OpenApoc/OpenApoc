#include "game/ui/battle/battleview.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battleunit.h"
#include "game/state/battle/battlemappart_type.h"
#include "game/state/gameevent.h"
#include "game/state/tileview/collision.h"
#include "game/state/tileview/tileobject_battlemappart.h"
#include "game/state/tileview/tileobject_battleunit.h"
#include "game/ui/base/basescreen.h"
#include "game/ui/general/ingameoptions.h"
#include "game/ui/tileview/tileview.h"
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
    : TileView(*state->current_battle->map, Vec3<int>{BATTLE_TILE_X, BATTLE_TILE_Y, BATTLE_TILE_Z},
               Vec2<int>{BATTLE_STRAT_TILE_X, BATTLE_STRAT_TILE_Y}, TileViewMode::Isometric,
               TileView::Mode::Battle),
      baseForm(ui().getForm("FORM_BATTLE_UI")), updateSpeed(BattleUpdateSpeed::Pause),
      lastSpeed(BattleUpdateSpeed::Speed1), state(state), followAgent(false),
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
				    setLayerDrawingMode(LayerDrawingMode::UpToCurrentLevel);
				    break;
			    case 2:
				    setLayerDrawingMode(LayerDrawingMode::AllLevels);
				    break;
			    case 3:
				    setLayerDrawingMode(LayerDrawingMode::OnlyCurrentLevel);
				    break;
		    }

		});


	this->baseForm->findControl("BUTTON_CEASE_FIRE")
		->addCallback(FormEventType::MouseClick, [this](Event *) {
		bool at_will = false;
		for (auto u : selectedUnits)
		{
			if (u->fire_permission_mode == BattleUnit::FirePermissionMode::AtWill)
			{
				at_will = true;
			}
		}
		for (auto u : selectedUnits)
		{
			if (at_will)
			{
				u->fire_permission_mode = BattleUnit::FirePermissionMode::CeaseFire;
			}
			else
			{
				u->fire_permission_mode = BattleUnit::FirePermissionMode::AtWill;
			}

		}
	});
	this->baseForm->findControl("BUTTON_AIMED")
		->addCallback(FormEventType::MouseClick, [this](Event *) {
		for (auto u : selectedUnits)
		{
			u->fire_aiming_mode = BattleUnit::FireAimingMode::Aimed;
		}
	});
	this->baseForm->findControl("BUTTON_SNAP")
		->addCallback(FormEventType::MouseClick, [this](Event *) {
		for (auto u : selectedUnits)
		{
			u->fire_aiming_mode = BattleUnit::FireAimingMode::Snap;
		}
	});
	this->baseForm->findControl("BUTTON_AUTO")
		->addCallback(FormEventType::MouseClick, [this](Event *) {
		for (auto u : selectedUnits)
		{
			u->fire_aiming_mode = BattleUnit::FireAimingMode::Auto;
		}
	});
	this->baseForm->findControl("BUTTON_KNEEL")
		->addCallback(FormEventType::MouseClick, [this](Event *) {
		bool not_kneeling = false;

		for (auto u : selectedUnits)
		{
			if (u->kneeling_mode == BattleUnit::KneelingMode::None)
			{
				not_kneeling = true;
			}
		}
		for (auto u : selectedUnits)
		{
			if (not_kneeling)
			{
				u->kneeling_mode = BattleUnit::KneelingMode::Kneeling;
			}
			else
			{
				u->kneeling_mode = BattleUnit::KneelingMode::None;
			}
		}
	});
	this->baseForm->findControl("BUTTON_PRONE")
		->addCallback(FormEventType::MouseClick, [this](Event *) {
		for (auto u : selectedUnits)
		{
			u->movement_mode = BattleUnit::MovementMode::Prone;
		}
	});
	this->baseForm->findControl("BUTTON_WALK")
		->addCallback(FormEventType::MouseClick, [this](Event *) {
		for (auto u : selectedUnits)
		{
			u->movement_mode = BattleUnit::MovementMode::Walking;
		}
	});
	this->baseForm->findControl("BUTTON_RUN")
		->addCallback(FormEventType::MouseClick, [this](Event *) {
		for (auto u : selectedUnits)
		{
			u->movement_mode = BattleUnit::MovementMode::Running;
		}
	});
	this->baseForm->findControl("BUTTON_EVASIVE")
		->addCallback(FormEventType::MouseClick, [this](Event *) {
		for (auto u : selectedUnits)
		{
			u->behavior_mode = BattleUnit::BehaviorMode::Evasive;
		}
	});
	this->baseForm->findControl("BUTTON_NORMAL")
		->addCallback(FormEventType::MouseClick, [this](Event *) {
		for (auto u : selectedUnits)
		{
			u->behavior_mode = BattleUnit::BehaviorMode::Normal;
		}
	});
	this->baseForm->findControl("BUTTON_AGGRESSIVE")
		->addCallback(FormEventType::MouseClick, [this](Event *) {
		for (auto u : selectedUnits)
		{
			u->behavior_mode = BattleUnit::BehaviorMode::Aggressive;
		}
	});

	this->baseForm->findControl("BUTTON_LAYER_UP")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->setZLevel(getZLevel() + 1);
		    updateLayerButtons();
		});
	this->baseForm->findControl("BUTTON_LAYER_DOWN")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
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
	this->baseForm->findControl("BUTTON_FOLLOW_AGENT")
		->addCallback(FormEventType::CheckBoxChange, [this](Event *e) {
		this->followAgent =
			std::dynamic_pointer_cast<CheckBox>(e->forms().RaisedBy)->isChecked();
	});
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
	this->baseForm->findControl("BUTTON_LAYER_1")->Visible = maxZDraw >= 1;
	this->baseForm->findControl("BUTTON_LAYER_2")->Visible = maxZDraw >= 2;
	this->baseForm->findControl("BUTTON_LAYER_3")->Visible = maxZDraw >= 3;
	this->baseForm->findControl("BUTTON_LAYER_4")->Visible = maxZDraw >= 4;
	this->baseForm->findControl("BUTTON_LAYER_5")->Visible = maxZDraw >= 5;
	this->baseForm->findControl("BUTTON_LAYER_6")->Visible = maxZDraw >= 6;
	this->baseForm->findControl("BUTTON_LAYER_7")->Visible = maxZDraw >= 7;
	this->baseForm->findControl("BUTTON_LAYER_8")->Visible = maxZDraw >= 8;
	this->baseForm->findControl("BUTTON_LAYER_9")->Visible = maxZDraw >= 9;
	this->baseForm->findControlTyped<RadioButton>("BUTTON_LAYER_1")->setChecked(true);
	this->setZLevel(1);
}

void BattleView::resume() {}

void BattleView::render()
{
	TRACE_FN;

	// FIXME: Draw charges
	// Set items in hands
	if (selectedUnits.size() > 0)
	{
		auto u = *selectedUnits.begin();
		{
			auto e = u->agent->getFirstItemInSlot(AgentType::EquipmentSlotType::RightHand);
			if (e)
			{
				this->baseForm->findControlTyped<Graphic>("IMAGE_RIGHT_HAND")->setImage(e->type->equipscreen_sprite);
				auto p = e->getPayloadType();
				if (p && p->damage_type)
				{
					this->baseForm->findControlTyped<Graphic>("IMAGE_RIGHT_DAMAGETYPE")->setImage(p->damage_type->icon_sprite);
				}
				else
				{
					this->baseForm->findControlTyped<Graphic>("IMAGE_RIGHT_DAMAGETYPE")->setImage(nullptr);
				}
			}
			else
			{
				this->baseForm->findControlTyped<Graphic>("IMAGE_RIGHT_HAND")->setImage(nullptr);
				this->baseForm->findControlTyped<Graphic>("IMAGE_RIGHT_DAMAGETYPE")->setImage(nullptr);
			}
		}
		{
			auto e = u->agent->getFirstItemInSlot(AgentType::EquipmentSlotType::LeftHand);
			if (e)
			{
				this->baseForm->findControlTyped<Graphic>("IMAGE_LEFT_HAND")->setImage(e->type->equipscreen_sprite);
				auto p = e->getPayloadType();
				if (p && p->damage_type)
				{
					this->baseForm->findControlTyped<Graphic>("IMAGE_LEFT_DAMAGETYPE")->setImage(p->damage_type->icon_sprite);
				}
				else
				{
					this->baseForm->findControlTyped<Graphic>("IMAGE_LEFT_DAMAGETYPE")->setImage(nullptr);
				}
			}
			else
			{
				this->baseForm->findControlTyped<Graphic>("IMAGE_LEFT_HAND")->setImage(nullptr);
				this->baseForm->findControlTyped<Graphic>("IMAGE_LEFT_DAMAGETYPE")->setImage(nullptr);
			}
		}
	}
	else
	{
		this->baseForm->findControlTyped<Graphic>("IMAGE_RIGHT_HAND")->setImage(nullptr);
		this->baseForm->findControlTyped<Graphic>("IMAGE_RIGHT_DAMAGETYPE")->setImage(nullptr);
		this->baseForm->findControlTyped<Graphic>("IMAGE_LEFT_HAND")->setImage(nullptr);
		this->baseForm->findControlTyped<Graphic>("IMAGE_LEFT_DAMAGETYPE")->setImage(nullptr);
	}
	
	TileView::render();
	activeTab->render();
	baseForm->render();

	// If there's a modal dialog, darken the screen
	if (fw().stageGetCurrent() != this->shared_from_this())
	{
		fw().renderer->drawFilledRect({0, 0}, fw().displayGetSize(), Colour{0, 0, 0, 128});
	}
}

void BattleView::setUpdateSpeed(BattleUpdateSpeed updateSpeed)
{
	this->lastSpeed = this->updateSpeed;
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
	updateSelectedUnits();
	updateSelectionMode();
	updateSoldierButtons();

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

	// FIXME: Pulsate palette colors for strategy map and gravlifts
	this->pal = palette;

	// FIXME: Possibly more efficient ways than re-generating all controls every frame?

	activeTab->update();
	baseForm->update();

	// If we have 'follow agent' enabled we clobber any other movement that may have occurred in
	// this frame
	if (this->followAgent)
	{
		if (selectedUnits.size() > 0)
		{
			auto u = *selectedUnits.begin();
			setScreenCenterTile(u->getPosition());
		}
	}
}

void BattleView::updateSelectedUnits()
{
	auto it = selectedUnits.begin();
	while (it != selectedUnits.end())
	{
		auto u = *it;
		auto o = state->getPlayer();
		if (!u || u->isDead() || u->isUnconscious() || u->owner != o)
		{
			it = selectedUnits.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void BattleView::updateSelectionMode()
{
	// FIXME: Add Throwing and Psi in the mix
	if (selectedUnits.size() == 0)
	{
		if (ModifierRCtrl || ModifierRCtrl)
		{
			selectionState = BattleSelectionState::NormalCtrl;
		}
		else
		{
			selectionState = BattleSelectionState::Normal;
		}
	}
	else
	{
		if (ModifierLCtrl || ModifierRCtrl)
		{
			selectionState = BattleSelectionState::NormalCtrl;
		}
		else if (ModifierLShift || ModifierRShift)
		{
			selectionState = BattleSelectionState::Fire;
		}
		else if (ModifierLAlt || ModifierRAlt)
		{
			selectionState = BattleSelectionState::NormalAlt;
		}
		else
		{
			selectionState = BattleSelectionState::Normal;
		}
	}
}

void BattleView::updateSoldierButtons()
{
	bool cease_fire = false;
	bool at_will = false;
	bool aimed = false;
	bool snap = false;
	bool auto_fire = false;
	bool kneeling = false;
	bool not_kneeling = false;
	bool prone = false;
	bool walk = false;
	bool run = false;
	bool evasive = false;
	bool normal = false;
	bool aggressive = false;

	for (auto u : selectedUnits)
	{
		switch (u->fire_aiming_mode)
		{
			case BattleUnit::FireAimingMode::Aimed:
				aimed = true;
				break;
			case BattleUnit::FireAimingMode::Snap:
				snap = true;
				break;
			case BattleUnit::FireAimingMode::Auto:
				auto_fire = true;
				break;
		}
		if (u->fire_permission_mode == BattleUnit::FirePermissionMode::CeaseFire)
		{
			cease_fire = true;
		}
		else
		{
			at_will = true;
		}
		switch (u->behavior_mode)
		{
			case BattleUnit::BehaviorMode::Evasive:
				evasive = true;
				break;
			case BattleUnit::BehaviorMode::Normal:
				normal = true;
				break;
			case BattleUnit::BehaviorMode::Aggressive:
				aggressive = true;
				break;
		}
		switch (u->movement_mode)
		{
			case BattleUnit::MovementMode::Prone:
				prone = true;
				break;
			case BattleUnit::MovementMode::Walking:
				walk = true;
				break;
			case BattleUnit::MovementMode::Running:
				run = true;
				break;
		}
		if (u->kneeling_mode == BattleUnit::KneelingMode::Kneeling)
		{
			kneeling = true;
		}
		else
		{
			not_kneeling = true;
		}
	}

	this->baseForm->findControlTyped<TriStateBox>("BUTTON_CEASE_FIRE")->setState(cease_fire && at_will ? 3 : (cease_fire ? 2 : 1));
	this->baseForm->findControlTyped<CheckBox>("BUTTON_AIMED")->setChecked(aimed);
	this->baseForm->findControlTyped<CheckBox>("BUTTON_SNAP")->setChecked(snap);
	this->baseForm->findControlTyped<CheckBox>("BUTTON_AUTO")->setChecked(auto_fire);
	this->baseForm->findControlTyped<TriStateBox>("BUTTON_KNEEL")->setState(kneeling && not_kneeling ? 3 : (kneeling ? 2 : 1));
	this->baseForm->findControlTyped<CheckBox>("BUTTON_PRONE")->setChecked(prone);
	this->baseForm->findControlTyped<CheckBox>("BUTTON_WALK")->setChecked(walk);
	this->baseForm->findControlTyped<CheckBox>("BUTTON_RUN")->setChecked(run);
	this->baseForm->findControlTyped<CheckBox>("BUTTON_EVASIVE")->setChecked(evasive);
	this->baseForm->findControlTyped<CheckBox>("BUTTON_NORMAL")->setChecked(normal);
	this->baseForm->findControlTyped<CheckBox>("BUTTON_AGGRESSIVE")->setChecked(aggressive);
}

void BattleView::eventOccurred(Event *e)
{
	activeTab->eventOccured(e);
	baseForm->eventOccured(e);

	if (activeTab->eventIsWithin(e) || baseForm->eventIsWithin(e))
	{
		return;
	}

	if (e->type() == EVENT_KEY_DOWN &&(
		e->keyboard().KeyCode == SDLK_ESCAPE
		|| e->keyboard().KeyCode == SDLK_PAGEUP
		|| e->keyboard().KeyCode == SDLK_PAGEDOWN
		|| e->keyboard().KeyCode == SDLK_TAB
		|| e->keyboard().KeyCode == SDLK_SPACE
		|| e->keyboard().KeyCode == SDLK_RSHIFT
		|| e->keyboard().KeyCode == SDLK_LSHIFT
		|| e->keyboard().KeyCode == SDLK_RALT
		|| e->keyboard().KeyCode == SDLK_LALT
		|| e->keyboard().KeyCode == SDLK_RCTRL
		|| e->keyboard().KeyCode == SDLK_LCTRL
		))
	{
		switch (e->keyboard().KeyCode)
		{
			case SDLK_RSHIFT:
				ModifierRShift = true;
				updateSelectionMode();
				break;
			case SDLK_LSHIFT:
				ModifierLShift = true;
				updateSelectionMode();
				break;
			case SDLK_RALT:
				ModifierRAlt = true;
				updateSelectionMode();
				break;
			case SDLK_LALT:
				ModifierLAlt = true;
				updateSelectionMode();
				break;
			case SDLK_RCTRL:
				ModifierLCtrl = true;
				updateSelectionMode();
				break;
			case SDLK_LCTRL:
				ModifierRCtrl = true;
				updateSelectionMode();
				break;
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
			case SDLK_SPACE:
				if (this->updateSpeed != BattleUpdateSpeed::Pause)
					setUpdateSpeed(BattleUpdateSpeed::Pause);
				else
					setUpdateSpeed(this->lastSpeed);
				break;
		}
	}
	else if (e->type() == EVENT_KEY_UP && (
		e->keyboard().KeyCode == SDLK_RSHIFT
		|| e->keyboard().KeyCode == SDLK_LSHIFT
		|| e->keyboard().KeyCode == SDLK_RALT
		|| e->keyboard().KeyCode == SDLK_LALT
		|| e->keyboard().KeyCode == SDLK_RCTRL
		|| e->keyboard().KeyCode == SDLK_LCTRL
		))
	{
		switch (e->keyboard().KeyCode)
		{
		case SDLK_RSHIFT:
			ModifierRShift = false;
			updateSelectionMode();
			break;
		case SDLK_LSHIFT:
			ModifierLShift = false;
			updateSelectionMode();
			break;
		case SDLK_RALT:
			ModifierRAlt = false;
			updateSelectionMode();
			break;
		case SDLK_LALT:
			ModifierLAlt = false;
			updateSelectionMode();
			break;
		case SDLK_RCTRL:
			ModifierLCtrl = false;
			updateSelectionMode();
			break;
		case SDLK_LCTRL:
			ModifierRCtrl = false;
			updateSelectionMode();
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
		else if (e->type() == EVENT_MOUSE_DOWN &&
		         (e->mouse().Button == 1 
				|| e->mouse().Button == 4))
		{
			// If a click has not been handled by a form it's in the map.
			auto t = this->getSelectedTilePosition();
			auto o = map.getTile(t.x, t.y, t.z)->getUnitIfPresent();
			auto u = o ? o->getUnit() : nullptr;
		
			switch (selectionState)
			{
			case BattleSelectionState::Normal:
				switch (e->mouse().Button)
				{
				case 1:
					// Select if friendly unit present under cursor
					if (u && u->owner == state->getPlayer())
					{
						auto pos = std::find(selectedUnits.begin(), selectedUnits.end(), u);
						if (pos == selectedUnits.end())
						{
							// Unit not in selection => replace selection with unit
							selectedUnits.clear();
							selectedUnits.push_back(u);
						}
						else
						{
							// Unit in selection  => move unit to front
							selectedUnits.erase(pos);
							selectedUnits.push_front(u);
						}
					}
					else if (!u)
					{
						// FIXME: Order move here
					}
					break;
				case 4:
					if (selectedUnits.size() == 0)
						break;
					// FIXME: Turn to look, focus fire if clicked on enemy
					break;
				}
				break;
			case BattleSelectionState::NormalAlt:
				switch (e->mouse().Button)
				{
				case 1:
					// FIXME: Move strafing
					break;
				case 4:
					if (selectedUnits.size() == 0)
						break;
					// FIXME: Turn to look
					break;
				}
				break;
			case BattleSelectionState::NormalCtrl:
				switch (e->mouse().Button)
				{
				// LMB = Add to selection
				case 1:
					if (u && u->owner == state->getPlayer())
					{
						auto pos = std::find(selectedUnits.begin(), selectedUnits.end(), u);
						if (pos == selectedUnits.end())
						{
							// Unit not in selection, and not full => add unit to selection
							if (selectedUnits.size() < 6)
							{
								selectedUnits.push_front(u);
							}
						}
						else
						{
							// Unit in selection  => move unit to front
							selectedUnits.erase(pos);
							selectedUnits.push_front(u);
						}
					}
					break;
				// RMB = Remove from selection
				case 4:
					if (u && u->owner == state->getPlayer())
					{
						auto pos = std::find(selectedUnits.begin(), selectedUnits.end(), u);
						if (pos != selectedUnits.end())
						{
							// Unit in selection => remove
							selectedUnits.erase(pos);
						}
					}
					break;
				}
				break;
			case BattleSelectionState::Fire:
				if (e->mouse().Button != 1 && e->mouse().Button != 4)
					break;
				if (selectedUnits.size() == 0)
					break;
				// FIXME: Fire
				break;
			}
			LogWarning("Click at tile %d, %d, %d", t.x, t.y, t.z);
			LogWarning("Selected units count: %d", (int)selectedUnits.size());
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
		TileView::eventOccurred(e);
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
