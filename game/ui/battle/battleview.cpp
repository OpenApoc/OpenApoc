#include "game/ui/battle/battleview.h"
#include "forms/checkbox.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/radiobutton.h"
#include "forms/scrollbar.h"
#include "forms/ticker.h"
#include "forms/tristatebox.h"
#include "forms/ui.h"
#include "framework/apocresources/cursor.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "framework/palette.h"
#include "framework/renderer.h"
#include "framework/trace.h"
#include "game/state/aequipment.h"
#include "game/state/battle/ai/ai.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battlehazard.h"
#include "game/state/battle/battleitem.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battlemappart_type.h"
#include "game/state/battle/battleunit.h"
#include "game/state/gameevent.h"
#include "game/state/gamestate.h"
#include "game/state/rules/aequipment_type.h"
#include "game/state/rules/damage.h"
#include "game/state/tileview/collision.h"
#include "game/state/tileview/tileobject_battlehazard.h"
#include "game/state/tileview/tileobject_battlemappart.h"
#include "game/state/tileview/tileobject_battleunit.h"
#include "game/ui/base/basescreen.h"
#include "game/ui/general/ingameoptions.h"
#include "library/sp.h"
#include "library/strings_format.h"
#include <cmath>
#include <glm/glm.hpp>

namespace OpenApoc
{

namespace
{
static const std::vector<UString> TAB_FORM_NAMES_RT = {
    "battle/battle_rt_tab1", "battle/battle_rt_tab2", "battle/battle_rt_tab3",
};
static const std::vector<UString> TAB_FORM_NAMES_TB = {
    "battle/battle_tb_tab1", "battle/battle_tb_tab2", "battle/battle_tb_tab3",
};
static const std::set<BodyPart> bodyParts{BodyPart::Body, BodyPart::Helmet, BodyPart::LeftArm,
                                          BodyPart::Legs, BodyPart::RightArm};

} // anonymous namespace

BattleView::BattleView(sp<GameState> gameState)
    : BattleTileView(*gameState->current_battle->map,
                     Vec3<int>{TILE_X_BATTLE, TILE_Y_BATTLE, TILE_Z_BATTLE},
                     Vec2<int>{STRAT_TILE_X, STRAT_TILE_Y}, TileViewMode::Isometric,
                     gameState->current_battle->battleViewScreenCenter, *gameState),
      baseForm(ui().getForm("battle/battle")), state(gameState), battle(*state->current_battle),
      followAgent(false), palette(fw().data->loadPalette("xcom3/tacdata/tactical.pal")),
      selectionState(BattleSelectionState::Normal)
{
	this->pal = palette;

	for (int j = 0; j <= 15; j++)
	{
		colorCurrent = j;
		auto newPal = mksp<Palette>();

		for (int i = 0; i < 255 - 4; i++)
		{
			newPal->setColour(i, palette->getColour(i));
		}
		// Lift color, pulsates from (0r 3/8g 5/8b) to (0r 8/8g 4/8b)
		newPal->setColour(255 - 4, Colour(0, (colorCurrent * 16 * 5 + 255 * 3) / 8,
		                                  (colorCurrent * 16 * -1 + 255 * 5) / 8));
		// Red color, for enemy indicators, pulsates from (3/8r 0g 0b) to (8/8r 0g 0b)
		newPal->setColour(255 - 3, Colour((colorCurrent * 16 * 5 + 255 * 3) / 8, 0, 0));
		// Blue color, for misc. indicators, pulsates from (0r 3/8g 3/8b) to (0r 8/8g 8/8b)
		newPal->setColour(255 - 2, Colour(0, (colorCurrent * 16 * 5 + 255 * 3) / 8,
		                                  (colorCurrent * 16 * 5 + 255 * 3) / 8));
		// Pink color, for neutral indicators, pulsates from (3/8r 0g 3/8b) to (8/8r 0g 8/8b)
		newPal->setColour(255 - 1, Colour((colorCurrent * 16 * 5 + 255 * 3) / 8, 0,
		                                  (colorCurrent * 16 * 5 + 255 * 3) / 8));
		// Yellow color, for owned indicators, pulsates from (3/8r 3/8g 0b) to (8/8r 8/8g 0b)
		newPal->setColour(255 - 0, Colour((colorCurrent * 16 * 5 + 255 * 3) / 8,
		                                  (colorCurrent * 16 * 5 + 255 * 3) / 8, 0));

		modPalette.push_back(newPal);
	}

	selectedItemOverlay = fw().data->loadImage("battle/battle-item-select-icon.png");
	selectedPsiOverlay = fw().data->loadImage("battle/battle-psi-select-icon.png");
	pauseIcon = fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                        "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                        260));

	for (auto &formName : TAB_FORM_NAMES_RT)
	{
		sp<Form> f(ui().getForm(formName));
		if (!f)
		{
			LogError("Failed to load form \"%s\"", formName);
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
			LogError("Failed to load form \"%s\"", formName);
			return;
		}
		f->takesFocus = false;
		this->uiTabsTB.push_back(f);
	}

	switch (battle.mode)
	{
		case Battle::Mode::RealTime:
			this->activeTab = this->uiTabsRT[0];
			this->mainTab = this->uiTabsRT[0];
			this->psiTab = this->uiTabsRT[1];
			this->primingTab = this->uiTabsRT[2];
			baseForm->findControlTyped<RadioButton>("BUTTON_SPEED0")->setChecked(true);
			updateSpeed = BattleUpdateSpeed::Pause;
			lastSpeed = BattleUpdateSpeed::Speed1;
			break;
		case Battle::Mode::TurnBased:
			this->activeTab = this->uiTabsTB[0];
			this->mainTab = this->uiTabsTB[0];
			this->psiTab = this->uiTabsTB[1];
			this->primingTab = this->uiTabsTB[2];
			baseForm->findControlTyped<RadioButton>("BUTTON_SPEED2")->setChecked(true);
			updateSpeed = BattleUpdateSpeed::Speed2;
			lastSpeed = BattleUpdateSpeed::Pause;
			break;
	}

	medikitForms[false] = ui().getForm("battle/battle_medkit_left");
	medikitForms[true] = ui().getForm("battle/battle_medkit_right");
	motionScannerForms[false] = ui().getForm("battle/battle_scanner_left");
	motionScannerForms[true] = ui().getForm("battle/battle_scanner_right");

	itemForms.push_back(medikitForms[false]);
	itemForms.push_back(medikitForms[true]);
	itemForms.push_back(motionScannerForms[false]);
	itemForms.push_back(motionScannerForms[true]);
	for (auto f : itemForms)
	{
		f->Enabled = false;
	}

	medikitBodyParts[false][BodyPart::Legs][false] =
	    medikitForms[false]->findControl("MEDIKIT_LEGS_RED");
	medikitBodyParts[false][BodyPart::Body][false] =
	    medikitForms[false]->findControl("MEDIKIT_BODY_RED");
	medikitBodyParts[false][BodyPart::LeftArm][false] =
	    medikitForms[false]->findControl("MEDIKIT_LEFT_HAND_RED");
	medikitBodyParts[false][BodyPart::RightArm][false] =
	    medikitForms[false]->findControl("MEDIKIT_RIGHT_HAND_RED");
	medikitBodyParts[false][BodyPart::Helmet][false] =
	    medikitForms[false]->findControl("MEDIKIT_HEAD_RED");
	medikitBodyParts[true][BodyPart::Legs][false] =
	    medikitForms[true]->findControl("MEDIKIT_LEGS_RED");
	medikitBodyParts[true][BodyPart::Body][false] =
	    medikitForms[true]->findControl("MEDIKIT_BODY_RED");
	medikitBodyParts[true][BodyPart::LeftArm][false] =
	    medikitForms[true]->findControl("MEDIKIT_LEFT_HAND_RED");
	medikitBodyParts[true][BodyPart::RightArm][false] =
	    medikitForms[true]->findControl("MEDIKIT_RIGHT_HAND_RED");
	medikitBodyParts[true][BodyPart::Helmet][false] =
	    medikitForms[true]->findControl("MEDIKIT_HEAD_RED");
	medikitBodyParts[false][BodyPart::Legs][true] =
	    medikitForms[false]->findControl("MEDIKIT_LEGS_GREEN");
	medikitBodyParts[false][BodyPart::Body][true] =
	    medikitForms[false]->findControl("MEDIKIT_BODY_GREEN");
	medikitBodyParts[false][BodyPart::LeftArm][true] =
	    medikitForms[false]->findControl("MEDIKIT_LEFT_HAND_GREEN");
	medikitBodyParts[false][BodyPart::RightArm][true] =
	    medikitForms[false]->findControl("MEDIKIT_RIGHT_HAND_GREEN");
	medikitBodyParts[false][BodyPart::Helmet][true] =
	    medikitForms[false]->findControl("MEDIKIT_HEAD_GREEN");
	medikitBodyParts[true][BodyPart::Legs][true] =
	    medikitForms[true]->findControl("MEDIKIT_LEGS_GREEN");
	medikitBodyParts[true][BodyPart::Body][true] =
	    medikitForms[true]->findControl("MEDIKIT_BODY_GREEN");
	medikitBodyParts[true][BodyPart::LeftArm][true] =
	    medikitForms[true]->findControl("MEDIKIT_LEFT_HAND_GREEN");
	medikitBodyParts[true][BodyPart::RightArm][true] =
	    medikitForms[true]->findControl("MEDIKIT_RIGHT_HAND_GREEN");
	medikitBodyParts[true][BodyPart::Helmet][true] =
	    medikitForms[true]->findControl("MEDIKIT_HEAD_GREEN");

	std::function<void(FormsEvent * e)> medikitButtonHead = [this](Event *) {
		orderHeal(BodyPart::Helmet);
	};
	std::function<void(FormsEvent * e)> medikitButtonBody = [this](Event *) {
		orderHeal(BodyPart::Body);
	};
	std::function<void(FormsEvent * e)> medikitButtonLeftHand = [this](Event *) {
		orderHeal(BodyPart::LeftArm);
	};
	std::function<void(FormsEvent * e)> medikitButtonRightHand = [this](Event *) {
		orderHeal(BodyPart::RightArm);
	};
	std::function<void(FormsEvent * e)> medikitButtonLegs = [this](Event *) {
		orderHeal(BodyPart::Legs);
	};
	medikitForms[false]
	    ->findControlTyped<GraphicButton>("MEDIKIT_HEAD_BUTTON")
	    ->addCallback(FormEventType::ButtonClick, medikitButtonHead);
	medikitForms[false]
	    ->findControlTyped<GraphicButton>("MEDIKIT_BODY_BUTTON")
	    ->addCallback(FormEventType::ButtonClick, medikitButtonBody);
	medikitForms[false]
	    ->findControlTyped<GraphicButton>("MEDIKIT_LEFT_HAND_BUTTON")
	    ->addCallback(FormEventType::ButtonClick, medikitButtonLeftHand);
	medikitForms[false]
	    ->findControlTyped<GraphicButton>("MEDIKIT_RIGHT_HAND_BUTTON")
	    ->addCallback(FormEventType::ButtonClick, medikitButtonRightHand);
	medikitForms[false]
	    ->findControlTyped<GraphicButton>("MEDIKIT_LEGS_BUTTON")
	    ->addCallback(FormEventType::ButtonClick, medikitButtonLegs);
	medikitForms[true]
	    ->findControlTyped<GraphicButton>("MEDIKIT_HEAD_BUTTON")
	    ->addCallback(FormEventType::ButtonClick, medikitButtonHead);
	medikitForms[true]
	    ->findControlTyped<GraphicButton>("MEDIKIT_BODY_BUTTON")
	    ->addCallback(FormEventType::ButtonClick, medikitButtonBody);
	medikitForms[true]
	    ->findControlTyped<GraphicButton>("MEDIKIT_LEFT_HAND_BUTTON")
	    ->addCallback(FormEventType::ButtonClick, medikitButtonLeftHand);
	medikitForms[true]
	    ->findControlTyped<GraphicButton>("MEDIKIT_RIGHT_HAND_BUTTON")
	    ->addCallback(FormEventType::ButtonClick, medikitButtonRightHand);
	medikitForms[true]
	    ->findControlTyped<GraphicButton>("MEDIKIT_LEGS_BUTTON")
	    ->addCallback(FormEventType::ButtonClick, medikitButtonLegs);
	medikitForms[false]
	    ->findControlTyped<GraphicButton>("MEDIKIT_HEAD_BUTTON")
	    ->setClickSound(nullptr);
	medikitForms[false]
	    ->findControlTyped<GraphicButton>("MEDIKIT_BODY_BUTTON")
	    ->setClickSound(nullptr);
	medikitForms[false]
	    ->findControlTyped<GraphicButton>("MEDIKIT_LEFT_HAND_BUTTON")
	    ->setClickSound(nullptr);
	medikitForms[false]
	    ->findControlTyped<GraphicButton>("MEDIKIT_RIGHT_HAND_BUTTON")
	    ->setClickSound(nullptr);
	medikitForms[false]
	    ->findControlTyped<GraphicButton>("MEDIKIT_LEGS_BUTTON")
	    ->setClickSound(nullptr);
	medikitForms[true]
	    ->findControlTyped<GraphicButton>("MEDIKIT_HEAD_BUTTON")
	    ->setClickSound(nullptr);
	medikitForms[true]
	    ->findControlTyped<GraphicButton>("MEDIKIT_BODY_BUTTON")
	    ->setClickSound(nullptr);
	medikitForms[true]
	    ->findControlTyped<GraphicButton>("MEDIKIT_LEFT_HAND_BUTTON")
	    ->setClickSound(nullptr);
	medikitForms[true]
	    ->findControlTyped<GraphicButton>("MEDIKIT_RIGHT_HAND_BUTTON")
	    ->setClickSound(nullptr);
	medikitForms[true]
	    ->findControlTyped<GraphicButton>("MEDIKIT_LEGS_BUTTON")
	    ->setClickSound(nullptr);

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
		    for (auto u : this->battle.battleViewSelectedUnits)
		    {
			    if (u->fire_permission_mode == BattleUnit::FirePermissionMode::AtWill)
			    {
				    at_will = true;
			    }
		    }
		    for (auto u : this->battle.battleViewSelectedUnits)
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
		    for (auto u : this->battle.battleViewSelectedUnits)
		    {
			    u->fire_aiming_mode = WeaponAimingMode::Aimed;
		    }
		});
	this->baseForm->findControl("BUTTON_SNAP")
	    ->addCallback(FormEventType::MouseClick, [this](Event *) {
		    for (auto u : this->battle.battleViewSelectedUnits)
		    {
			    u->fire_aiming_mode = WeaponAimingMode::Snap;
		    }
		});
	this->baseForm->findControl("BUTTON_AUTO")
	    ->addCallback(FormEventType::MouseClick, [this](Event *) {
		    for (auto u : this->battle.battleViewSelectedUnits)
		    {
			    u->fire_aiming_mode = WeaponAimingMode::Auto;
		    }
		});
	this->baseForm->findControl("BUTTON_KNEEL")
	    ->addCallback(FormEventType::MouseClick, [this](Event *) {
		    bool not_kneeling = false;

		    for (auto u : this->battle.battleViewSelectedUnits)
		    {
			    if (u->kneeling_mode == KneelingMode::None &&
			        u->agent->isBodyStateAllowed(BodyState::Kneeling))
			    {
				    not_kneeling = true;
			    }
		    }
		    for (auto u : this->battle.battleViewSelectedUnits)
		    {
			    if (not_kneeling)
			    {
				    u->kneeling_mode = KneelingMode::Kneeling;
			    }
			    else
			    {
				    u->kneeling_mode = KneelingMode::None;
			    }
		    }
		});
	this->baseForm->findControl("BUTTON_PRONE")
	    ->addCallback(FormEventType::MouseClick, [this](Event *) {
		    for (auto u : this->battle.battleViewSelectedUnits)
		    {
			    u->setMovementMode(MovementMode::Prone);
		    }
		});
	this->baseForm->findControl("BUTTON_WALK")
	    ->addCallback(FormEventType::MouseClick, [this](Event *) {
		    for (auto u : this->battle.battleViewSelectedUnits)
		    {
			    u->setMovementMode(MovementMode::Walking);
		    }
		});
	this->baseForm->findControl("BUTTON_RUN")
	    ->addCallback(FormEventType::MouseClick, [this](Event *) {
		    for (auto u : this->battle.battleViewSelectedUnits)
		    {
			    u->setMovementMode(MovementMode::Running);
		    }
		});
	this->baseForm->findControl("BUTTON_EVASIVE")
	    ->addCallback(FormEventType::MouseClick, [this](Event *) {
		    for (auto u : this->battle.battleViewSelectedUnits)
		    {
			    u->behavior_mode = BattleUnit::BehaviorMode::Evasive;
		    }
		});
	this->baseForm->findControl("BUTTON_NORMAL")
	    ->addCallback(FormEventType::MouseClick, [this](Event *) {
		    for (auto u : this->battle.battleViewSelectedUnits)
		    {
			    u->behavior_mode = BattleUnit::BehaviorMode::Normal;
		    }
		});
	this->baseForm->findControl("BUTTON_AGGRESSIVE")
	    ->addCallback(FormEventType::MouseClick, [this](Event *) {
		    for (auto u : this->battle.battleViewSelectedUnits)
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

	this->baseForm->findControl("BUTTON_MOVE_GROUP")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *) { this->battle.battleViewGroupMove = true; });
	this->baseForm->findControl("BUTTON_MOVE_INDIVIDUALLY")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *) { this->battle.battleViewGroupMove = false; });
	if (battle.battleViewGroupMove)
	{
		this->baseForm->findControlTyped<RadioButton>("BUTTON_MOVE_GROUP")->setChecked(true);
	}
	else
	{
		this->baseForm->findControlTyped<RadioButton>("BUTTON_MOVE_INDIVIDUALLY")->setChecked(true);
	}

	this->uiTabsRT[0]
	    ->findControl("BUTTON_LAYER_1")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { this->setZLevel(1); });
	this->uiTabsRT[0]
	    ->findControl("BUTTON_LAYER_2")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { this->setZLevel(2); });
	this->uiTabsRT[0]
	    ->findControl("BUTTON_LAYER_3")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { this->setZLevel(3); });
	this->uiTabsRT[0]
	    ->findControl("BUTTON_LAYER_4")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { this->setZLevel(4); });
	this->uiTabsRT[0]
	    ->findControl("BUTTON_LAYER_5")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { this->setZLevel(5); });
	this->uiTabsRT[0]
	    ->findControl("BUTTON_LAYER_6")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { this->setZLevel(6); });
	this->uiTabsRT[0]
	    ->findControl("BUTTON_LAYER_7")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { this->setZLevel(7); });
	this->uiTabsRT[0]
	    ->findControl("BUTTON_LAYER_8")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { this->setZLevel(8); });
	this->uiTabsRT[0]
	    ->findControl("BUTTON_LAYER_9")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { this->setZLevel(9); });
	this->uiTabsTB[0]
	    ->findControl("BUTTON_LAYER_1")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { this->setZLevel(1); });
	this->uiTabsTB[0]
	    ->findControl("BUTTON_LAYER_2")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { this->setZLevel(2); });
	this->uiTabsTB[0]
	    ->findControl("BUTTON_LAYER_3")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { this->setZLevel(3); });
	this->uiTabsTB[0]
	    ->findControl("BUTTON_LAYER_4")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { this->setZLevel(4); });
	this->uiTabsTB[0]
	    ->findControl("BUTTON_LAYER_5")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { this->setZLevel(5); });
	this->uiTabsTB[0]
	    ->findControl("BUTTON_LAYER_6")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { this->setZLevel(6); });
	this->uiTabsTB[0]
	    ->findControl("BUTTON_LAYER_7")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { this->setZLevel(7); });
	this->uiTabsTB[0]
	    ->findControl("BUTTON_LAYER_8")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { this->setZLevel(8); });
	this->uiTabsTB[0]
	    ->findControl("BUTTON_LAYER_9")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { this->setZLevel(9); });
	this->baseForm->findControl("BUTTON_FOLLOW_AGENT")
	    ->addCallback(FormEventType::CheckBoxChange, [this](Event *e) {
		    this->followAgent =
		        std::dynamic_pointer_cast<CheckBox>(e->forms().RaisedBy)->isChecked();
		});

	this->baseForm->findControl("BUTTON_SHOW_OPTIONS")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    fw().stageQueueCommand(
		        {StageCmd::Command::PUSH, mksp<InGameOptions>(this->state->shared_from_this())});
		});
	this->baseForm->findControl("BUTTON_SHOW_LOG")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) { LogWarning("Show log"); });
	this->baseForm->findControl("BUTTON_ZOOM_EVENT")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) { LogWarning("Zoom to event"); });

	// FIXME: When clicking on items or weapons, activate them or go into fire / teleport mode
	// accordingly

	std::function<void(FormsEvent * e)> clickedRightHand = [this](FormsEvent *e) {
		if (Event::isPressed(e->forms().MouseInfo.Button, Event::MouseButton::Right))
			orderUse(true, true);
		else if (Event::isPressed(e->forms().MouseInfo.Button, Event::MouseButton::Left))
			orderUse(true, false);
	};

	std::function<void(FormsEvent * e)> clickedLeftHand = [this](FormsEvent *e) {
		if (Event::isPressed(e->forms().MouseInfo.Button, Event::MouseButton::Right))
			orderUse(false, true);
		else if (Event::isPressed(e->forms().MouseInfo.Button, Event::MouseButton::Left))
			orderUse(false, false);
	};

	std::function<void(FormsEvent * e)> dropRightHand = [this](Event *) { orderDrop(true); };

	std::function<void(FormsEvent * e)> dropLeftHand = [this](Event *) { orderDrop(false); };

	std::function<void(FormsEvent * e)> cancelThrow = [this](Event *) {
		this->activeTab = this->mainTab;
	};

	std::function<void(bool right)> throwItem = [this](bool right) {
		bool fail = false;
		if (this->battle.battleViewSelectedUnits.size() == 0)
		{
			fail = true;
		}
		else
		{
			auto unit = this->battle.battleViewSelectedUnits.front();
			if (!(unit->agent->getFirstItemInSlot(right ? AEquipmentSlotType::RightHand
			                                            : AEquipmentSlotType::LeftHand)) ||
			    !unit->agent->type->inventory)
			{
				fail = true;
			}
		}
		if (fail)
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
		else
		{
			this->selectionState =
			    right ? BattleSelectionState::ThrowRight : BattleSelectionState::ThrowLeft;
		}
	};

	std::function<void(FormsEvent * e)> throwRightHand = [this, throwItem](Event *) {
		throwItem(true);
	};

	std::function<void(FormsEvent * e)> throwLeftHand = [this, throwItem](Event *) {
		throwItem(false);
	};

	std::function<void(FormsEvent * e)> finishPriming = [this, throwItem](Event *) {
		bool right =
		    this->primingTab->findControlTyped<CheckBox>("HIDDEN_CHECK_RIGHT_HAND")->isChecked();
		auto unit = this->battle.battleViewSelectedUnits.front();
		auto item = unit->agent->getFirstItemInSlot(right ? AEquipmentSlotType::RightHand
		                                                  : AEquipmentSlotType::LeftHand);

		int delay = this->primingTab->findControlTyped<ScrollBar>("DELAY_SLIDER")->getValue();
		LogWarning("Delay %d", delay);
		int range = this->primingTab->findControlTyped<ScrollBar>("RANGE_SLIDER")->getValue();
		if (delay == 0 && (item->type->trigger_type != TriggerType::Boomeroid ||
		                   item->type->trigger_type != TriggerType::Proximity))
		{
			item->prime();
		}
		else
		{
			item->prime(false, delay * TICKS_PER_SECOND / 4, (range + 1) * 6);
		}
		this->activeTab = this->mainTab;
	};

	std::function<void(FormsEvent * e)> updateDelay = [this, throwItem](Event *) {
		int delay = this->primingTab->findControlTyped<ScrollBar>("DELAY_SLIDER")->getValue();
		LogWarning("Delay %d", delay);
		UString text;
		if (delay == 0)
			text = format(tr("Activates now."));
		else
			text = format(tr("Delay = %i"), (int)((float)delay / 4.0f));
		this->primingTab->findControlTyped<Label>("DELAY_TEXT")->setText(text);
	};

	std::function<void(FormsEvent * e)> updateRange = [this, throwItem](Event *) {
		int range = this->primingTab->findControlTyped<ScrollBar>("RANGE_SLIDER")->getValue();

		UString text = format(tr("Range = %2.1fm."), ((float)(range + 1) * 1.5f));
		this->primingTab->findControlTyped<Label>("RANGE_TEXT")->setText(text);
	};

	// Priming controls

	this->uiTabsRT[2]
	    ->findControlTyped<CheckBox>("HIDDEN_CHECK_RIGHT_HAND")
	    ->setClickSound(nullptr);
	this->uiTabsRT[2]->findControlTyped<CheckBox>("HIDDEN_CHECK_RIGHT_HAND")->Enabled = false;
	this->uiTabsRT[2]
	    ->findControlTyped<GraphicButton>("BUTTON_CANCEL")
	    ->addCallback(FormEventType::ButtonClick, cancelThrow);
	this->uiTabsRT[2]
	    ->findControlTyped<GraphicButton>("BUTTON_OK")
	    ->addCallback(FormEventType::ButtonClick, finishPriming);
	this->uiTabsRT[2]
	    ->findControlTyped<ScrollBar>("DELAY_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, updateDelay);
	this->uiTabsRT[2]
	    ->findControlTyped<ScrollBar>("RANGE_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, updateRange);
	this->uiTabsRT[2]
	    ->findControlTyped<CheckBox>("HIDDEN_CHECK_RIGHT_HAND")
	    ->setClickSound(nullptr);
	this->uiTabsTB[2]->findControlTyped<CheckBox>("HIDDEN_CHECK_RIGHT_HAND")->Enabled = false;
	this->uiTabsTB[2]
	    ->findControlTyped<GraphicButton>("BUTTON_CANCEL")
	    ->addCallback(FormEventType::ButtonClick, cancelThrow);
	this->uiTabsTB[2]
	    ->findControlTyped<GraphicButton>("BUTTON_OK")
	    ->addCallback(FormEventType::ButtonClick, finishPriming);
	this->uiTabsTB[2]
	    ->findControlTyped<ScrollBar>("DELAY_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, updateDelay);
	this->uiTabsTB[2]
	    ->findControlTyped<ScrollBar>("RANGE_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, updateRange);

	std::function<void(FormsEvent * e)> psiControl = [this](Event *) {
		bool control = this->psiInfo.status == PsiStatus::Control;
		this->orderCancelPsi();
		if (control)
		{
			this->state->current_battle->updateVision(*this->state);
		}
		this->selectionState = BattleSelectionState::PsiControl;
	};
	std::function<void(FormsEvent * e)> psiPanic = [this](Event *) {
		bool control = this->psiInfo.status == PsiStatus::Control;
		this->orderCancelPsi();
		if (control)
		{
			this->state->current_battle->updateVision(*this->state);
		}
		this->selectionState = BattleSelectionState::PsiPanic;
	};
	std::function<void(FormsEvent * e)> psiStun = [this](Event *) {
		bool control = this->psiInfo.status == PsiStatus::Control;
		this->orderCancelPsi();
		if (control)
		{
			this->state->current_battle->updateVision(*this->state);
		}
		this->selectionState = BattleSelectionState::PsiStun;
	};
	std::function<void(FormsEvent * e)> psiProbe = [this](Event *) {
		bool control = this->psiInfo.status == PsiStatus::Control;
		this->orderCancelPsi();
		if (control)
		{
			this->state->current_battle->updateVision(*this->state);
		}
		this->selectionState = BattleSelectionState::PsiProbe;
	};

	std::function<void(FormsEvent * e)> cancelPsi = [this](Event *) {
		this->activeTab = this->mainTab;
		this->selectionState = BattleSelectionState::Normal;
	};

	// Psi controls

	this->uiTabsRT[1]
	    ->findControlTyped<GraphicButton>("BUTTON_CANCEL")
	    ->addCallback(FormEventType::ButtonClick, cancelPsi);
	this->uiTabsRT[1]
	    ->findControlTyped<RadioButton>("BUTTON_CONTROL")
	    ->addCallback(FormEventType::MouseClick, psiControl);
	this->uiTabsRT[1]
	    ->findControlTyped<RadioButton>("BUTTON_PANIC")
	    ->addCallback(FormEventType::MouseClick, psiPanic);
	this->uiTabsRT[1]
	    ->findControlTyped<RadioButton>("BUTTON_STUN")
	    ->addCallback(FormEventType::MouseClick, psiStun);
	this->uiTabsRT[1]
	    ->findControlTyped<RadioButton>("BUTTON_PROBE")
	    ->addCallback(FormEventType::MouseClick, psiProbe);
	this->uiTabsTB[1]
	    ->findControlTyped<GraphicButton>("BUTTON_CANCEL")
	    ->addCallback(FormEventType::ButtonClick, cancelPsi);
	this->uiTabsTB[1]
	    ->findControlTyped<RadioButton>("BUTTON_CONTROL")
	    ->addCallback(FormEventType::MouseClick, psiControl);
	this->uiTabsTB[1]
	    ->findControlTyped<RadioButton>("BUTTON_PANIC")
	    ->addCallback(FormEventType::MouseClick, psiPanic);
	this->uiTabsTB[1]
	    ->findControlTyped<RadioButton>("BUTTON_STUN")
	    ->addCallback(FormEventType::MouseClick, psiStun);
	this->uiTabsTB[1]
	    ->findControlTyped<RadioButton>("BUTTON_PROBE")
	    ->addCallback(FormEventType::MouseClick, psiProbe);

	// Hand controls

	this->uiTabsRT[0]
	    ->findControlTyped<Graphic>("OVERLAY_RIGHT_HAND")
	    ->addCallback(FormEventType::MouseDown, clickedRightHand);
	this->uiTabsRT[0]
	    ->findControlTyped<Graphic>("OVERLAY_LEFT_HAND")
	    ->addCallback(FormEventType::MouseDown, clickedLeftHand);
	this->uiTabsTB[0]
	    ->findControlTyped<Graphic>("OVERLAY_RIGHT_HAND")
	    ->addCallback(FormEventType::MouseDown, clickedRightHand);
	this->uiTabsTB[0]
	    ->findControlTyped<Graphic>("OVERLAY_LEFT_HAND")
	    ->addCallback(FormEventType::MouseDown, clickedLeftHand);
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

	// We need this in TB because we will be able to allow pausing then
	this->baseForm->findControl("BUTTON_SPEED0")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    this->lastSpeed = this->updateSpeed;
		    this->updateSpeed = BattleUpdateSpeed::Pause;
		});
	this->baseForm->findControl("BUTTON_SPEED1")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->updateSpeed = BattleUpdateSpeed::Speed1; });
	this->baseForm->findControl("BUTTON_SPEED2")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->updateSpeed = BattleUpdateSpeed::Speed2; });
	this->baseForm->findControl("BUTTON_SPEED3")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->updateSpeed = BattleUpdateSpeed::Speed3; });

	switch (battle.mode)
	{
		case Battle::Mode::RealTime:
			this->baseForm->findControl("BUTTON_ENDTURN")->setVisible(false);
			break;
		case Battle::Mode::TurnBased:
			this->baseForm->findControl("BUTTON_SPEED0")->setVisible(false);
			this->baseForm->findControl("BUTTON_SPEED1")->setVisible(false);
			this->baseForm->findControl("BUTTON_SPEED2")->setVisible(false);
			this->baseForm->findControl("BUTTON_SPEED3")->setVisible(false);
			this->baseForm->findControl("CLOCK")->setVisible(false);
			this->baseForm->findControl("BUTTON_ENDTURN")
			    ->addCallback(FormEventType::ButtonClick,
			                  [this](Event *) { this->battle.endTurn(); });
			break;
	}

	updateLayerButtons();
}

