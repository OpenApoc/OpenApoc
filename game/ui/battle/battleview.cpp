#include "game/ui/battle/battleview.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battlemappart_type.h"
#include "game/state/battle/battleunit.h"
#include "game/state/gameevent.h"
#include "game/state/tileview/collision.h"
#include "game/state/tileview/tileobject_battlemappart.h"
#include "game/state/tileview/tileobject_battleunit.h"
#include "game/ui/base/basescreen.h"
#include "game/ui/general/ingameoptions.h"
#include "library/sp.h"

namespace OpenApoc
{

namespace
{
static const std::vector<UString> TAB_FORM_NAMES_RT = {
    "FORM_BATTLE_UI_RT_1", "FORM_BATTLE_UI_RT_2", "FORM_BATTLE_UI_RT_3",
};
static const std::vector<UString> TAB_FORM_NAMES_TB = {
    "FORM_BATTLE_UI_TB_1", "FORM_BATTLE_UI_TB_2", "FORM_BATTLE_UI_TB_3",
};
} // anonymous namespace

BattleView::BattleView(sp<GameState> state)
    : BattleTileView(
          *state->current_battle->map, Vec3<int>{TILE_X_BATTLE, TILE_Y_BATTLE, TILE_Z_BATTLE},
          Vec2<int>{STRAT_TILE_X, STRAT_TILE_Y}, TileViewMode::Isometric,
          state->current_battle->battleviewZLevel, state->current_battle->battleviewScreenCenter),
      baseForm(ui().getForm("FORM_BATTLE_UI")), state(state), followAgent(false),
      palette(fw().data->loadPalette("xcom3/tacdata/tactical.pal")),
      selectionState(BattleSelectionState::Normal)
{
	this->pal = palette;

	selectedItemOverlay = fw().data->loadImage("battle/battle-item-select-icon.png");

	for (auto &formName : TAB_FORM_NAMES_RT)
	{
		sp<Form> f(ui().getForm(formName));
		if (!f)
		{
			LogError("Failed to load form \"%s\"", formName.cStr());
			return;
		}
		f->takesFocus = false;
		this->uiTabsRT.push_back(f);
	}
	for (auto &formName : TAB_FORM_NAMES_TB)
	{
		sp<Form> f(ui().getForm(formName));
		if (!f)
		{
			LogError("Failed to load form \"%s\"", formName.cStr());
			return;
		}
		f->takesFocus = false;
		this->uiTabsTB.push_back(f);
	}

	switch (state->current_battle->mode)
	{
		case Battle::Mode::RealTime:
			this->activeTab = this->uiTabsRT[0];
			baseForm->findControlTyped<RadioButton>("BUTTON_SPEED0")->setChecked(true);
			updateSpeed = BattleUpdateSpeed::Pause;
			lastSpeed = BattleUpdateSpeed::Speed1;
			break;
		case Battle::Mode::TurnBased:
			this->activeTab = this->uiTabsTB[0];
			updateSpeed = BattleUpdateSpeed::Speed2;
			lastSpeed = BattleUpdateSpeed::Pause;
			break;
		default:
			LogError("Unexpected battle mode \"%d\"", (int)state->current_battle->mode);
			break;
	}

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
			    if (u->kneeling_mode == BattleUnit::KneelingMode::None &&
			        u->agent->isBodyStateAllowed(AgentType::BodyState::Kneeling))
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
			    if (u->agent->isBodyStateAllowed(AgentType::BodyState::Prone))
			    {
				    u->movement_mode = BattleUnit::MovementMode::Prone;
			    }
		    }
		});
	this->baseForm->findControl("BUTTON_WALK")
	    ->addCallback(FormEventType::MouseClick, [this](Event *) {
		    for (auto u : selectedUnits)
		    {
			    if (u->agent->isBodyStateAllowed(AgentType::BodyState::Standing) ||
			        u->agent->isBodyStateAllowed(AgentType::BodyState::Flying))
			    {
				    u->movement_mode = BattleUnit::MovementMode::Walking;
			    }
		    }
		});
	this->baseForm->findControl("BUTTON_RUN")
	    ->addCallback(FormEventType::MouseClick, [this](Event *) {
		    for (auto u : selectedUnits)
		    {
			    if (u->agent->isBodyStateAllowed(AgentType::BodyState::Standing) ||
			        u->agent->isBodyStateAllowed(AgentType::BodyState::Flying))
			    {
				    u->movement_mode = BattleUnit::MovementMode::Running;
			    }
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

	this->baseForm->findControl("BUTTON_SHOW_OPTIONS")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<InGameOptions>(this->state)});
		});
	this->baseForm->findControl("BUTTON_SHOW_LOG")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) { LogWarning("Show log"); });
	this->baseForm->findControl("BUTTON_ZOOM_EVENT")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) { LogWarning("Zoom to event"); });

	// FIXME: When clicking on items or weapons, activate them or go into fire / teleport mode
	// accordingly

	std::function<void(FormsEvent * e)> clickedRightHand = [this](Event *) {
		LogWarning("Clicked right hand");
	};

	std::function<void(FormsEvent * e)> clickedLeftHand = [this](Event *) {
		LogWarning("Clicked left hand");
	};

	std::function<void(FormsEvent * e)> dropRightHand = [this](Event *) { orderDrop(true); };

	std::function<void(FormsEvent * e)> dropLeftHand = [this](Event *) { orderDrop(false); };

	std::function<void(bool right)> throwItem = [this](bool right) {
		if (selectedUnits.size() == 0 ||
		    !(selectedUnits.front()->agent->getFirstItemInSlot(
		        right ? AgentEquipmentLayout::EquipmentSlotType::RightHand
		              : AgentEquipmentLayout::EquipmentSlotType::LeftHand)))
		{
			if (right)
			{
				this->rightThrowDelay = 5;
			}
			else
			{
				this->leftThrowDelay = 5;
			}
			return;
		}
		this->selectionState =
		    right ? BattleSelectionState::ThrowRight : BattleSelectionState::ThrowLeft;
	};

	std::function<void(FormsEvent * e)> throwRightHand = [this, throwItem](Event *) {
		throwItem(true);
	};

	std::function<void(FormsEvent * e)> throwLeftHand = [this, throwItem](Event *) {
		throwItem(false);
	};

	this->uiTabsRT[0]
	    ->findControlTyped<Graphic>("OVERLAY_RIGHT_HAND")
	    ->addCallback(FormEventType::MouseClick, clickedRightHand);
	this->uiTabsRT[0]
	    ->findControlTyped<Graphic>("OVERLAY_LEFT_HAND")
	    ->addCallback(FormEventType::MouseClick, clickedLeftHand);
	this->uiTabsTB[0]
	    ->findControlTyped<Graphic>("OVERLAY_RIGHT_HAND")
	    ->addCallback(FormEventType::MouseClick, clickedRightHand);
	this->uiTabsTB[0]
	    ->findControlTyped<Graphic>("OVERLAY_LEFT_HAND")
	    ->addCallback(FormEventType::MouseClick, clickedLeftHand);
	this->uiTabsRT[0]
	    ->findControlTyped<GraphicButton>("BUTTON_RIGHT_HAND_DROP")
	    ->addCallback(FormEventType::MouseClick, dropRightHand);
	this->uiTabsRT[0]
	    ->findControlTyped<GraphicButton>("BUTTON_LEFT_HAND_DROP")
	    ->addCallback(FormEventType::MouseClick, dropLeftHand);
	this->uiTabsTB[0]
	    ->findControlTyped<GraphicButton>("BUTTON_RIGHT_HAND_DROP")
	    ->addCallback(FormEventType::MouseClick, dropRightHand);
	this->uiTabsTB[0]
	    ->findControlTyped<GraphicButton>("BUTTON_LEFT_HAND_DROP")
	    ->addCallback(FormEventType::MouseClick, dropLeftHand);
	this->uiTabsRT[0]
	    ->findControlTyped<CheckBox>("BUTTON_RIGHT_HAND_THROW")
	    ->addCallback(FormEventType::MouseClick, throwRightHand);
	this->uiTabsRT[0]
	    ->findControlTyped<CheckBox>("BUTTON_LEFT_HAND_THROW")
	    ->addCallback(FormEventType::MouseClick, throwLeftHand);
	this->uiTabsTB[0]
	    ->findControlTyped<CheckBox>("BUTTON_RIGHT_HAND_THROW")
	    ->addCallback(FormEventType::MouseClick, throwRightHand);
	this->uiTabsTB[0]
	    ->findControlTyped<CheckBox>("BUTTON_LEFT_HAND_THROW")
	    ->addCallback(FormEventType::MouseClick, throwLeftHand);

	switch (state->current_battle->mode)
	{
		case Battle::Mode::RealTime:
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
			// FIXME: Disable TB controls
			break;
		case Battle::Mode::TurnBased:
			// FIXME: Assign events to TB controls
			this->baseForm->findControl("BUTTON_SPEED0")->Visible = false;
			this->baseForm->findControl("BUTTON_SPEED1")->Visible = false;
			this->baseForm->findControl("BUTTON_SPEED2")->Visible = false;
			this->baseForm->findControl("BUTTON_SPEED3")->Visible = false;
			this->baseForm->findControl("CLOCK")->Visible = false;
			break;
		default:
			LogError("Unexpected battle mode \"%d\"", (int)state->current_battle->mode);
			break;
	}

	updateLayerButtons();
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
}