BattleView::~BattleView() = default;

void BattleView::begin()
{
	this->uiTabsRT[0]->findControl("BUTTON_LAYER_1")->setVisible(maxZDraw >= 1);
	this->uiTabsRT[0]->findControl("BUTTON_LAYER_2")->setVisible(maxZDraw >= 2);
	this->uiTabsRT[0]->findControl("BUTTON_LAYER_3")->setVisible(maxZDraw >= 3);
	this->uiTabsRT[0]->findControl("BUTTON_LAYER_4")->setVisible(maxZDraw >= 4);
	this->uiTabsRT[0]->findControl("BUTTON_LAYER_5")->setVisible(maxZDraw >= 5);
	this->uiTabsRT[0]->findControl("BUTTON_LAYER_6")->setVisible(maxZDraw >= 6);
	this->uiTabsRT[0]->findControl("BUTTON_LAYER_7")->setVisible(maxZDraw >= 7);
	this->uiTabsRT[0]->findControl("BUTTON_LAYER_8")->setVisible(maxZDraw >= 8);
	this->uiTabsRT[0]->findControl("BUTTON_LAYER_9")->setVisible(maxZDraw >= 9);
	this->uiTabsTB[0]->findControl("BUTTON_LAYER_1")->setVisible(maxZDraw >= 1);
	this->uiTabsTB[0]->findControl("BUTTON_LAYER_2")->setVisible(maxZDraw >= 2);
	this->uiTabsTB[0]->findControl("BUTTON_LAYER_3")->setVisible(maxZDraw >= 3);
	this->uiTabsTB[0]->findControl("BUTTON_LAYER_4")->setVisible(maxZDraw >= 4);
	this->uiTabsTB[0]->findControl("BUTTON_LAYER_5")->setVisible(maxZDraw >= 5);
	this->uiTabsTB[0]->findControl("BUTTON_LAYER_6")->setVisible(maxZDraw >= 6);
	this->uiTabsTB[0]->findControl("BUTTON_LAYER_7")->setVisible(maxZDraw >= 7);
	this->uiTabsTB[0]->findControl("BUTTON_LAYER_8")->setVisible(maxZDraw >= 8);
	this->uiTabsTB[0]->findControl("BUTTON_LAYER_9")->setVisible(maxZDraw >= 9);

	if (battle.mode == Battle::Mode::TurnBased)
		onNewTurn();
}

void BattleView::resume() {}

void BattleView::render()
{
	TRACE_FN;

	BattleTileView::render();
	activeTab->render();
	baseForm->render();

	int pauseIconOffsetX = 0;

	// Item forms
	for (auto f : itemForms)
	{
		if (f->Enabled)
		{
			f->render();
			if (f == motionScannerForms[false] || f == medikitForms[false])
			{
				pauseIconOffsetX = f->Size.x;
			}
		}
	}
	// Pause icon
	if (battle.mode == Battle::Mode::TurnBased)
	{
		int PAUSE_ICON_BLINK_TIME = 30;
		pauseIconTimer++;
		pauseIconTimer %= PAUSE_ICON_BLINK_TIME * 2;
		if (updateSpeed == BattleUpdateSpeed::Pause && pauseIconTimer > PAUSE_ICON_BLINK_TIME)
		{
			fw().renderer->draw(
			    pauseIcon, {fw().displayGetSize().x - pauseIconOffsetX - pauseIcon->size.x, 0.0f});
		}
	}

	// If there's a modal dialog, darken the screen
	if (fw().stageGetCurrent() != this->shared_from_this())
	{
		fw().renderer->drawFilledRect({0, 0}, fw().displayGetSize(), Colour{0, 0, 0, 128});
	}
}