void BattleView::resume() {}

void BattleView::render()
{
	TRACE_FN;

	BattleTileView::render();
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
	// FIXME: Is there a more efficient way? But TileView does not know about battle or state!
	state->current_battle->battleviewScreenCenter = centerPos;
	state->current_battle->battleviewZLevel = getZLevel();

	updateSelectedUnits();
	updateSelectionMode();
	updateSoldierButtons();

	if (leftThrowDelay > 0)
	{
		leftThrowDelay--;
	}
	if (rightThrowDelay > 0)
	{
		rightThrowDelay--;
	}

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
	if (state->current_battle->mode == Battle::Mode::RealTime)
	{
		auto clockControl = baseForm->findControlTyped<Label>("CLOCK");
		clockControl->setText(state->gameTime.getTimeString());
	}

	// Pulsate palette colors
	auto newPal = mksp<Palette>();
	for (int i = 0; i < 255 - 4; i++)
	{
		newPal->setColour(i, palette->getColour(i));
	}
	colorCurrent += (colorForward ? 1 : -1) * 16;
	if (colorCurrent <= 0 || colorCurrent >= 255)
	{
		colorCurrent = clamp(colorCurrent, 0, 255);
		colorForward = !colorForward;
	}
	// Lift color, pulsates from (0r 3/8g 5/8b) to (0r 8/8g 4/8b)
	newPal->setColour(
	    255 - 4, Colour(0, (colorCurrent * 5 + 255 * 3) / 8, (colorCurrent * -1 + 255 * 5) / 8));
	// Red color, for enemy indicators, pulsates from (3/8r 0g 0b) to (8/8r 0g 0b)
	newPal->setColour(255 - 3, Colour((colorCurrent * 5 + 255 * 3) / 8, 0, 0));
	// Blue color, for misc. indicators, pulsates from (0r 3/8g 3/8b) to (0r 8/8g 8/8b)
	newPal->setColour(
	    255 - 2, Colour(0, (colorCurrent * 5 + 255 * 3) / 8, (colorCurrent * 5 + 255 * 3) / 8));
	// Pink color, for neutral indicators, pulsates from (3/8r 0g 3/8b) to (8/8r 0g 8/8b)
	newPal->setColour(
	    255 - 1, Colour((colorCurrent * 5 + 255 * 3) / 8, 0, (colorCurrent * 5 + 255 * 3) / 8));
	// Yellow color, for owned indicators, pulsates from (3/8r 3/8g 0b) to (8/8r 8/8g 0b)
	newPal->setColour(
	    255 - 0, Colour((colorCurrent * 5 + 255 * 3) / 8, (colorCurrent * 5 + 255 * 3) / 8, 0));
	this->pal = newPal;

	// Update weapons if required
	auto rightInfo = createItemOverlayInfo(true);
	if (!(rightInfo == rightHandInfo) &&
	    (this->activeTab == uiTabsRT[0] || this->activeTab == uiTabsTB[0]))
	{
		rightHandInfo = rightInfo;
		updateItemInfo(true);
	}
	auto leftInfo = createItemOverlayInfo(false);
	if (!(leftInfo == leftHandInfo) &&
	    (this->activeTab == uiTabsRT[0] || this->activeTab == uiTabsTB[0]))
	{
		leftHandInfo = leftInfo;
		updateItemInfo(false);
	}

	// FIXME: Possibly more efficient ways than re-generating all controls every frame?
	activeTab->update();
	baseForm->update();

	// If we have 'follow agent' enabled we clobber any other movement that may have occurred in
	// this frame
	if (this->followAgent)
	{
		if (selectedUnits.size() > 0)
		{
			setScreenCenterTile(selectedUnits.front()->tileObject->getPosition());
		}
	}
}