void BattleView::setUpdateSpeed(BattleUpdateSpeed updateSpeed)
{
	if (this->updateSpeed == updateSpeed)
	{
		return;
	}
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
	BattleTileView::update();
	// FIXME: Is there a more efficient way? But TileView does not know about battle or state!
	battle.battleViewScreenCenter = centerPos;

	updateSelectedUnits();
	updateSelectionMode();
	updateSoldierButtons();

	if (ticksUntilFireSound > 0)
	{
		ticksUntilFireSound--;
	}

	if (battle.mode == Battle::Mode::TurnBased)
	{
		if (previewedPathCost == -1)
		{
			pathPreviewTicksAccumulated++;
			// Show path preview if hovering for over half a second
			if (pathPreviewTicksAccumulated > 30)
			{
				updatePathPreview();
			}
		}
	}

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
	if (battle.mode == Battle::Mode::RealTime)
	{
		auto clockControl = baseForm->findControlTyped<Label>("CLOCK");
		clockControl->setText(state->gameTime.getTimeString());
	}

	// Pulsate palette colors

	colorCurrent += (colorForward ? 1 : -1);
	if (colorCurrent <= 0 || colorCurrent >= 15)
	{
		colorCurrent = clamp(colorCurrent, 0, 15);
		colorForward = !colorForward;
	}
	this->pal = modPalette[colorCurrent];

	// Update weapons if required

	auto rightInfo = createItemOverlayInfo(true);
	if (rightInfo != rightHandInfo &&
	    (this->activeTab == uiTabsRT[0] || this->activeTab == uiTabsTB[0]))
	{
		rightHandInfo = rightInfo;
		updateItemInfo(true);
	}
	auto leftInfo = createItemOverlayInfo(false);
	if (leftInfo != leftHandInfo &&
	    (this->activeTab == uiTabsRT[0] || this->activeTab == uiTabsTB[0]))
	{
		leftHandInfo = leftInfo;
		updateItemInfo(false);
	}

	// Update item forms

	for (auto f : itemForms)
	{
		f->Enabled = false;
	}
	if (leftHandInfo.selected || rightHandInfo.selected)
	{
		auto unit = battle.battleViewSelectedUnits.front();
		for (int i = 0; i < 2; i++)
		{
			bool right = i == 0;
			if (right ? !rightHandInfo.selected : !leftHandInfo.selected)
			{
				continue;
			}
			auto item = unit->agent->getFirstItemInSlot(right ? AEquipmentSlotType::RightHand
			                                                  : AEquipmentSlotType::LeftHand);
			if (!item->inUse)
			{
				continue;
			}
			switch (item->type->type)
			{
				case AEquipmentType::Type::MotionScanner:
					motionScannerForms[right]->Enabled = true;
					break;
				case AEquipmentType::Type::MediKit:
					medikitForms[right]->Enabled = true;
					for (auto c : medikitBodyParts[right])
					{
						c.second[false]->setVisible(false);
						c.second[true]->setVisible(false);
					}
					for (auto p : bodyParts)
					{
						if (unit->fatalWounds[p] > 0)
						{
							medikitBodyParts[right][p]
							                [unit->isHealing && p == unit->healingBodyPart]
							                    ->setVisible(true);
						}
					}
					break;
				default:
					LogError("Using an item other than the motion scanner / medikit?");
			}
		}
	}

	// Update psi
	if (this->activeTab == psiTab)
	{
		auto newPsiInfo = createPsiInfo();
		if (psiInfo != newPsiInfo)
		{
			psiInfo = newPsiInfo;
			updatePsiInfo();
		}
	}

	// Call forms->update()
	for (auto f : itemForms)
	{
		if (f->Enabled)
		{
			f->update();
		}
	}
	activeTab->update();
	baseForm->update();

	// If we have 'follow agent' enabled we clobber any other movement that may have occurred in
	// this frame
	if (this->followAgent)
	{
		if (battle.battleViewSelectedUnits.size() > 0)
		{
			setScreenCenterTile(battle.battleViewSelectedUnits.front()->tileObject->getPosition());
		}
	}
}

void BattleView::updateSelectedUnits()
{
	auto prevLSU = lastSelectedUnit;
	auto it = battle.battleViewSelectedUnits.begin();
	auto o = battle.currentPlayer;
	while (it != battle.battleViewSelectedUnits.end())
	{
		auto u = *it;
		if (!u || u->isDead() || u->isUnconscious() || u->owner != o || u->retreated ||
		    u->moraleState != MoraleState::Normal)
		{
			it = battle.battleViewSelectedUnits.erase(it);
		}
		else
		{
			it++;
		}
	}
	lastSelectedUnit = battle.battleViewSelectedUnits.size() == 0
	                       ? nullptr
	                       : battle.battleViewSelectedUnits.front();

	// Cancel stuff that cancels on unit change
	if (prevLSU != lastSelectedUnit)
	{
		if (selectionState == BattleSelectionState::ThrowLeft ||
		    selectionState == BattleSelectionState::ThrowRight)
		{
			selectionState = BattleSelectionState::Normal;
		}
	}
	if (prevLSU != lastSelectedUnit || !lastSelectedUnit)
	{
		resetPathPreview();
		switch (battle.mode)
		{
			case Battle::Mode::RealTime:
				this->activeTab = this->uiTabsRT[0];
				break;
			case Battle::Mode::TurnBased:
				this->activeTab = this->uiTabsTB[0];
				break;
		}
	}
	else
	{
		auto p = lastSelectedUnit->tileObject->getOwningTile()->position;
		if (lastSelectedUnitPosition != p)
		{
			resetPathPreview();
		}
		lastSelectedUnitPosition = p;
	}
}

void BattleView::updateSelectionMode()
{
	// FIXME: Add Psi in the mix
	// FIXME: Change cursor
	if (battle.battleViewSelectedUnits.size() == 0)
	{
		if (modifierLCtrl || modifierRCtrl)
		{
			if (modifierLAlt || modifierRAlt)
			{
				selectionState = BattleSelectionState::NormalCtrlAlt;
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
		if (selectionState == BattleSelectionState::ThrowLeft ||
		    selectionState == BattleSelectionState::ThrowRight ||
		    selectionState == BattleSelectionState::TeleportLeft ||
		    selectionState == BattleSelectionState::TeleportRight ||
		    selectionState == BattleSelectionState::PsiControl ||
		    selectionState == BattleSelectionState::PsiPanic ||
		    selectionState == BattleSelectionState::PsiStun ||
		    selectionState == BattleSelectionState::PsiProbe ||
		    selectionState == BattleSelectionState::FireLeft ||
		    selectionState == BattleSelectionState::FireRight)
		{
			// Cannot cancel out of action here
		}
		else if (modifierLCtrl || modifierRCtrl)
		{
			if (modifierLAlt || modifierRAlt)
			{
				selectionState = BattleSelectionState::NormalCtrlAlt;
			}
			else
			{
				selectionState = BattleSelectionState::NormalCtrl;
			}
		}
		else if (modifierLShift || modifierRShift)
		{
			selectionState = BattleSelectionState::FireAny;
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
	// Reset path preview
	switch (selectionState)
	{
		case BattleSelectionState::FireAny:
		case BattleSelectionState::FireLeft:
		case BattleSelectionState::FireRight:
		case BattleSelectionState::ThrowLeft:
		case BattleSelectionState::ThrowRight:
		case BattleSelectionState::PsiControl:
		case BattleSelectionState::PsiPanic:
		case BattleSelectionState::PsiStun:
		case BattleSelectionState::PsiProbe:
		case BattleSelectionState::TeleportLeft:
		case BattleSelectionState::TeleportRight:
			resetPathPreview();
			break;
		case BattleSelectionState::Normal:
		case BattleSelectionState::NormalAlt:
		case BattleSelectionState::NormalCtrl:
		case BattleSelectionState::NormalCtrlAlt:
			// Fine, don't need to
			break;
	}
	// Change cursor
	bool psiCursor = false;
	switch (selectionState)
	{
		case BattleSelectionState::FireAny:
		case BattleSelectionState::FireLeft:
		case BattleSelectionState::FireRight:
			fw().getCursor().CurrentType = ApocCursor::CursorType::Shoot;
			break;
		case BattleSelectionState::ThrowLeft:
		case BattleSelectionState::ThrowRight:
			if (actionImpossibleDelay > 0)
			{
				fw().getCursor().CurrentType = ApocCursor::CursorType::NoTarget;
			}
			else
			{
				fw().getCursor().CurrentType = ApocCursor::CursorType::ThrowTarget;
			}
			break;
		case BattleSelectionState::PsiControl:
		case BattleSelectionState::PsiPanic:
		case BattleSelectionState::PsiStun:
		case BattleSelectionState::PsiProbe:
			fw().getCursor().CurrentType = ApocCursor::CursorType::PsiTarget;
			psiCursor = true;
			break;
		case BattleSelectionState::Normal:
		case BattleSelectionState::NormalAlt:
			fw().getCursor().CurrentType = ApocCursor::CursorType::Normal;
			break;
		case BattleSelectionState::NormalCtrl:
		case BattleSelectionState::NormalCtrlAlt:
			fw().getCursor().CurrentType = ApocCursor::CursorType::Add;
			break;
		case BattleSelectionState::TeleportLeft:
		case BattleSelectionState::TeleportRight:
			if (actionImpossibleDelay > 0)
			{
				fw().getCursor().CurrentType = ApocCursor::CursorType::NoTeleport;
			}
			else
			{
				fw().getCursor().CurrentType = ApocCursor::CursorType::Teleport;
			}
			break;
	}
	actionImpossibleDelay--;
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

	for (auto u : battle.battleViewSelectedUnits)
	{
		switch (u->fire_aiming_mode)
		{
			case WeaponAimingMode::Aimed:
				aimed = true;
				break;
			case WeaponAimingMode::Snap:
				snap = true;
				break;
			case WeaponAimingMode::Auto:
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
			case MovementMode::Prone:
				prone = true;
				break;
			case MovementMode::Walking:
				walk = true;
				break;
			case MovementMode::Running:
				run = true;
				break;
		}
		if (u->kneeling_mode == KneelingMode::Kneeling)
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

	bool throwing = !battle.battleViewSelectedUnits.empty() &&
	                battle.battleViewSelectedUnits.front()->isThrowing();

	this->mainTab->findControlTyped<CheckBox>("BUTTON_LEFT_HAND_THROW")
	    ->setChecked(selectionState == BattleSelectionState::ThrowLeft || leftThrowDelay > 0 ||
	                 throwing);
	this->mainTab->findControlTyped<CheckBox>("BUTTON_RIGHT_HAND_THROW")
	    ->setChecked(selectionState == BattleSelectionState::ThrowRight || rightThrowDelay > 0 ||
	                 throwing);
}

void BattleView::orderJump(Vec3<int> target, BodyState bodyState)
{
	orderJump((Vec3<float>)target + Vec3<float>{0.5f, 0.5f, 0.0f}, bodyState);
}

void BattleView::orderJump(Vec3<float> target, BodyState bodyState)
{
	if (battle.battleViewSelectedUnits.empty())
	{
		return;
	}
	auto unit = battle.battleViewSelectedUnits.front();
	unit->setMission(*state, BattleUnitMission::jump(*unit, target, bodyState));
}

void BattleView::orderMove(Vec3<int> target, bool strafe, bool demandGiveWay)
{
	if (battle.battleViewSelectedUnits.empty())
	{
		return;
	}

	// Check if ordered to exit
	bool runAway = map.getTile(target)->getHasExit();

	auto u = battle.battleViewSelectedUnits.front();
	BattleUnit temp;
	temp.agent = u->agent;
	int facingDelta = strafe ? BattleUnitMission::getFacingDelta(
	                               u->facing, BattleUnitMission::getFacing(temp, target))
	                         : 0;

	if (battle.battleViewGroupMove && !runAway)
	{
		Battle::groupMove(*state, battle.battleViewSelectedUnits, target, facingDelta,
		                  demandGiveWay);
	}
	else
	{
		// FIXME: Handle group movement (don't forget to turn it off when running away)
		for (auto unit : battle.battleViewSelectedUnits)
		{
			if (strafe)
			{
				// FIXME: handle strafe movement
				LogWarning("Implement strafing!");
			}

			BattleUnitMission *mission;
			if (runAway)
			{
				// Running away units are impatient!
				mission = BattleUnitMission::gotoLocation(*unit, target, facingDelta, demandGiveWay,
				                                          true, 1, true);
			}
			else // not running away
			{
				mission =
				    BattleUnitMission::gotoLocation(*unit, target, facingDelta, demandGiveWay);
			}

			if (unit->setMission(*state, mission))
			{
				LogWarning("BattleUnit \"%s\" going to location %s", unit->agent->name, target);
			}
			else
			{
				LogWarning("BattleUnit \"%s\" could not receive order to move", unit->agent->name);
			}
		}
	}
}

void BattleView::orderTurn(Vec3<int> target)
{
	for (auto unit : battle.battleViewSelectedUnits)
	{
		if (unit->setMission(*state, BattleUnitMission::turn(*unit, target)))
		{
			LogWarning("BattleUnit \"%s\" turning to face location %s", unit->agent->name, target);
		}
		else
		{
			LogWarning("BattleUnit \"%s\" could not receive order to turn", unit->agent->name);
		}
	}
}

void BattleView::orderThrow(Vec3<int> target, bool right)
{
	if (battle.battleViewSelectedUnits.size() == 0)
	{
		return;
	}
	auto unit = battle.battleViewSelectedUnits.front();
	auto item = unit->agent->getFirstItemInSlot(right ? AEquipmentSlotType::RightHand
	                                                  : AEquipmentSlotType::LeftHand);
	if (!item)
	{
		return;
	}

	if (unit->setMission(*state, BattleUnitMission::throwItem(*unit, item, target)))
	{
		LogWarning("BattleUnit \"%s\" throwing item in the %s hand", unit->agent->name,
		           right ? "right" : "left");
		selectionState = BattleSelectionState::Normal;
	}
	else
	{
		actionImpossibleDelay = 40;
		return;
	}
}

void BattleView::orderUse(bool right, bool automatic)
{
	if (battle.battleViewSelectedUnits.size() == 0)
	{
		return;
	}
	auto unit = battle.battleViewSelectedUnits.front();
	auto item = unit->agent->getFirstItemInSlot(right ? AEquipmentSlotType::RightHand
	                                                  : AEquipmentSlotType::LeftHand);

	if (!item)
		return;

	switch (item->type->type)
	{
		case AEquipmentType::Type::Weapon:
			// Weapon has no automatic mode
			if (!item->canFire() || automatic)
			{
				break;
			}
			selectionState =
			    right ? BattleSelectionState::FireRight : BattleSelectionState::FireLeft;
			break;
		case AEquipmentType::Type::Grenade:
			if (automatic)
			{
				if (!item->primed)
				{
					item->prime();
				}
				this->selectionState =
				    right ? BattleSelectionState::ThrowRight : BattleSelectionState::ThrowLeft;
			}
			else
			{
				if (item->primed)
				{
					break;
				}
				this->activeTab = this->primingTab;
				this->activeTab->findControlTyped<CheckBox>("HIDDEN_CHECK_RIGHT_HAND")
				    ->setChecked(right);
				bool range;
				switch (item->type->trigger_type)
				{
					case TriggerType::Proximity:
					case TriggerType::Boomeroid:
						range = true;
						break;
					default:
						range = false;
				}
				this->activeTab->findControl("RANGE_TEXT")->setVisible(range);
				this->activeTab->findControl("RANGE_SLIDER")->setVisible(range);
			}
			break;
		case AEquipmentType::Type::MindBender:
			// Mind bender does not care for automatic mode
			selectionState = BattleSelectionState::Normal;
			this->activeTab = this->psiTab;
			this->activeTab->findControlTyped<RadioButton>("BUTTON_CONTROL")->setChecked(false);
			this->activeTab->findControlTyped<RadioButton>("BUTTON_PANIC")->setChecked(false);
			this->activeTab->findControlTyped<RadioButton>("BUTTON_STUN")->setChecked(false);
			this->activeTab->findControlTyped<RadioButton>("BUTTON_PROBE")->setChecked(false);
			this->activeTab->findControlTyped<CheckBox>("RIGHTHANDUSED")->setChecked(right);
			break;
		case AEquipmentType::Type::Teleporter:
			// Teleporter does not care for automatic mode
			selectionState =
			    right ? BattleSelectionState::TeleportRight : BattleSelectionState::TeleportLeft;
			break;
		// Usable items that have no automatic mode
		case AEquipmentType::Type::MotionScanner:
		case AEquipmentType::Type::MediKit:
			if (automatic)
			{
				break;
			}
			unit->useItem(*state, item);
			break;
		// Usable items that care not for automatic mode
		case AEquipmentType::Type::Popper:
		case AEquipmentType::Type::Brainsucker:
		case AEquipmentType::Type::Spawner:
			unit->useItem(*state, item);
			break;
		// Items that do nothing
		case AEquipmentType::Type::AlienDetector:
		case AEquipmentType::Type::Ammo:
		case AEquipmentType::Type::Armor:
		case AEquipmentType::Type::CloakingField:
		case AEquipmentType::Type::DimensionForceField:
		case AEquipmentType::Type::DisruptorShield:
		case AEquipmentType::Type::Loot:
		case AEquipmentType::Type::MindShield:
		case AEquipmentType::Type::MultiTracker:
		case AEquipmentType::Type::StructureProbe:
		case AEquipmentType::Type::VortexAnalyzer:
			break;
	}
}

void BattleView::orderDrop(bool right)
{
	if (battle.battleViewSelectedUnits.size() == 0)
	{
		return;
	}
	auto unit = battle.battleViewSelectedUnits.front();

	if (!unit->agent->type->inventory)
	{
		return;
	}

	auto item = unit->agent->getFirstItemInSlot(right ? AEquipmentSlotType::RightHand
	                                                  : AEquipmentSlotType::LeftHand);
	if (item) // Drop item
	{
		// Special case, just add mission in front of anything and start it, no need to clear orders
		unit->addMission(*state, BattleUnitMission::dropItem(*unit, item));
		LogWarning("BattleUnit \"%s\" dropping item in %s hand", unit->agent->name,
		           right ? "right" : "left");
	}
	else // Try to pick something up
	{
		auto items = unit->tileObject->getOwningTile()->getItems();
		if (items.empty())
		{
			return;
		}
		int cost = 8;
		if (!unit->spendTU(*state, cost))
		{
			return;
		}
		auto item = items.front();
		unit->agent->addEquipment(*state, item->item, right ? AEquipmentSlotType::RightHand
		                                                    : AEquipmentSlotType::LeftHand);
		item->die(*state, false);
	}
}

void BattleView::orderSelect(StateRef<BattleUnit> u, bool inverse, bool additive)
{
	auto pos =
	    std::find(battle.battleViewSelectedUnits.begin(), battle.battleViewSelectedUnits.end(), u);
	if (inverse)
	{
		// Unit in selection => remove
		if (pos != battle.battleViewSelectedUnits.end())
		{
			battle.battleViewSelectedUnits.erase(pos);
		}
	}
	else
	{
		// Unit not selected
		if (pos == battle.battleViewSelectedUnits.end())
		{
			if (additive)
			{
				// Unit not in selection, and not full => add unit to selection
				if (battle.battleViewSelectedUnits.size() < 6)
				{
					battle.battleViewSelectedUnits.push_front(u);
				}
			}
			else
			{
				// Unit not in selection => replace selection with unit
				battle.battleViewSelectedUnits.clear();
				battle.battleViewSelectedUnits.push_back(u);
			}
		}
		// Unit is selected
		else
		{
			// Unit in selection and additive  => move unit to front
			if (additive)
			{
				battle.battleViewSelectedUnits.erase(pos);
				battle.battleViewSelectedUnits.push_front(u);
			}
			// If not additive and in selection - select only this unit
			else if (battle.battleViewSelectedUnits.size() > 1)
			{
				battle.battleViewSelectedUnits.clear();
				battle.battleViewSelectedUnits.push_front(u);
			}
			// If not in additive mode and clicked on selected unit - deselect
			else
			{
				battle.battleViewSelectedUnits.clear();
			}
		}
	}
}

void BattleView::orderTeleport(Vec3<int> target, bool right)
{
	if (battle.battleViewSelectedUnits.size() == 0)
	{
		return;
	}
	auto unit = battle.battleViewSelectedUnits.front();
	auto item = unit->agent->getFirstItemInSlot(right ? AEquipmentSlotType::RightHand
	                                                  : AEquipmentSlotType::LeftHand);

	// FIXME: REMOVE TEMPORARY CHEAT
	if (!item || item->type->type != AEquipmentType::Type::Teleporter)
	{
		LogWarning("Using teleporter cheat!");
		item = mksp<AEquipment>();
		UString tp = "AEQUIPMENTTYPE_PERSONAL_TELEPORTER";
		item->type = {&*state, tp};
		item->ammo = item->type->max_ammo;
	}

	if (!item)
	{
		return;
	}

	auto m = BattleUnitMission::teleport(*unit, item, target);
	if (unit->setMission(*state, m) && !m->cancelled)
	{
		LogWarning("BattleUnit \"%s\" teleported using item in %s hand ", unit->agent->name,
		           right ? "right" : "left");
		selectionState = BattleSelectionState::Normal;
	}
	else
	{
		actionImpossibleDelay = 40;
		LogWarning("BattleUnit \"%s\" could not teleport using item in %s hand ", unit->agent->name,
		           right ? "right" : "left");
	}
}

void BattleView::orderFire(Vec3<int> target, WeaponStatus status, bool modifier)
{
	// FIXME: If TB ensure enough TUs for turn and fire
	for (auto unit : battle.battleViewSelectedUnits)
	{
		unit->startAttacking(*state, target, status, modifier);
	}
}

void BattleView::orderFire(StateRef<BattleUnit> u, WeaponStatus status)
{
	// FIXME: If TB ensure enough TUs for turn and fire
	for (auto unit : battle.battleViewSelectedUnits)
	{
		unit->startAttacking(*state, u, status);
	}
}

void BattleView::orderFocus(StateRef<BattleUnit> u)
{
	// FIXME: Check if player can see unit
	for (auto unit : battle.battleViewSelectedUnits)
	{
		unit->setFocus(*state, u);
	}
}

void BattleView::orderCancelPsi()
{
	if (battle.battleViewSelectedUnits.size() == 0)
	{
		return;
	}

	battle.battleViewSelectedUnits.front()->stopAttackPsi(*state);
}

void BattleView::orderPsiAttack(StateRef<BattleUnit> u, PsiStatus status, bool right)
{
	if (battle.battleViewSelectedUnits.size() == 0)
	{
		return;
	}

	auto unit = battle.battleViewSelectedUnits.front();
	auto item = unit->agent->getFirstItemInSlot(right ? AEquipmentSlotType::RightHand
	                                                  : AEquipmentSlotType::LeftHand);

	if (unit->startAttackPsi(*state, u, status, item->type))
	{
		this->activeTab->findControlTyped<RadioButton>("BUTTON_CONTROL")->setChecked(false);
		this->activeTab->findControlTyped<RadioButton>("BUTTON_PANIC")->setChecked(false);
		this->activeTab->findControlTyped<RadioButton>("BUTTON_STUN")->setChecked(false);
		this->activeTab->findControlTyped<RadioButton>("BUTTON_PROBE")->setChecked(false);
		this->selectionState = BattleSelectionState::Normal;
	}
}

void BattleView::orderHeal(BodyPart part)
{
	auto unit = battle.battleViewSelectedUnits.front();

	unit->useMedikit(*state, part);
}

void BattleView::eventOccurred(Event *e)
{
	activeTab->eventOccured(e);
	baseForm->eventOccured(e);
	bool eventWithin = false;
	for (auto f : itemForms)
	{
		if (f->Enabled)
		{
			f->eventOccured(e);
			if (f->eventIsWithin(e))
			{
				eventWithin = true;
			}
		}
	}

	if (eventWithin || activeTab->eventIsWithin(e) || baseForm->eventIsWithin(e))
	{
		return;
	}

	if (e->type() == EVENT_KEY_DOWN &&
	    (e->keyboard().KeyCode == SDLK_ESCAPE || e->keyboard().KeyCode == SDLK_PAGEUP ||
	     e->keyboard().KeyCode == SDLK_PAGEDOWN || e->keyboard().KeyCode == SDLK_TAB ||
	     e->keyboard().KeyCode == SDLK_SPACE || e->keyboard().KeyCode == SDLK_RSHIFT ||
	     e->keyboard().KeyCode == SDLK_LSHIFT || e->keyboard().KeyCode == SDLK_RALT ||
	     e->keyboard().KeyCode == SDLK_LALT || e->keyboard().KeyCode == SDLK_RCTRL ||
	     e->keyboard().KeyCode == SDLK_LCTRL || e->keyboard().KeyCode == SDLK_f ||
	     e->keyboard().KeyCode == SDLK_r || e->keyboard().KeyCode == SDLK_a ||
	     e->keyboard().KeyCode == SDLK_p || e->keyboard().KeyCode == SDLK_h ||
	     e->keyboard().KeyCode == SDLK_k || e->keyboard().KeyCode == SDLK_q ||
	     e->keyboard().KeyCode == SDLK_j))
	{
		switch (e->keyboard().KeyCode)
		{
			case SDLK_RSHIFT:
				modifierRShift = true;
				break;
			case SDLK_LSHIFT:
				modifierLShift = true;
				break;
			case SDLK_RALT:
				modifierRAlt = true;
				break;
			case SDLK_LALT:
				modifierLAlt = true;
				break;
			case SDLK_RCTRL:
				modifierRCtrl = true;
				break;
			case SDLK_LCTRL:
				modifierLCtrl = true;
				break;
			case SDLK_ESCAPE:
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH, mksp<InGameOptions>(state->shared_from_this())});
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
			case SDLK_f:
			{
				auto t = this->getSelectedTilePosition();
				auto &map = *battle.map;
				auto tile = map.getTile(t);
				for (auto &o : tile->ownedObjects)
				{
					if (o->getType() == TileObject::Type::Ground ||
					    o->getType() == TileObject::Type::Feature ||
					    o->getType() == TileObject::Type::LeftWall ||
					    o->getType() == TileObject::Type::RightWall)
					{
						auto mp = std::static_pointer_cast<TileObjectBattleMapPart>(o)->getOwner();
						auto set = mksp<std::set<BattleMapPart *>>();
						set->insert(mp.get());
						mp->queueCollapse();
						BattleMapPart::attemptReLinkSupports(set);
					}
				}
				break;
			}
			case SDLK_r:
			{
				revealWholeMap = !revealWholeMap;
				break;
			}
			case SDLK_q:
			{
				for (auto &u : battle.units)
				{
					if (u.second->isDead())
					{
						continue;
					}

					if (u.second->tileObject->getOwningTile()->position == selectedTilePosition)
					{
						auto movement = AIMovement();
						movement.type = AIMovement::Type::ChangeStance;
						movement.movementMode = MovementMode::Prone;
						u.second->executeAIMovement(*state, movement);
					}
				}
				break;
			}
			case SDLK_k:
			{
				bool inverse = modifierLShift || modifierRShift;
				bool local = modifierLCtrl || modifierRCtrl;
				for (auto &u : battle.units)
				{
					if (u.second->isDead())
					{
						continue;
					}

					if (((local &&
					      u.second->tileObject->getOwningTile()->position ==
					          selectedTilePosition) ||
					     (!local &&
					      glm::length(u.second->position - (Vec3<float>)selectedTilePosition) <
					          5.0f)) == !inverse)
					{
						u.second->die(*state);
						u.second->tileObject->removeFromMap();
						u.second->destroyed = true;
					}
				}
				break;
			}
			case SDLK_p:
			{
				if (modifierLShift || modifierRShift)
				{
					LogWarning("Psi amplified!");
					for (auto &u : battle.units)
					{
						if (u.second->isDead())
						{
							continue;
						}

						u.second->agent->modified_stats.psi_defence = 0;
						u.second->agent->modified_stats.psi_attack = 100;
						u.second->agent->modified_stats.psi_energy = 100;
					}
					break;
				}
				else
				{
					LogWarning("Panic mode engaged!");
					for (auto &u : battle.units)
					{
						if (u.second->isConscious())
						{
							u.second->agent->modified_stats.morale = 25;
						}
					}
				}
				break;
			}
			case SDLK_h:
			{
				LogWarning("Heals for everybody!");
				for (auto &u : battle.units)
				{
					if (u.second->isDead())
					{
						continue;
					}
					u.second->stunDamage = 0;
					u.second->agent->modified_stats = u.second->agent->current_stats;
					u.second->fatalWounds[BodyPart::Body] = 0;
					u.second->fatalWounds[BodyPart::Helmet] = 0;
					u.second->fatalWounds[BodyPart::LeftArm] = 0;
					u.second->fatalWounds[BodyPart::Legs] = 0;
					u.second->fatalWounds[BodyPart::RightArm] = 0;
				}
				break;
			}
			case SDLK_a:
			{
				LogWarning("AI debug key currently disabled");
				if (battle.units.empty())
				{
					break;
				}
				bool activeAIFound = battle.units.begin()->second->aiList.aiList.size() > 3;
				if (activeAIFound)
				{
					for (auto &u : battle.units)
					{
						u.second->aiList.aiList.pop_back();
					}
				}
				else
				{
					for (auto &u : battle.units)
					{
						u.second->aiList.init(*state, *u.second);
					}
				}
				break;
			}
			case SDLK_j:
			{
				if (!battle.battleViewSelectedUnits.empty())
				{
					auto t = this->getSelectedTilePosition();
					orderJump(t);
				}
				break;
			}
		}
	}
	else if (e->type() == EVENT_MOUSE_MOVE)
	{
		Vec2<float> screenOffset = {this->getScreenOffset().x, this->getScreenOffset().y};
		// Offset by 4 since ingame 4 is the typical height of the ground, and game displays cursor
		// on top of the ground
		setSelectedTilePosition(this->screenToTileCoords(
		    Vec2<float>((float)e->mouse().X, (float)e->mouse().Y + 4) - screenOffset,
		    (float)getZLevel() - 1.0f));
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
				break;
			case SDLK_LSHIFT:
				modifierLShift = false;
				break;
			case SDLK_RALT:
				modifierRAlt = false;
				break;
			case SDLK_LALT:
				modifierLAlt = false;
				break;
			case SDLK_RCTRL:
				modifierRCtrl = false;
				break;
			case SDLK_LCTRL:
				modifierLCtrl = false;
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
		         (Event::isPressed(e->mouse().Button, Event::MouseButton::Middle)))
		{
			// CHEAT - move unit to mouse
			if (!battle.battleViewSelectedUnits.empty())
			{
				selectionState = BattleSelectionState::TeleportLeft;
			}
		}
		else if (e->type() == EVENT_MOUSE_DOWN &&
		         (Event::isPressed(e->mouse().Button, Event::MouseButton::Left) ||
		          Event::isPressed(e->mouse().Button, Event::MouseButton::Right)))
		{
			auto buttonPressed = Event::isPressed(e->mouse().Button, Event::MouseButton::Left)
			                         ? Event::MouseButton::Left
			                         : Event::MouseButton::Right;

			auto player = state->getPlayer();
			// If a click has not been handled by a form it's in the map.
			auto t = this->getSelectedTilePosition();
			auto objsPresent = map.getTile(t.x, t.y, t.z)->getUnits(true);
			auto unitPresent = objsPresent.empty() ? nullptr : objsPresent.front();
			auto objsOccupying = map.getTile(t.x, t.y, t.z)->getUnits(true, true);
			auto unitOccupying = objsOccupying.empty() ? nullptr : objsOccupying.front();
			if (unitOccupying)
			{
				unitPresent = unitOccupying;
			}
			// Determine attack target
			sp<BattleUnit> attackTarget = nullptr;
			if (!attackTarget)
			{
				for (auto u : objsOccupying)
				{
					if (player->isRelatedTo(u->owner) == Organisation::Relation::Hostile &&
					    battle.visibleUnits[player].find({&*state, u->id}) !=
					        battle.visibleUnits[player].end())
					{
						attackTarget = u;
						break;
					}
				}
			}
			if (!attackTarget)
			{
				for (auto u : objsPresent)
				{
					if (player->isRelatedTo(u->owner) == Organisation::Relation::Hostile &&
					    battle.visibleUnits[player].find({&*state, u->id}) !=
					        battle.visibleUnits[player].end())
					{
						attackTarget = u;
						break;
					}
				}
			}
			if (!attackTarget && unitPresent && (unitPresent->owner == player ||
			    battle.visibleUnits[player].find({&*state, unitPresent->id}) !=
			        battle.visibleUnits[player].end()))
			{
				attackTarget = unitPresent;
			}
			// Determine selection/deselection target
			sp<BattleUnit> selectionTarget = nullptr;
			if (!selectionTarget)
			{
				for (auto u : objsOccupying)
				{
					if (player == u->owner && u->moraleState == MoraleState::Normal)
					{
						selectionTarget = u;
						break;
					}
				}
			}
			if (!selectionTarget)
			{
				for (auto u : objsPresent)
				{
					if (player == u->owner && u->moraleState == MoraleState::Normal)
					{
						selectionTarget = u;
						break;
					}
				}
			}
			// Determine course of action
			LogWarning("Click at tile %d, %d, %d", t.x, t.y, t.z);
			switch (selectionState)
			{
				case BattleSelectionState::Normal:
				case BattleSelectionState::NormalAlt:
					switch (buttonPressed)
					{
						case Event::MouseButton::Left:
							// If selectable unit is present, priority is to move if not occupied
							if (selectionTarget)
							{
								// Move if units are selected and noone is occupying
								if (!unitOccupying && battle.battleViewSelectedUnits.size() > 0)
								{
									orderMove(t, selectionState == BattleSelectionState::NormalAlt);
								}
								// Select if friendly unit present under cursor
								else
								{
									orderSelect({&*state, selectionTarget->id});
								}
							}
							// Otherwise move if not occupied
							else if (!unitOccupying)
							{
								orderMove(t, selectionState == BattleSelectionState::NormalAlt);
							}
							break;
						case Event::MouseButton::Right:
							// Turn if no enemy unit present under cursor
							// or if holding alt
							if (!attackTarget ||
							    player->isRelatedTo(attackTarget->owner) !=
							        Organisation::Relation::Hostile ||
							    selectionState == BattleSelectionState::NormalAlt)
							{
								orderTurn(t);
							}
							// Focus fire RT / Fire TB
							else
							{
								switch (battle.mode)
								{
									case Battle::Mode::TurnBased:
										if (attackTarget)
										{
											orderFire({&*state, attackTarget->id});
										}
										break;
									case Battle::Mode::RealTime:
										if (attackTarget->isConscious())
										{
											orderFocus({&*state, attackTarget->id});
										}
										break;
								}
							}
							break;
						default:
							LogError("Unhandled mouse button!");
							break;
					}
					break;
				case BattleSelectionState::NormalCtrl:
				case BattleSelectionState::NormalCtrlAlt:
					// Selection add/remove mode
					switch (buttonPressed)
					{
						// LMB = Add to selection
						case Event::MouseButton::Left:
							if (selectionTarget)
							{
								orderSelect({&*state, selectionTarget->id}, false, true);
							}
							// If none occupying - order move if additional modifiers held
							else if (!unitOccupying &&
							         selectionState != BattleSelectionState::NormalCtrl)
							{
								// Move pathing through
								orderMove(t, false, true);
							}
							break;
						// RMB = Remove from selection
						case Event::MouseButton::Right:
							if (selectionTarget)
							{
								orderSelect({&*state, selectionTarget->id}, true);
							}
							break;
						default:
							LogError("Unhandled mouse button!");
							break;
					}
					// Debug section below
					if (true)
					{
						UString debug = "";
						debug += format("\nDEBUG INFORMATION ABOUT TILE %d, %d, %d", t.x, t.y, t.z);
						debug += format("\n LOS BLOCK %d", battle.getLosBlockID(t.x, t.y, t.z));
						auto &map = *battle.map;
						auto tile = map.getTile(t);
						for (auto &o : tile->ownedObjects)
						{
							if (o->getType() == TileObject::Type::Ground ||
							    o->getType() == TileObject::Type::Feature ||
							    o->getType() == TileObject::Type::LeftWall ||
							    o->getType() == TileObject::Type::RightWall)
							{
								auto mp = std::static_pointer_cast<TileObjectBattleMapPart>(o)
								              ->getOwner();
								debug += format(
								    "\n[%s] SBT %d STATUS %s\nFIRE Res=%d Tim=%d Burned=%D",
								    mp->type.id, mp->type->getVanillaSupportedById(),
								    !mp->isAlive()
								        ? "DEAD "
								        : (mp->damaged
								               ? "DAMAGED"
								               : (mp->providesHardSupport ? "HARD " : "SOFT ")),
								    mp->type->fire_resist, mp->type->fire_burn_time,
								    mp->burnTicksAccumulated);
								for (int x = t.x - 1; x <= t.x + 1; x++)
								{
									for (int y = t.y - 1; y <= t.y + 1; y++)
									{
										for (int z = t.z - 1; z <= t.z + 1; z++)
										{
											if (x < 0 || x >= map.size.x || y < 0 ||
											    y >= map.size.y || z < 0 || z >= map.size.z)
											{
												continue;
											}
											auto tile2 = map.getTile(x, y, z);
											for (auto &o2 : tile2->ownedObjects)
											{
												if (o2->getType() == TileObject::Type::Ground ||
												    o2->getType() == TileObject::Type::Feature ||
												    o2->getType() == TileObject::Type::LeftWall ||
												    o2->getType() == TileObject::Type::RightWall)
												{
													auto mp2 = std::static_pointer_cast<
													               TileObjectBattleMapPart>(o2)
													               ->getOwner();
													for (auto &p : mp2->supportedParts)
													{
														if (p.first == t &&
														    p.second == mp->type->type)
														{
															debug += format(
															    "\nSupported by %s at %d %d %d",
															    mp2->type.id, x - t.x, y - t.y,
															    z - t.z);
														}
													}
												}
											}
										}
									}
								}
							}
							if (o->getType() == TileObject::Type::Hazard)
							{
								auto h = std::static_pointer_cast<TileObjectBattleHazard>(o)
								             ->getHazard();
								debug += format("\nHazard %s %s Pow=%d Age=%d LT=%d  ",
								                h->damageType.id, h->damageType->hazardType.id,
								                h->power, h->age, h->lifetime);
							}
						}

						auto uto = tile->getUnitIfPresent();
						if (uto)
						{
							auto u = uto->getUnit();
							debug += format("\nContains unit %s.", u->id.cStr());
							debug += format("\nMorale state: %d", (int)u->moraleState);
							debug += format("\nPosition: %f, %f, %f", u->position.x, u->position.y,
							                u->position.z);
							debug += format("\nGoal: %f, %f, %f", u->goalPosition.x,
							                u->goalPosition.y, u->goalPosition.z);
							debug += format("\nCurrent movement: %d, falling: %d",
							                (int)u->current_movement_state, (int)u->falling);
							debug += format("\nMissions [%d]:", (int)u->missions.size());
							for (auto &m : u->missions)
							{
								debug += format("\n%s", m->getName());
							}
							debug += format("\nSeen units [%d]:", (int)u->visibleUnits.size());
							for (auto &unit : u->visibleUnits)
							{
								debug += format("\n%s", unit.id);
							}
							/*	debug += format(
							        "\nCurrent ai state:\n  %s\n  enSp %d enSpPr %d attPos %s "
							        "lasSnEnPos %s",
							        u->aiList.lastDecision.getName(), (int)u->aiState.enemySpotted,
							        (int)u->aiState.enemySpottedPrevious,
							   u->aiState.attackerPosition,
							        u->aiList.lastSeenEnemyPosition);*/
						}
						LogWarning("%s", debug);
					}
					break;
				case BattleSelectionState::FireAny:
				case BattleSelectionState::FireLeft:
				case BattleSelectionState::FireRight:
					switch (buttonPressed)
					{
						case Event::MouseButton::Left:
						{
							WeaponStatus status = WeaponStatus::FiringBothHands;
							switch (selectionState)
							{
								case BattleSelectionState::FireLeft:
									status = WeaponStatus::FiringLeftHand;
									break;
								case BattleSelectionState::FireRight:
									status = WeaponStatus::FiringRightHand;
									break;
								case BattleSelectionState::FireAny:
								default:
									// Do nothing
									break;
							}
							if (attackTarget)
							{
								orderFire({&*state, attackTarget->id}, status);
							}
							else
							{
								bool modified = (modifierLAlt || modifierRAlt);
								orderFire(t, status, modified);
							}
							break;
						}
						case Event::MouseButton::Right:
						{
							if (selectionState != BattleSelectionState::FireAny)
							{
								selectionState = BattleSelectionState::Normal;
							}
							break;
						}
						default:
							LogError("Unhandled mouse button!");
							break;
					}
					break;
				case BattleSelectionState::ThrowRight:
				case BattleSelectionState::ThrowLeft:
					switch (buttonPressed)
					{
						case Event::MouseButton::Left:
						{
							bool right = selectionState == BattleSelectionState::ThrowRight;
							orderThrow(t, right);
							break;
						}
						case Event::MouseButton::Right:
						{
							selectionState = BattleSelectionState::Normal;
							break;
						}
						default:
							LogError("Unhandled mouse button!");
							break;
					}
					break;
				case BattleSelectionState::TeleportLeft:
				case BattleSelectionState::TeleportRight:
					switch (buttonPressed)
					{
						case Event::MouseButton::Left:
						{
							bool right = selectionState == BattleSelectionState::TeleportRight;
							orderTeleport(t, right);
							break;
						}
						case Event::MouseButton::Right:
						{
							selectionState = BattleSelectionState::Normal;
							break;
						}
						default:
							LogError("Unhandled mouse button!");
							break;
					}
					break;
				case BattleSelectionState::PsiControl:
				case BattleSelectionState::PsiPanic:
				case BattleSelectionState::PsiStun:
				case BattleSelectionState::PsiProbe:
					switch (buttonPressed)
					{
						case Event::MouseButton::Left:
						{
							auto u = battle.battleViewSelectedUnits.front();
							if (attackTarget && attackTarget != u.getSp())
							{
								bool right =
								    this->activeTab->findControlTyped<CheckBox>("RIGHTHANDUSED")
								        ->isChecked();
								switch (selectionState)
								{
									case BattleSelectionState::PsiControl:
										orderPsiAttack({&*state, attackTarget->id},
										               PsiStatus::Control, right);
										break;
									case BattleSelectionState::PsiPanic:
										orderPsiAttack({&*state, attackTarget->id},
										               PsiStatus::Panic, right);
										break;
									case BattleSelectionState::PsiStun:
										orderPsiAttack({&*state, attackTarget->id}, PsiStatus::Stun,
										               right);
										break;
									case BattleSelectionState::PsiProbe:
										orderPsiAttack({&*state, attackTarget->id},
										               PsiStatus::Probe, right);
										break;
									default:
										// Don't cry Travis
										break;
								}
							}
							break;
						}
						case Event::MouseButton::Right:
						{
							selectionState = BattleSelectionState::Normal;
							if (this->activeTab != psiTab)
							{
								LogError("How come are we in psi mode but not in psi tab?");
							}
							else
							{
								this->activeTab->findControlTyped<RadioButton>("BUTTON_CONTROL")
								    ->setChecked(false);
								this->activeTab->findControlTyped<RadioButton>("BUTTON_PANIC")
								    ->setChecked(false);
								this->activeTab->findControlTyped<RadioButton>("BUTTON_STUN")
								    ->setChecked(false);
								this->activeTab->findControlTyped<RadioButton>("BUTTON_PROBE")
								    ->setChecked(false);
							}
							break;
						}
						default:
							LogError("Unhandled mouse button!");
							break;
					}
					break;
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
			this->mainTab->findControlTyped<RadioButton>("BUTTON_LAYER_1")->setChecked(true);
			break;
		case 2:
			this->mainTab->findControlTyped<RadioButton>("BUTTON_LAYER_2")->setChecked(true);
			break;
		case 3:
			this->mainTab->findControlTyped<RadioButton>("BUTTON_LAYER_3")->setChecked(true);
			break;
		case 4:
			this->mainTab->findControlTyped<RadioButton>("BUTTON_LAYER_4")->setChecked(true);
			break;
		case 5:
			this->mainTab->findControlTyped<RadioButton>("BUTTON_LAYER_5")->setChecked(true);
			break;
		case 6:
			this->mainTab->findControlTyped<RadioButton>("BUTTON_LAYER_6")->setChecked(true);
			break;
		case 7:
			this->mainTab->findControlTyped<RadioButton>("BUTTON_LAYER_7")->setChecked(true);
			break;
		case 8:
			this->mainTab->findControlTyped<RadioButton>("BUTTON_LAYER_8")->setChecked(true);
			break;
		case 9:
			this->mainTab->findControlTyped<RadioButton>("BUTTON_LAYER_9")->setChecked(true);
			break;
	}
}

void BattleView::onNewTurn()
{
	if (battle.currentActiveOrganisation == battle.currentPlayer)
	{
		baseForm->findControlTyped<Ticker>("NEWS_TICKER")
		    ->addMessage(tr("Turn:") + " " + format("%d", battle.currentTurn) + "   " +
		                 tr("Side:") + "  " + tr(battle.currentActiveOrganisation->name));
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
			int accuracy = info.accuracy;
			int colorsCount = (int)accuracyColors.size();
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
		if (info.maxAmmo > 0 && info.curAmmo > 0)
		{
			int ammoDisplaySize = 90;

			int ammoCount = info.curAmmo;
			int ammoPadding = 1;
			int ammoSize = ammoDisplaySize / info.maxAmmo - 1;
			int x = right ? 1 : 47;
			if (ammoSize <= 0)
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

void BattleView::updatePsiInfo()
{
	this->activeTab->findControlTyped<Graphic>("OVERLAY_CONTROL")
	    ->setImage(psiInfo.status == PsiStatus::Control ? selectedPsiOverlay : nullptr);
	this->activeTab->findControlTyped<Graphic>("OVERLAY_PANIC")
	    ->setImage(psiInfo.status == PsiStatus::Panic ? selectedPsiOverlay : nullptr);
	this->activeTab->findControlTyped<Graphic>("OVERLAY_STUN")
	    ->setImage(psiInfo.status == PsiStatus::Stun ? selectedPsiOverlay : nullptr);
	this->activeTab->findControlTyped<Graphic>("OVERLAY_PROBE")
	    ->setImage(psiInfo.status == PsiStatus::Probe ? selectedPsiOverlay : nullptr);

	this->activeTab->findControlTyped<Label>("PSI_ENERGY_LABEL")
	    ->setText(format("%d", psiInfo.curEnergy));
	this->activeTab->findControlTyped<Label>("PSI_ATTACK_LABEL")
	    ->setText(format("%d", psiInfo.curAttack));
	this->activeTab->findControlTyped<Label>("PSI_DEFENSE_LABEL")
	    ->setText(format("%d", psiInfo.curDefense));

	// FIXME: Maybe pre-draw all 100 of them?

	this->activeTab->findControlTyped<Graphic>("PSI_ENERGY_BAR")
	    ->setImage(drawPsiBar(psiInfo.curEnergy, psiInfo.maxEnergy));
	this->activeTab->findControlTyped<Graphic>("PSI_ATTACK_BAR")
	    ->setImage(drawPsiBar(psiInfo.curAttack, psiInfo.maxAttack));
	this->activeTab->findControlTyped<Graphic>("PSI_DEFENSE_BAR")
	    ->setImage(drawPsiBar(psiInfo.curDefense, psiInfo.maxDefense));
}

sp<RGBImage> BattleView::drawPsiBar(int cur, int max)
{
	int width = 137;
	int height = 6;
	Colour border = {150, 70, 150, 255};
	Colour bar = {180, 120, 150, 255};
	Colour black = {0, 0, 0, 255};
	auto psiBar = mksp<RGBImage>(Vec2<int>{width, height});
	{
		int curWidth = clamp((width - 3) * cur / 100, 0, (width - 3)) + 1;
		int maxWidth = clamp((width - 3) * max / 100, 0, (width - 3)) + 2;
		{
			RGBImageLock l(psiBar);
			// Content
			for (int x = 1; x < maxWidth; x++)
			{
				for (int y = 1; y < height - 1; y++)
				{
					if (x <= curWidth)
						l.set({x, y}, bar);
					else
						l.set({x, y}, black);
				}
			}
			// Borders
			for (int y = 1; y < height - 1; y++)
			{
				l.set({0, y}, border);
				l.set({maxWidth, y}, border);
			}
			for (int x = 0; x <= maxWidth; x++)
			{
				l.set({x, 0}, border);
				l.set({x, 5}, border);
			}
			// Remainder
			for (int x = maxWidth + 1; x < width; x++)
			{
				for (int y = 0; y < height; y++)
				{
					l.set({x, y}, black);
				}
			}
		}
	}
	return psiBar;
}

void BattleView::finish()
{
	fw().getCursor().CurrentType = ApocCursor::CursorType::Normal;
	Battle::finishBattle(*state);
}

AgentEquipmentInfo BattleView::createItemOverlayInfo(bool rightHand)
{
	AgentEquipmentInfo a;
	if (battle.battleViewSelectedUnits.size() == 0)
	{
		return a;
	}
	auto u = battle.battleViewSelectedUnits.front();
	sp<AEquipment> e = nullptr;
	if (rightHand)
	{
		e = u->agent->getFirstItemInSlot(AEquipmentSlotType::RightHand);
	}
	else
	{
		e = u->agent->getFirstItemInSlot(AEquipmentSlotType::LeftHand);
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
				switch (a.itemType->type)
				{
					case AEquipmentType::Type::Weapon:
					case AEquipmentType::Type::Ammo:
						a.maxAmmo = p->max_ammo;
						a.curAmmo = e->ammo;
						break;
					case AEquipmentType::Type::Armor:
						a.maxAmmo = 30;
						a.curAmmo = e->armor * 30 / p->armor;
						break;
					case AEquipmentType::Type::DisruptorShield:
						a.maxAmmo = 30;
						a.curAmmo = e->ammo * 30 / p->max_ammo;
						break;
					case AEquipmentType::Type::Teleporter:
						a.maxAmmo = 3;
						a.curAmmo = e->ammo * 3 / p->max_ammo;
						break;
					default:
						break;
				}
				a.selected = e->inUse || e->primed ||
				             (u->psiStatus != PsiStatus::NotEngaged && e->type == u->psiItem) ||
				             (selectionState == BattleSelectionState::FireRight && rightHand) ||
				             (selectionState == BattleSelectionState::FireLeft && !rightHand) ||
				             (selectionState == BattleSelectionState::TeleportRight && rightHand) ||
				             (selectionState == BattleSelectionState::TeleportLeft && !rightHand);
			}

			auto accuracy =
			    (float)e->getAccuracy(u->current_body_state, u->current_movement_state,
			                          u->fire_aiming_mode,
			                          a.itemType->type != AEquipmentType::Type::Weapon) /
			    100.0f;
			// erf takes values -2 to 2 and returns -1 to 1,
			// we need it to take values 0 to 1 and return 0 to 1
			// val -> val * 4 - 2, result -> result /2 + 0,5
			a.accuracy = (int)(((erf(accuracy * 4.0f - 2.0f)) / 2.0f + 0.5f) * 50.0f);
		}
	}
	return a;
}

AgentPsiInfo BattleView::createPsiInfo()
{
	AgentPsiInfo a;

	if (!battle.battleViewSelectedUnits.empty())
	{
		auto u = battle.battleViewSelectedUnits.front();

		a.status = u->psiStatus;
		a.curEnergy = u->agent->modified_stats.psi_energy;
		a.curAttack = u->agent->modified_stats.psi_attack;
		a.curDefense = u->agent->modified_stats.psi_defence;
		a.maxEnergy = u->agent->current_stats.psi_energy;
		a.maxAttack = u->agent->current_stats.psi_attack;
		a.maxDefense = u->agent->current_stats.psi_defence;
	}

	return a;
}

bool AgentEquipmentInfo::operator==(const AgentEquipmentInfo &other) const
{
	return (this->accuracy == other.accuracy && this->curAmmo == other.curAmmo &&
	        this->itemType == other.itemType && this->damageType == other.damageType &&
	        this->selected == other.selected);
}

bool AgentEquipmentInfo::operator!=(const AgentEquipmentInfo &other) const
{
	return !(*this == other);
}

bool AgentPsiInfo::operator==(const AgentPsiInfo &other) const
{
	return (this->status == other.status && this->curEnergy == other.curEnergy &&
	        this->curAttack == other.curAttack && this->curDefense == other.curDefense &&
	        this->maxEnergy == other.maxEnergy && this->maxAttack == other.maxAttack &&
	        this->maxDefense == other.maxDefense);
}

bool AgentPsiInfo::operator!=(const AgentPsiInfo &other) const { return !(*this == other); }
}; // namespace OpenApoc