void BattleView::updateSelectedUnits()
{
	auto prevLSU = lastSelectedUnit;
	auto it = selectedUnits.begin();
	while (it != selectedUnits.end())
	{
		auto u = *it;
		auto o = state->getPlayer();
		if (!u || u->isDead() || u->isUnconscious() || u->owner != o || u->retreated)
		{
			it = selectedUnits.erase(it);
		}
		else
		{
			it++;
		}
	}
	lastSelectedUnit = selectedUnits.size() == 0 ? nullptr : selectedUnits.front();

	// Cancel stuff that cancels on unit change
	if (prevLSU != lastSelectedUnit)
	{
		if (selectionState == BattleSelectionState::ThrowLeft ||
		    selectionState == BattleSelectionState::ThrowRight)
		{
			selectionState = BattleSelectionState::Normal;
		}
	}
}

void BattleView::updateSelectionMode()
{
	// FIXME: Add Psi in the mix
	// FIXME: Change cursor
	if (selectedUnits.size() == 0)
	{
		if (modifierLCtrl || modifierRCtrl)
		{
			if (modifierLAlt || modifierRAlt)
			{
				if (modifierLShift || modifierRShift)
				{
					selectionState = BattleSelectionState::NormalCtrlAltShift;
				}
				else
				{
					selectionState = BattleSelectionState::NormalCtrlAlt;
				}
			}
			else
			{
				selectionState = BattleSelectionState::NormalCtrl;
			}
		}
		else
		{
			selectionState = BattleSelectionState::Normal;
		}
	}
	else
	{
		// To cancel out of throw, we must switch first agent or cancel throwing
		if (selectionState == BattleSelectionState::ThrowLeft ||
		    selectionState == BattleSelectionState::ThrowRight)
		{
			return;
		}
		if (modifierRCtrl || modifierRCtrl)
		{
			if (modifierLAlt || modifierRAlt)
			{
				if (modifierLShift || modifierRShift)
				{
					selectionState = BattleSelectionState::NormalCtrlAltShift;
				}
				else
				{
					selectionState = BattleSelectionState::NormalCtrlAlt;
				}
			}
			else
			{
				selectionState = BattleSelectionState::NormalCtrl;
			}
		}
		else if (modifierLShift || modifierRShift)
		{
			selectionState = BattleSelectionState::Fire;
		}
		else if (modifierLAlt || modifierRAlt)
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

	this->baseForm->findControlTyped<TriStateBox>("BUTTON_CEASE_FIRE")
	    ->setState(cease_fire && at_will ? 3 : (cease_fire ? 2 : 1));
	this->baseForm->findControlTyped<CheckBox>("BUTTON_AIMED")->setChecked(aimed);
	this->baseForm->findControlTyped<CheckBox>("BUTTON_SNAP")->setChecked(snap);
	this->baseForm->findControlTyped<CheckBox>("BUTTON_AUTO")->setChecked(auto_fire);
	this->baseForm->findControlTyped<TriStateBox>("BUTTON_KNEEL")
	    ->setState(kneeling && not_kneeling ? 3 : (kneeling ? 2 : 1));
	this->baseForm->findControlTyped<CheckBox>("BUTTON_PRONE")->setChecked(prone);
	this->baseForm->findControlTyped<CheckBox>("BUTTON_WALK")->setChecked(walk);
	this->baseForm->findControlTyped<CheckBox>("BUTTON_RUN")->setChecked(run);
	this->baseForm->findControlTyped<CheckBox>("BUTTON_EVASIVE")->setChecked(evasive);
	this->baseForm->findControlTyped<CheckBox>("BUTTON_NORMAL")->setChecked(normal);
	this->baseForm->findControlTyped<CheckBox>("BUTTON_AGGRESSIVE")->setChecked(aggressive);

	this->activeTab->findControlTyped<CheckBox>("BUTTON_LEFT_HAND_THROW")
	    ->setChecked(selectionState == BattleSelectionState::ThrowLeft || leftThrowDelay > 0);
	this->activeTab->findControlTyped<CheckBox>("BUTTON_RIGHT_HAND_THROW")
	    ->setChecked(selectionState == BattleSelectionState::ThrowRight || rightThrowDelay > 0);
}

void BattleView::attemptToClearCurrentOrders(sp<BattleUnit> u, bool overrideBodyStateChange)
{
	bool startRequired = false;

	for (auto it = u->missions.begin(); it != u->missions.end();)
	{
		auto m = it++;
		// See if we can remove the mission
		switch ((*m)->type)
		{
			// Missions that cannot be cancelled before finished
			case BattleUnitMission::MissionType::Fall:
			case BattleUnitMission::MissionType::Snooze:
			case BattleUnitMission::MissionType::DropItem:
			case BattleUnitMission::MissionType::ThrowItem:
				continue;
			case BattleUnitMission::MissionType::ChangeBodyState:
				if (overrideBodyStateChange)
					break;
				else
					continue;
			// Missions that can be cancelled before finished
			case BattleUnitMission::MissionType::AcquireTU:
			case BattleUnitMission::MissionType::GotoLocation:
			case BattleUnitMission::MissionType::ReachGoal:
			case BattleUnitMission::MissionType::RestartNextMission:
				break;
			// Special case - can cancel turning but must undo it
			case BattleUnitMission::MissionType::Turn:
				u->goalFacing = u->facing;
				u->turning_animation_ticks_remaining = 0;
				break;
			default:
				LogError("Unknown mission type %d!", (int)(*m)->type);
				continue;
		}
		if (it == u->missions.begin())
		{
			startRequired = true;
		}
		u->missions.erase(m);
	}
	if (startRequired && u->missions.size() > 0)
	{
		u->missions.front()->start(*this->state, *u);
	}
}

void BattleView::orderMove(Vec3<int> target, int facingOffset, bool demandGiveWay)
{
	// Check if ordered to exit
	bool runAway = map.getTile(target)->getHasExit();

	// FIXME: Handle group movement (don't forget to turn it off when running away)
	for (auto unit : selectedUnits)
	{
		attemptToClearCurrentOrders(unit);
		// FIXME: handle strafe and backwards movement properly
		if (runAway)
		{
			// Running away units are impatient!
			unit->missions.emplace_back(BattleUnitMission::gotoLocation(
			    *unit, target, facingOffset, true, 1, demandGiveWay, true));
		}
		else // not running away
		{
			unit->missions.emplace_back(BattleUnitMission::gotoLocation(*unit, target, facingOffset,
			                                                            true, 10, demandGiveWay));
		}
		if (unit->missions.size() == 1)
		{
			unit->missions.front()->start(*this->state, *unit);
			LogWarning("BattleUnit \"%s\" going to location {%d,%d,%d}", unit->agent->name.cStr(),
			           target.x, target.y, target.z);
		}
	}
}

bool BattleView::canEmplaceTurnInFront(sp<BattleUnit> u)
{
	bool onlyFallingEncountered = true;

	for (auto &m : u->missions)
	{
		switch (m->type)
		{
			case BattleUnitMission::MissionType::Fall:
				break;
			default:
				onlyFallingEncountered = false;
				break;
		}
	}
	return onlyFallingEncountered;
}

void BattleView::orderTurn(Vec3<int> target)
{
	for (auto unit : selectedUnits)
	{
		attemptToClearCurrentOrders(unit);
		if (canEmplaceTurnInFront(unit))
		{
			unit->missions.emplace_front(BattleUnitMission::turn(*unit, target));
			unit->missions.front()->start(*this->state, *unit);
			LogWarning("BattleUnit \"%s\" turning to face location {%d,%d,%d}",
			           unit->agent->name.cStr(), target.x, target.y, target.z);
		}
		else
		{
			unit->missions.emplace_back(BattleUnitMission::turn(*unit, target));
			if (unit->missions.size() == 1)
			{
				unit->missions.front()->start(*this->state, *unit);
				LogWarning("BattleUnit \"%s\" turning to face location {%d,%d,%d}",
				           unit->agent->name.cStr(), target.x, target.y, target.z);
			}
		}
	}
}

void BattleView::orderThrow(Vec3<int> target, bool right)
{
	if (selectedUnits.size() == 0)
	{
		return;
	}
	// FIXME: Check if we can actually throw it!

	auto unit = selectedUnits.front();
	auto item =
	    unit->agent->getFirstItemInSlot(right ? AgentEquipmentLayout::EquipmentSlotType::RightHand
	                                          : AgentEquipmentLayout::EquipmentSlotType::LeftHand);
	if (!item)
	{
		return;
	}

	// FIXME: actually read the option
	bool USER_OPTION_ALLOW_INSTANT_THROWS = false;
	attemptToClearCurrentOrders(unit, USER_OPTION_ALLOW_INSTANT_THROWS);

	if (unit->missions.size() > 0)
	{
		// FIXME: Report unable to throw
		return;
	}

	unit->missions.emplace_front(BattleUnitMission::throwItem(*unit, item, target));
	unit->missions.front()->start(*this->state, *unit);
	LogWarning("BattleUnit \"%s\" throwing %s hand item", unit->agent->name.cStr(),
	           right ? "right" : "left");
	selectionState = BattleSelectionState::Normal;
}

void BattleView::orderDrop(bool right)
{
	if (selectedUnits.size() == 0)
	{
		return;
	}
	auto unit = selectedUnits.front();
	auto item =
	    unit->agent->getFirstItemInSlot(right ? AgentEquipmentLayout::EquipmentSlotType::RightHand
	                                          : AgentEquipmentLayout::EquipmentSlotType::LeftHand);
	if (!item)
	{
		return;
	}
	unit->missions.emplace_front(BattleUnitMission::dropItem(*unit, item));
	unit->missions.front()->start(*this->state, *unit);
	LogWarning("BattleUnit \"%s\" dropping %s hand item", unit->agent->name.cStr(),
	           right ? "right" : "left");
}

void BattleView::orderSelect(sp<BattleUnit> u, bool inverse, bool additive)
{
	auto pos = std::find(selectedUnits.begin(), selectedUnits.end(), u);
	if (inverse)
	{
		// Unit in selection => remove
		if (pos != selectedUnits.end())
		{
			selectedUnits.erase(pos);
		}
	}
	else
	{
		// Unit not selected
		if (pos == selectedUnits.end())
		{
			if (additive)
			{
				// Unit not in selection, and not full => add unit to selection
				if (selectedUnits.size() < 6)
				{
					selectedUnits.push_front(u);
				}
			}
			else
			{
				// Unit not in selection => replace selection with unit
				selectedUnits.clear();
				selectedUnits.push_back(u);
			}
		}
		// Unit is selected
		else
		{
			// Unit in selection  => move unit to front
			if (additive || selectedUnits.size() > 1)
			{
				selectedUnits.erase(pos);
				selectedUnits.push_front(u);
			}
			// If not in additive mode and clicked on selected unit - deselect
			else
			{
				selectedUnits.clear();
			}
		}
	}
}

void BattleView::eventOccurred(Event *e)
{
	activeTab->eventOccured(e);
	baseForm->eventOccured(e);

	if (activeTab->eventIsWithin(e) || baseForm->eventIsWithin(e))
	{
		return;
	}

	if (e->type() == EVENT_KEY_DOWN &&
	    (e->keyboard().KeyCode == SDLK_ESCAPE || e->keyboard().KeyCode == SDLK_PAGEUP ||
	     e->keyboard().KeyCode == SDLK_PAGEDOWN || e->keyboard().KeyCode == SDLK_TAB ||
	     e->keyboard().KeyCode == SDLK_SPACE || e->keyboard().KeyCode == SDLK_RSHIFT ||
	     e->keyboard().KeyCode == SDLK_LSHIFT || e->keyboard().KeyCode == SDLK_RALT ||
	     e->keyboard().KeyCode == SDLK_LALT || e->keyboard().KeyCode == SDLK_RCTRL ||
	     e->keyboard().KeyCode == SDLK_LCTRL))
	{
		switch (e->keyboard().KeyCode)
		{
			case SDLK_RSHIFT:
				modifierRShift = true;
				updateSelectionMode();
				break;
			case SDLK_LSHIFT:
				modifierLShift = true;
				updateSelectionMode();
				break;
			case SDLK_RALT:
				modifierRAlt = true;
				updateSelectionMode();
				break;
			case SDLK_LALT:
				modifierLAlt = true;
				updateSelectionMode();
				break;
			case SDLK_RCTRL:
				modifierLCtrl = true;
				updateSelectionMode();
				break;
			case SDLK_LCTRL:
				modifierRCtrl = true;
				updateSelectionMode();
				break;
			case SDLK_ESCAPE:
				fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<InGameOptions>(state)});
				return;
			case SDLK_PAGEUP:
				this->setZLevel(getZLevel() + 1);
				setSelectedTilePosition(
				    {selectedTilePosition.x, selectedTilePosition.y, selectedTilePosition.z + 1});
				updateLayerButtons();
				break;
			case SDLK_PAGEDOWN:
				this->setZLevel(getZLevel() - 1);
				setSelectedTilePosition(
				    {selectedTilePosition.x, selectedTilePosition.y, selectedTilePosition.z - 1});
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
	else if (e->type() == EVENT_KEY_UP &&
	         (e->keyboard().KeyCode == SDLK_RSHIFT || e->keyboard().KeyCode == SDLK_LSHIFT ||
	          e->keyboard().KeyCode == SDLK_RALT || e->keyboard().KeyCode == SDLK_LALT ||
	          e->keyboard().KeyCode == SDLK_RCTRL || e->keyboard().KeyCode == SDLK_LCTRL))
	{
		switch (e->keyboard().KeyCode)
		{
			case SDLK_RSHIFT:
				modifierRShift = false;
				updateSelectionMode();
				break;
			case SDLK_LSHIFT:
				modifierLShift = false;
				updateSelectionMode();
				break;
			case SDLK_RALT:
				modifierRAlt = false;
				updateSelectionMode();
				break;
			case SDLK_LALT:
				modifierLAlt = false;
				updateSelectionMode();
				break;
			case SDLK_RCTRL:
				modifierLCtrl = false;
				updateSelectionMode();
				break;
			case SDLK_LCTRL:
				modifierRCtrl = false;
				updateSelectionMode();
				break;
		}
	}
	// Exclude mouse down events that are over the form
	else if (e->type() == EVENT_MOUSE_DOWN)
	{

		if (this->getViewMode() == TileViewMode::Strategy && e->type() == EVENT_MOUSE_DOWN &&
		    Event::isPressed(e->mouse().Button, Event::MouseButton::Middle))
		{
			Vec2<float> screenOffset = {this->getScreenOffset().x, this->getScreenOffset().y};
			auto clickTile = this->screenToTileCoords(
			    Vec2<float>{e->mouse().X, e->mouse().Y} - screenOffset, 0.0f);
			this->setScreenCenterTile({clickTile.x, clickTile.y});
		}

		else if (e->type() == EVENT_MOUSE_DOWN &&
		         (Event::isPressed(e->mouse().Button, Event::MouseButton::Left) ||
		          Event::isPressed(e->mouse().Button, Event::MouseButton::Right)))
		{
			// If a click has not been handled by a form it's in the map.
			auto t = this->getSelectedTilePosition();
			auto objPresent = map.getTile(t.x, t.y, t.z)->getUnitIfPresent(true);
			auto unitPresent = objPresent ? objPresent->getUnit() : nullptr;
			auto objOccupying = map.getTile(t.x, t.y, t.z)->getUnitIfPresent(true, true);
			auto unitOccupying = objOccupying ? objOccupying->getUnit() : nullptr;
			if (unitOccupying && unitOccupying->owner == state->getPlayer())
			{
				// Give priority to selecting/deselecting occupying units
				unitPresent = unitOccupying;
			}

			switch (selectionState)
			{
				case BattleSelectionState::Normal:
				case BattleSelectionState::NormalAlt:
					switch (e->mouse().Button)
					{
						case 1:
							// If unit is present, priority is to move if not occupied
							if (unitPresent && unitPresent->owner == state->getPlayer())
							{
								// Move if units are selected and noone is occupying
								if (!unitOccupying && selectedUnits.size() > 0)
								{
									orderMove(t, selectionState == BattleSelectionState::NormalAlt
									                 ? 1
									                 : 0);
								}
								// Select if friendly unit present under cursor
								else
								{
									orderSelect(unitPresent);
								}
							}
							// Move if empty
							else if (!unitOccupying)
							{
								orderMove(t);
							}
							break;
						case 4:
							// Turn if no enemy unit present under cursor
							// or if holding alt
							if (!unitPresent || unitPresent->owner == state->getPlayer() ||
							    selectionState == BattleSelectionState::NormalAlt)
							{
								orderTurn(t);
							}
							// Focus fire / fire in tb if enemy unit present
							else
							{
								// FIXME: Focus fire on enemy or fire in turn based
							}
							break;
					}
					break;
				case BattleSelectionState::NormalCtrl:
				case BattleSelectionState::NormalCtrlAlt:
				case BattleSelectionState::NormalCtrlAltShift:

					switch (e->mouse().Button)
					{
						// LMB = Add to selection
						case 1:
							if (unitPresent && unitPresent->owner == state->getPlayer())
							{
								orderSelect(unitPresent, false, true);
							}
							// If none occupying - order move if additional modifiers held
							else if (!unitOccupying &&
							         selectionState != BattleSelectionState::NormalCtrl)
							{
								// Move backwards
								if (selectionState == BattleSelectionState::NormalCtrlAlt)
								{
									orderMove(t, 2);
								}
								// Move pathing through
								else // selectionState == BattleSelectionState::NormalCtrlAltShift
								{
									orderMove(t, 0, true);
								}
							}
							break;
						// RMB = Remove from selection
						case 4:
							if (unitPresent && unitPresent->owner == state->getPlayer())
							{
								orderSelect(unitPresent, true);
							}
							break;
					}
					break;
				case BattleSelectionState::Fire:
					if (e->mouse().Button != 1 && e->mouse().Button != 4)
						break;
					// FIXME: Fire!
					break;
				case BattleSelectionState::ThrowRight:
				case BattleSelectionState::ThrowLeft:
					switch (e->mouse().Button)
					{
						case 1:
						{
							bool right = selectionState == BattleSelectionState::ThrowRight;
							orderThrow(t, right);
							break;
						}
						case 4:
						{
							selectionState = BattleSelectionState::Normal;
							break;
						}
					}
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

void BattleView::updateItemInfo(bool right)
{
	UString name = right ? "RIGHT" : "LEFT";
	AgentEquipmentInfo info = right ? rightHandInfo : leftHandInfo;
	// Item info
	if (info.itemType)
	{
		this->activeTab->findControlTyped<Graphic>("IMAGE_" + name + "_HAND")
		    ->setImage(info.itemType->equipscreen_sprite);
		if (info.damageType)
		{
			this->activeTab->findControlTyped<Graphic>("IMAGE_" + name + "_DAMAGETYPE")
			    ->setImage(info.damageType->icon_sprite);
		}
		else
		{
			this->activeTab->findControlTyped<Graphic>("IMAGE_" + name + "_DAMAGETYPE")
			    ->setImage(nullptr);
		}
	}
	else
	{
		this->activeTab->findControlTyped<Graphic>("IMAGE_" + name + "_HAND")->setImage(nullptr);
		this->activeTab->findControlTyped<Graphic>("IMAGE_" + name + "_DAMAGETYPE")
		    ->setImage(nullptr);
	}

	// Selection bracket
	if (info.selected)
	{
		this->activeTab->findControlTyped<Graphic>("IMAGE_" + name + "_HAND_SELECTED")
		    ->setImage(selectedItemOverlay);
	}
	else
	{
		this->activeTab->findControlTyped<Graphic>("IMAGE_" + name + "_HAND_SELECTED")
		    ->setImage(nullptr);
	}

	auto overlay = mksp<RGBImage>(Vec2<int>{50, 95});
	{
		RGBImageLock l(overlay);

		// Draw accuracy
		if (info.accuracy / 2 > 0)
		{
			int accuracy = info.accuracy / 2;
			int colorsCount = accuracyColors.size();
			int y = 93;
			if (right)
			{
				for (int x = 0; x < accuracy; x++)
				{
					l.set({x, y}, accuracyColors[x * colorsCount / accuracy]);
					l.set({x, y + 1}, accuracyColors[x * colorsCount / accuracy]);
				}
			}
			else
			{
				for (int x = 0; x < accuracy; x++)
				{
					l.set({50 - 1 - x, y}, accuracyColors[x * colorsCount / accuracy]);
					l.set({50 - 1 - x, y + 1}, accuracyColors[x * colorsCount / accuracy]);
				}
			}
		}
		// Draw ammo
		if (info.maxAmmo > 0 && info.curAmmo > 0)
		{
			int ammoDisplaySize = 90;

			int ammoCount = info.curAmmo;
			int ammoPadding = 1;
			int ammoSize = ammoDisplaySize / info.maxAmmo - 1;
			int x = right ? 1 : 47;
			if (ammoSize == 0)
			{
				ammoSize = 1;
				ammoPadding = 0;
				ammoCount = ammoCount * ammoDisplaySize / info.maxAmmo;
			}

			for (int i = 0; i < ammoCount; i++)
			{
				for (int j = 0; j < ammoSize; j++)
				{
					l.set({x, ammoDisplaySize - 1 - i * (ammoSize + ammoPadding) - j}, ammoColour);
					l.set({x + 1, ammoDisplaySize - 1 - i * (ammoSize + ammoPadding) - j},
					      ammoColour);
				}
			}
		}
	}
	this->activeTab->findControlTyped<Graphic>("OVERLAY_" + name + "_HAND")->setImage(overlay);
}

AgentEquipmentInfo BattleView::createItemOverlayInfo(bool rightHand)
{
	AgentEquipmentInfo a;
	if (selectedUnits.size() == 0)
	{
		return a;
	}
	auto u = *selectedUnits.begin();
	sp<AEquipment> e = nullptr;
	if (rightHand)
	{
		e = u->agent->getFirstItemInSlot(AgentEquipmentLayout::EquipmentSlotType::RightHand);
	}
	else
	{
		e = u->agent->getFirstItemInSlot(AgentEquipmentLayout::EquipmentSlotType::LeftHand);
	}
	if (e)
	{
		a.itemType = e->type;
		if (a.itemType)
		{
			auto p = e->getPayloadType();
			if (p)
			{
				a.damageType = p->damage_type;
				if (a.itemType->type == AEquipmentType::Type::Weapon)
				{
					a.maxAmmo = p->max_ammo;
					a.curAmmo = e->ammo;
				}
				// FIXME: Handle selection
				a.selected = false;
			}
			// FIXME: Grenade throw accuracy?
			if (a.itemType->type == AEquipmentType::Type::Weapon)
			{
				a.accuracy = e->getAccuracy(u->current_body_state, u->fire_aiming_mode);
			}
		}
	}
	return a;
}

bool AgentEquipmentInfo::operator==(const AgentEquipmentInfo &other) const
{
	return (this->accuracy / 2 == other.accuracy / 2 && this->curAmmo == other.curAmmo &&
	        this->itemType == other.itemType && this->damageType == other.damageType);
}

}; // namespace OpenApoc
