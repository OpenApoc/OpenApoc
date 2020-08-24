#include "game/ui/tileview/battleview.h"
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
#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/font.h"
#include "framework/framework.h"
#include "framework/jukebox.h"
#include "framework/keycodes.h"
#include "framework/renderer.h"
#include "framework/sound.h"
#include "game/state/battle/ai/aitype.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battlehazard.h"
#include "game/state/battle/battleitem.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battlescanner.h"
#include "game/state/battle/battleunit.h"
#include "game/state/gameevent.h"
#include "game/state/gamestate.h"
#include "game/state/message.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/battle/battlemapparttype.h"
#include "game/state/rules/battle/damage.h"
#include "game/state/shared/aequipment.h"
#include "game/state/shared/projectile.h"
#include "game/state/tilemap/collision.h"
#include "game/state/tilemap/tileobject_battlehazard.h"
#include "game/state/tilemap/tileobject_battlemappart.h"
#include "game/state/tilemap/tileobject_battleunit.h"
#include "game/ui/base/basescreen.h"
#include "game/ui/battle/battledebriefing.h"
#include "game/ui/battle/battleturnbasedconfirmbox.h"
#include "game/ui/components/controlgenerator.h"
#include "game/ui/general/aequipscreen.h"
#include "game/ui/general/ingameoptions.h"
#include "game/ui/general/messagebox.h"
#include "game/ui/general/messagelogscreen.h"
#include "game/ui/general/savemenu.h"
#include "library/sp.h"
#include "library/strings_format.h"
#include <cmath>
#include <glm/glm.hpp>

namespace OpenApoc
{

namespace
{
static const std::vector<UString> HIDDEN_BACKGROUNDS = {
    "xcom3/tacdata/hidden1.pcx",
    "xcom3/tacdata/hidden2.pcx",
    "xcom3/tacdata/hidden3.pcx",
    "xcom3/tacdata/hidden4.pcx",
};
static const int TICKS_TRY_END_TURN = TICKS_PER_SECOND;
static const int TICKS_HIDE_DISPLAY = TICKS_PER_SECOND;
static const int TICKS_END_MISSION = TICKS_PER_TURN;
static const std::set<BodyPart> bodyParts{BodyPart::Body, BodyPart::Helmet, BodyPart::LeftArm,
                                          BodyPart::Legs, BodyPart::RightArm};

static const int NUM_TABS_RT = 3;
static const int NUM_TABS_TB = 4;

} // anonymous namespace

BattleView::BattleView(sp<GameState> gameState)
    : BattleTileView(*gameState->current_battle->map,
                     Vec3<int>{TILE_X_BATTLE, TILE_Y_BATTLE, TILE_Z_BATTLE},
                     Vec2<int>{STRAT_TILE_X, STRAT_TILE_Y}, TileViewMode::Isometric,
                     gameState->current_battle->battleViewScreenCenter, *gameState),
      baseForm(ui().getForm("battle/battle")), state(gameState), battle(*state->current_battle),
      followAgent(false), selectionState(BattleSelectionState::Normal)
{
	motionScannerDirectionIcons.push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                88)));
	motionScannerDirectionIcons.push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                89)));
	motionScannerDirectionIcons.push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                90)));
	motionScannerDirectionIcons.push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                91)));
	motionScannerDirectionIcons.push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                92)));
	motionScannerDirectionIcons.push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                93)));
	motionScannerDirectionIcons.push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                94)));
	motionScannerDirectionIcons.push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                95)));

	selectedItemOverlay = fw().data->loadImage("battle/battle-item-select-icon.png");
	selectedPsiOverlay = fw().data->loadImage("battle/battle-psi-select-icon.png");
	pauseIcon = fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                        "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                        260));

	squadOverlay.emplace_back();
	squadOverlay.push_back(fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                                   "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                                   1)));
	squadOverlay.push_back(fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                                   "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                                   0)));

	unitHostiles.emplace_back();
	unitHostiles.push_back(fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                                   "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                                   7)));
	unitHostiles.push_back(fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                                   "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                                   8)));
	unitHostiles.push_back(fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                                   "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                                   9)));
	unitHostiles.push_back(fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                                   "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                                   10)));
	unitHostiles.push_back(fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                                   "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                                   11)));
	unitHostiles.push_back(fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                                   "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                                   12)));

	lastClickedHostile.resize(6);

	auto font = ui().getFont("smallset");
	squadNumber.emplace_back();
	for (int i = 1; i <= 6; i++)
	{
		squadNumber.push_back(font->getString(format("%d", i)));
	}

	for (int i = 0; i < NUM_TABS_RT; ++i)
	{
		sp<Form> f = baseForm->findControlTyped<Form>(format("SUBFORM_RT_%d", i + 1));
		f->takesFocus = false;
		uiTabsRT.push_back(f);
	}
	for (int i = 0; i < NUM_TABS_TB; ++i)
	{
		sp<Form> f = baseForm->findControlTyped<Form>(format("SUBFORM_TB_%d", i + 1));
		f->takesFocus = false;
		uiTabsTB.push_back(f);
	}
	{
		executePlanPopup = mksp<BattleTurnBasedConfirmBox>(
		    tr("Execute remaining movement orders for this unit?"),
		    [this] {
			    unitPendingConfirmation->missions.pop_front();
			    unitPendingConfirmation = nullptr;
		    },
		    [this] { unitPendingConfirmation = nullptr; });
	}

	switch (battle.mode)
	{
		case Battle::Mode::RealTime:
			mainTab = uiTabsRT[0];
			psiTab = uiTabsRT[1];
			primingTab = uiTabsRT[2];
			baseForm->findControlTyped<RadioButton>("BUTTON_SPEED0")->setChecked(true);
			updateSpeed = BattleUpdateSpeed::Pause;
			lastSpeed = BattleUpdateSpeed::Speed1;
			break;
		case Battle::Mode::TurnBased:
			mainTab = uiTabsTB[0];
			psiTab = uiTabsTB[1];
			primingTab = uiTabsTB[2];
			notMyTurnTab = uiTabsTB[3];
			baseForm->findControlTyped<RadioButton>("BUTTON_SPEED2")->setChecked(true);
			updateSpeed = BattleUpdateSpeed::Speed2;
			lastSpeed = BattleUpdateSpeed::Pause;
			break;
	}
	setSelectedTab(mainTab);

	medikitForms[false] = ui().getForm("battle/battle_medkit_left");
	medikitForms[true] = ui().getForm("battle/battle_medkit_right");
	motionScannerForms[false] = ui().getForm("battle/battle_scanner_left");
	motionScannerForms[true] = ui().getForm("battle/battle_scanner_right");

	itemForms.push_back(medikitForms[false]);
	itemForms.push_back(medikitForms[true]);
	itemForms.push_back(motionScannerForms[false]);
	itemForms.push_back(motionScannerForms[true]);
	for (auto &f : itemForms)
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

	motionScannerData[false] =
	    motionScannerForms[false]->findControlTyped<Graphic>("MOTION_SCANNER_DATA");
	motionScannerData[true] =
	    motionScannerForms[true]->findControlTyped<Graphic>("MOTION_SCANNER_DATA");
	motionScannerUnit[false] =
	    motionScannerForms[false]->findControlTyped<Graphic>("MOTION_SCANNER_UNIT");
	motionScannerUnit[true] =
	    motionScannerForms[true]->findControlTyped<Graphic>("MOTION_SCANNER_UNIT");

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

	baseForm->findControl("BUTTON_FOLLOW_AGENT")
	    ->addCallback(FormEventType::CheckBoxChange, [this](FormsEvent *e) {
		    this->followAgent =
		        std::dynamic_pointer_cast<CheckBox>(e->forms().RaisedBy)->isChecked();
	    });
	baseForm->findControl("BUTTON_TOGGLE_STRATMAP")
	    ->addCallback(FormEventType::CheckBoxChange, [this](FormsEvent *e) {
		    bool strategy = std::dynamic_pointer_cast<CheckBox>(e->forms().RaisedBy)->isChecked();
		    this->setViewMode(strategy ? TileViewMode::Strategy : TileViewMode::Isometric);
	    });
	baseForm->findControl("BUTTON_LAYERING")
	    ->addCallback(FormEventType::TriStateBoxChange, [this](FormsEvent *e) {
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

	baseForm->findControl("BUTTON_CEASE_FIRE")
	    ->addCallback(FormEventType::MouseClick, [this](Event *) {
		    bool at_will = false;
		    for (auto &u : this->battle.battleViewSelectedUnits)
		    {
			    if (u->fire_permission_mode == BattleUnit::FirePermissionMode::AtWill)
			    {
				    at_will = true;
			    }
		    }
		    for (auto &u : this->battle.battleViewSelectedUnits)
		    {
			    if (at_will)
			    {
				    u->setFirePermissionMode(BattleUnit::FirePermissionMode::CeaseFire);
			    }
			    else
			    {
				    u->setFirePermissionMode(BattleUnit::FirePermissionMode::AtWill);
			    }
		    }
	    });
	baseForm->findControl("BUTTON_AIMED")->addCallback(FormEventType::MouseClick, [this](Event *) {
		for (auto &u : this->battle.battleViewSelectedUnits)
		{
			u->setWeaponAimingMode(WeaponAimingMode::Aimed);
		}
	});
	baseForm->findControl("BUTTON_SNAP")->addCallback(FormEventType::MouseClick, [this](Event *) {
		for (auto &u : this->battle.battleViewSelectedUnits)
		{
			u->setWeaponAimingMode(WeaponAimingMode::Snap);
		}
	});
	baseForm->findControl("BUTTON_AUTO")->addCallback(FormEventType::MouseClick, [this](Event *) {
		for (auto &u : this->battle.battleViewSelectedUnits)
		{
			u->setWeaponAimingMode(WeaponAimingMode::Auto);
		}
	});
	baseForm->findControl("BUTTON_KNEEL")->addCallback(FormEventType::MouseClick, [this](Event *) {
		bool not_kneeling = false;

		for (auto &u : this->battle.battleViewSelectedUnits)
		{
			if (u->kneeling_mode == KneelingMode::None &&
			    u->agent->isBodyStateAllowed(BodyState::Kneeling))
			{
				not_kneeling = true;
			}
		}
		for (auto &u : this->battle.battleViewSelectedUnits)
		{
			if (not_kneeling)
			{
				u->setKneelingMode(KneelingMode::Kneeling);
			}
			else
			{
				u->setKneelingMode(KneelingMode::None);
			}
		}
	});
	baseForm->findControl("BUTTON_PRONE")->addCallback(FormEventType::MouseClick, [this](Event *) {
		for (auto &u : this->battle.battleViewSelectedUnits)
		{
			u->setMovementMode(MovementMode::Prone);
		}
	});
	baseForm->findControl("BUTTON_WALK")->addCallback(FormEventType::MouseClick, [this](Event *) {
		for (auto &u : this->battle.battleViewSelectedUnits)
		{
			u->setMovementMode(MovementMode::Walking);
		}
	});
	baseForm->findControl("BUTTON_RUN")->addCallback(FormEventType::MouseClick, [this](Event *) {
		for (auto &u : this->battle.battleViewSelectedUnits)
		{
			u->setMovementMode(MovementMode::Running);
		}
	});
	baseForm->findControl("BUTTON_EVASIVE")
	    ->addCallback(FormEventType::MouseClick, [this](Event *) {
		    for (auto &u : this->battle.battleViewSelectedUnits)
		    {
			    u->setBehaviorMode(BattleUnit::BehaviorMode::Evasive);
		    }
	    });
	baseForm->findControl("BUTTON_NORMAL")->addCallback(FormEventType::MouseClick, [this](Event *) {
		for (auto &u : this->battle.battleViewSelectedUnits)
		{
			u->setBehaviorMode(BattleUnit::BehaviorMode::Normal);
		}
	});
	baseForm->findControl("BUTTON_AGGRESSIVE")
	    ->addCallback(FormEventType::MouseClick, [this](Event *) {
		    for (auto &u : this->battle.battleViewSelectedUnits)
		    {
			    u->setBehaviorMode(BattleUnit::BehaviorMode::Aggressive);
		    }
	    });

	baseForm->findControl("BUTTON_RESERVE_AIMED")
	    ->addCallback(FormEventType::MouseClick, [this](Event *) {
		    bool pushed = false;
		    for (auto &u : this->battle.battleViewSelectedUnits)
		    {
			    if (u->reserve_shot_mode == ReserveShotMode::Aimed)
			    {
				    pushed = true;
			    }
		    }
		    for (auto &u : this->battle.battleViewSelectedUnits)
		    {
			    u->setReserveShotMode(*state,
			                          pushed ? ReserveShotMode::None : ReserveShotMode::Aimed);
		    }
	    });

	baseForm->findControl("BUTTON_RESERVE_SNAP")
	    ->addCallback(FormEventType::MouseClick, [this](Event *) {
		    bool pushed = false;
		    for (auto &u : this->battle.battleViewSelectedUnits)
		    {
			    if (u->reserve_shot_mode == ReserveShotMode::Snap)
			    {
				    pushed = true;
			    }
		    }
		    for (auto &u : this->battle.battleViewSelectedUnits)
		    {
			    u->setReserveShotMode(*state,
			                          pushed ? ReserveShotMode::None : ReserveShotMode::Snap);
		    }
	    });

	baseForm->findControl("BUTTON_RESERVE_AUTO")
	    ->addCallback(FormEventType::MouseClick, [this](Event *) {
		    bool pushed = false;
		    for (auto &u : this->battle.battleViewSelectedUnits)
		    {
			    if (u->reserve_shot_mode == ReserveShotMode::Auto)
			    {
				    pushed = true;
			    }
		    }
		    for (auto &u : this->battle.battleViewSelectedUnits)
		    {
			    u->setReserveShotMode(*state,
			                          pushed ? ReserveShotMode::None : ReserveShotMode::Auto);
		    }
	    });

	baseForm->findControl("BUTTON_RESERVE_KNEEL")
	    ->addCallback(FormEventType::MouseClick, [this](Event *) {
		    bool pushed = false;
		    for (auto &u : this->battle.battleViewSelectedUnits)
		    {
			    if (u->reserve_kneel_mode == KneelingMode::Kneeling)
			    {
				    pushed = true;
			    }
		    }
		    for (auto &u : this->battle.battleViewSelectedUnits)
		    {
			    u->setReserveKneelMode(pushed ? KneelingMode::None : KneelingMode::Kneeling);
		    }
	    });

	baseForm->findControl("BUTTON_LAYER_UP")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->setZLevel(getZLevel() + 1);
		    updateLayerButtons();
	    });
	baseForm->findControl("BUTTON_LAYER_DOWN")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->setZLevel(getZLevel() - 1);
		    updateLayerButtons();
	    });

	baseForm->findControl("BUTTON_MOVE_GROUP")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *) { this->battle.battleViewGroupMove = true; });
	baseForm->findControl("BUTTON_MOVE_INDIVIDUALLY")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *) { this->battle.battleViewGroupMove = false; });
	if (battle.battleViewGroupMove)
	{
		baseForm->findControlTyped<RadioButton>("BUTTON_MOVE_GROUP")->setChecked(true);
	}
	else
	{
		baseForm->findControlTyped<RadioButton>("BUTTON_MOVE_INDIVIDUALLY")->setChecked(true);
	}

	std::function<void(int index)> clickedSquad = [this](int index) {
		if (baseForm->findControlTyped<CheckBox>("BUTTON_SQUAD_ASSIGN")->isChecked())
		{
			baseForm->findControlTyped<CheckBox>("BUTTON_SQUAD_ASSIGN")->setChecked(false);
			for (auto &u : this->battle.battleViewSelectedUnits)
			{
				if (u->squadNumber != index)
				{
					if (this->battle.forces[this->battle.currentPlayer]
					        .squads[index]
					        .getNumUnits() < 6)
					{
						u->assignToSquad(this->battle, index);
					}
				}
			}
		}
		if (this->battle.battleViewSquadIndex != index)
		{
			this->battle.battleViewSquadIndex = index;
		}
		else
		{
			StateRef<BattleUnit> firstUnit;
			bool ctrl = this->modifierRCtrl || this->modifierLCtrl;

			if (!ctrl)
			{
				this->battle.battleViewSelectedUnits.clear();
			}
			for (auto &u : this->battle.forces[this->battle.currentPlayer].squads[index].units)
			{
				if (!firstUnit)
				{
					firstUnit = {&*this->state, u->id};
				}
				if (battle.battleViewSelectedUnits.size() < 6)
				{
					this->battle.battleViewSelectedUnits.emplace_back(&*this->state, u->id);
				}
			}
			if (firstUnit && std::find(this->battle.battleViewSelectedUnits.begin(),
			                           this->battle.battleViewSelectedUnits.end(),
			                           firstUnit) == this->battle.battleViewSelectedUnits.end())
			{
				firstUnit.clear();
			}
			if (firstUnit)
			{
				this->zoomAt(firstUnit->position);
				this->battle.battleViewSelectedUnits.remove(firstUnit);
				this->battle.battleViewSelectedUnits.push_front(firstUnit);
			}
		}
	};

	std::function<void(FormsEvent * e)> clickedSquad1 = [clickedSquad](FormsEvent *) {
		clickedSquad(0);
	};
	std::function<void(FormsEvent * e)> clickedSquad2 = [clickedSquad](FormsEvent *) {
		clickedSquad(1);
	};
	std::function<void(FormsEvent * e)> clickedSquad3 = [clickedSquad](FormsEvent *) {
		clickedSquad(2);
	};
	std::function<void(FormsEvent * e)> clickedSquad4 = [clickedSquad](FormsEvent *) {
		clickedSquad(3);
	};
	std::function<void(FormsEvent * e)> clickedSquad5 = [clickedSquad](FormsEvent *) {
		clickedSquad(4);
	};
	std::function<void(FormsEvent * e)> clickedSquad6 = [clickedSquad](FormsEvent *) {
		clickedSquad(5);
	};
	baseForm->findControlTyped<Graphic>("SQUAD_1_OVERLAY")
	    ->addCallback(FormEventType::MouseClick, clickedSquad1);
	baseForm->findControlTyped<Graphic>("SQUAD_2_OVERLAY")
	    ->addCallback(FormEventType::MouseClick, clickedSquad2);
	baseForm->findControlTyped<Graphic>("SQUAD_3_OVERLAY")
	    ->addCallback(FormEventType::MouseClick, clickedSquad3);
	baseForm->findControlTyped<Graphic>("SQUAD_4_OVERLAY")
	    ->addCallback(FormEventType::MouseClick, clickedSquad4);
	baseForm->findControlTyped<Graphic>("SQUAD_5_OVERLAY")
	    ->addCallback(FormEventType::MouseClick, clickedSquad5);
	baseForm->findControlTyped<Graphic>("SQUAD_6_OVERLAY")
	    ->addCallback(FormEventType::MouseClick, clickedSquad6);

	std::function<void(int index)> clickedUnitPortrait = [this](int index) {
		if (!this->unitInfo[index].agent || this->unitInfo[index].faded)
		{
			return;
		}
		bool ctrl = this->modifierRCtrl || this->modifierLCtrl;
		StateRef<BattleUnit> unit = this->unitInfo[index].agent->unit;
		bool needZoom = !this->battle.battleViewSelectedUnits.empty() &&
		                this->battle.battleViewSelectedUnits.front() == unit;
		if (!ctrl)
		{
			this->battle.battleViewSelectedUnits.clear();
		}
		else
		{
			if (std::find(this->battle.battleViewSelectedUnits.begin(),
			              this->battle.battleViewSelectedUnits.end(),
			              unit) != this->battle.battleViewSelectedUnits.end())
			{
				this->battle.battleViewSelectedUnits.remove(unit);
			}
		}
		this->battle.battleViewSelectedUnits.push_front(unit);
		if (needZoom)
		{
			this->zoomAt(unit->position);
		}
	};
	std::function<void(FormsEvent * e)> clickedUnitPortrait1 = [clickedUnitPortrait](FormsEvent *) {
		clickedUnitPortrait(0);
	};
	std::function<void(FormsEvent * e)> clickedUnitPortrait2 = [clickedUnitPortrait](FormsEvent *) {
		clickedUnitPortrait(1);
	};
	std::function<void(FormsEvent * e)> clickedUnitPortrait3 = [clickedUnitPortrait](FormsEvent *) {
		clickedUnitPortrait(2);
	};
	std::function<void(FormsEvent * e)> clickedUnitPortrait4 = [clickedUnitPortrait](FormsEvent *) {
		clickedUnitPortrait(3);
	};
	std::function<void(FormsEvent * e)> clickedUnitPortrait5 = [clickedUnitPortrait](FormsEvent *) {
		clickedUnitPortrait(4);
	};
	std::function<void(FormsEvent * e)> clickedUnitPortrait6 = [clickedUnitPortrait](FormsEvent *) {
		clickedUnitPortrait(5);
	};
	baseForm->findControlTyped<Graphic>("UNIT_1")->addCallback(FormEventType::MouseClick,
	                                                           clickedUnitPortrait1);
	baseForm->findControlTyped<Graphic>("UNIT_2")->addCallback(FormEventType::MouseClick,
	                                                           clickedUnitPortrait2);
	baseForm->findControlTyped<Graphic>("UNIT_3")->addCallback(FormEventType::MouseClick,
	                                                           clickedUnitPortrait3);
	baseForm->findControlTyped<Graphic>("UNIT_4")->addCallback(FormEventType::MouseClick,
	                                                           clickedUnitPortrait4);
	baseForm->findControlTyped<Graphic>("UNIT_5")->addCallback(FormEventType::MouseClick,
	                                                           clickedUnitPortrait5);
	baseForm->findControlTyped<Graphic>("UNIT_6")->addCallback(FormEventType::MouseClick,
	                                                           clickedUnitPortrait6);

	std::function<void(int index)> clickedUnitHostiles = [this](int index) {
		if (!this->unitInfo[index].agent ||
		    this->unitInfo[index].agent->unit->visibleEnemies.empty())
		{
			return;
		}
		this->lastClickedHostile[index]++;
		if (this->lastClickedHostile[index] >=
		    this->unitInfo[index].agent->unit->visibleEnemies.size())
		{
			this->lastClickedHostile[index] = 0;
		}
		auto it = this->unitInfo[index].agent->unit->visibleEnemies.begin();
		for (int i = 0; i < this->lastClickedHostile[index]; i++)
		{
			it++;
		}
		this->zoomAt((*it)->position);
	};
	std::function<void(FormsEvent * e)> clickedUnitHostiles1 = [clickedUnitHostiles](FormsEvent *) {
		clickedUnitHostiles(0);
	};
	std::function<void(FormsEvent * e)> clickedUnitHostiles2 = [clickedUnitHostiles](FormsEvent *) {
		clickedUnitHostiles(1);
	};
	std::function<void(FormsEvent * e)> clickedUnitHostiles3 = [clickedUnitHostiles](FormsEvent *) {
		clickedUnitHostiles(2);
	};
	std::function<void(FormsEvent * e)> clickedUnitHostiles4 = [clickedUnitHostiles](FormsEvent *) {
		clickedUnitHostiles(3);
	};
	std::function<void(FormsEvent * e)> clickedUnitHostiles5 = [clickedUnitHostiles](FormsEvent *) {
		clickedUnitHostiles(4);
	};
	std::function<void(FormsEvent * e)> clickedUnitHostiles6 = [clickedUnitHostiles](FormsEvent *) {
		clickedUnitHostiles(5);
	};
	baseForm->findControlTyped<Graphic>("UNIT_1_HOSTILES")
	    ->addCallback(FormEventType::MouseClick, clickedUnitHostiles1);
	baseForm->findControlTyped<Graphic>("UNIT_2_HOSTILES")
	    ->addCallback(FormEventType::MouseClick, clickedUnitHostiles2);
	baseForm->findControlTyped<Graphic>("UNIT_3_HOSTILES")
	    ->addCallback(FormEventType::MouseClick, clickedUnitHostiles3);
	baseForm->findControlTyped<Graphic>("UNIT_4_HOSTILES")
	    ->addCallback(FormEventType::MouseClick, clickedUnitHostiles4);
	baseForm->findControlTyped<Graphic>("UNIT_5_HOSTILES")
	    ->addCallback(FormEventType::MouseClick, clickedUnitHostiles5);
	baseForm->findControlTyped<Graphic>("UNIT_6_HOSTILES")
	    ->addCallback(FormEventType::MouseClick, clickedUnitHostiles6);

	uiTabsRT[0]
	    ->findControl("BUTTON_LAYER_1")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { setZLevel(1); });
	uiTabsRT[0]
	    ->findControl("BUTTON_LAYER_2")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { setZLevel(2); });
	uiTabsRT[0]
	    ->findControl("BUTTON_LAYER_3")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { setZLevel(3); });
	uiTabsRT[0]
	    ->findControl("BUTTON_LAYER_4")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { setZLevel(4); });
	uiTabsRT[0]
	    ->findControl("BUTTON_LAYER_5")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { setZLevel(5); });
	uiTabsRT[0]
	    ->findControl("BUTTON_LAYER_6")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { setZLevel(6); });
	uiTabsRT[0]
	    ->findControl("BUTTON_LAYER_7")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { setZLevel(7); });
	uiTabsRT[0]
	    ->findControl("BUTTON_LAYER_8")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { setZLevel(8); });
	uiTabsRT[0]
	    ->findControl("BUTTON_LAYER_9")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) { setZLevel(9); });
	uiTabsTB[0]
	    ->findControl("BUTTON_LAYER_1")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    setZLevel(1);
		    uiTabsTB[3]->findControlTyped<RadioButton>("BUTTON_LAYER_1")->setChecked(true);
	    });
	uiTabsTB[0]
	    ->findControl("BUTTON_LAYER_2")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    setZLevel(2);
		    uiTabsTB[3]->findControlTyped<RadioButton>("BUTTON_LAYER_2")->setChecked(true);
	    });
	uiTabsTB[0]
	    ->findControl("BUTTON_LAYER_3")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    setZLevel(3);
		    uiTabsTB[3]->findControlTyped<RadioButton>("BUTTON_LAYER_3")->setChecked(true);
	    });
	uiTabsTB[0]
	    ->findControl("BUTTON_LAYER_4")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    setZLevel(4);
		    uiTabsTB[3]->findControlTyped<RadioButton>("BUTTON_LAYER_4")->setChecked(true);
	    });
	uiTabsTB[0]
	    ->findControl("BUTTON_LAYER_5")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    setZLevel(5);
		    uiTabsTB[3]->findControlTyped<RadioButton>("BUTTON_LAYER_5")->setChecked(true);
	    });
	uiTabsTB[0]
	    ->findControl("BUTTON_LAYER_6")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    setZLevel(6);
		    uiTabsTB[3]->findControlTyped<RadioButton>("BUTTON_LAYER_6")->setChecked(true);
	    });
	uiTabsTB[0]
	    ->findControl("BUTTON_LAYER_7")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    setZLevel(7);
		    uiTabsTB[3]->findControlTyped<RadioButton>("BUTTON_LAYER_7")->setChecked(true);
	    });
	uiTabsTB[0]
	    ->findControl("BUTTON_LAYER_8")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    setZLevel(8);
		    uiTabsTB[3]->findControlTyped<RadioButton>("BUTTON_LAYER_8")->setChecked(true);
	    });
	uiTabsTB[0]
	    ->findControl("BUTTON_LAYER_9")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    setZLevel(9);
		    uiTabsTB[3]->findControlTyped<RadioButton>("BUTTON_LAYER_9")->setChecked(true);
	    });
	uiTabsTB[3]
	    ->findControl("BUTTON_LAYER_1")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    uiTabsTB[0]->findControlTyped<RadioButton>("BUTTON_LAYER_1")->setChecked(true);
	    });
	uiTabsTB[3]
	    ->findControl("BUTTON_LAYER_2")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    uiTabsTB[0]->findControlTyped<RadioButton>("BUTTON_LAYER_2")->setChecked(true);
	    });
	uiTabsTB[3]
	    ->findControl("BUTTON_LAYER_3")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    uiTabsTB[0]->findControlTyped<RadioButton>("BUTTON_LAYER_3")->setChecked(true);
	    });
	uiTabsTB[3]
	    ->findControl("BUTTON_LAYER_4")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    uiTabsTB[0]->findControlTyped<RadioButton>("BUTTON_LAYER_4")->setChecked(true);
	    });
	uiTabsTB[3]
	    ->findControl("BUTTON_LAYER_5")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    uiTabsTB[0]->findControlTyped<RadioButton>("BUTTON_LAYER_5")->setChecked(true);
	    });
	uiTabsTB[3]
	    ->findControl("BUTTON_LAYER_6")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    uiTabsTB[0]->findControlTyped<RadioButton>("BUTTON_LAYER_6")->setChecked(true);
	    });
	uiTabsTB[3]
	    ->findControl("BUTTON_LAYER_7")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    uiTabsTB[0]->findControlTyped<RadioButton>("BUTTON_LAYER_7")->setChecked(true);
	    });
	uiTabsTB[3]
	    ->findControl("BUTTON_LAYER_8")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    uiTabsTB[0]->findControlTyped<RadioButton>("BUTTON_LAYER_8")->setChecked(true);
	    });
	uiTabsTB[3]
	    ->findControl("BUTTON_LAYER_9")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    uiTabsTB[0]->findControlTyped<RadioButton>("BUTTON_LAYER_9")->setChecked(true);
	    });

	baseForm->findControl("BUTTON_SHOW_OPTIONS")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    fw().stageQueueCommand(
		        {StageCmd::Command::PUSH, mksp<InGameOptions>(this->state->shared_from_this())});
	    });
	this->baseForm->findControl("BUTTON_SHOW_LOG")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    fw().stageQueueCommand(
		        {StageCmd::Command::PUSH, mksp<MessageLogScreen>(this->state, *this)});
	    });
	this->baseForm->findControl("BUTTON_ZOOM_EVENT")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    if (baseForm->findControlTyped<Ticker>("NEWS_TICKER")->hasMessages())
		    {
			    LogWarning("Has Messages!");
			    this->zoomLastEvent();
		    }
	    });

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

	std::function<void(FormsEvent * e)> openInventory = [this](Event *) { openAgentInventory(); };

	std::function<void(FormsEvent * e)> dropRightHand = [this](Event *) { orderDrop(true); };

	std::function<void(FormsEvent * e)> dropLeftHand = [this](Event *) { orderDrop(false); };

	std::function<void(FormsEvent * e)> cancelThrow = [this](Event *) {
		this->setSelectedTab(this->mainTab);
	};

	std::function<void(bool right)> throwItem = [this](bool right) {
		bool fail = false;
		if (this->battle.battleViewSelectedUnits.empty())
		{
			fail = true;
		}
		else
		{
			auto unit = this->battle.battleViewSelectedUnits.front();
			if (!(unit->agent->getFirstItemInSlot(right ? EquipmentSlotType::RightHand
			                                            : EquipmentSlotType::LeftHand)) ||
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

	std::function<void(FormsEvent * e)> throwRightHand = [throwItem](Event *) { throwItem(true); };

	std::function<void(FormsEvent * e)> throwLeftHand = [throwItem](Event *) { throwItem(false); };

	std::function<void(FormsEvent * e)> finishPriming = [this, throwItem](Event *) {
		bool right =
		    this->primingTab->findControlTyped<CheckBox>("HIDDEN_CHECK_RIGHT_HAND")->isChecked();
		auto unit = this->battle.battleViewSelectedUnits.front();
		auto item = unit->agent->getFirstItemInSlot(right ? EquipmentSlotType::RightHand
		                                                  : EquipmentSlotType::LeftHand);

		int delay = this->primingTab->findControlTyped<ScrollBar>("DELAY_SLIDER")->getValue();
		int range = this->primingTab->findControlTyped<ScrollBar>("RANGE_SLIDER")->getValue();
		if (delay == 0 && (item->type->trigger_type != TriggerType::Boomeroid ||
		                   item->type->trigger_type != TriggerType::Proximity))
		{
			item->prime();
		}
		else
		{
			if (this->battle.mode == Battle::Mode::TurnBased)
			{
				item->prime(false, delay * TICKS_PER_TURN, (range + 1) * 6);
			}
			else
			{
				item->prime(false, delay * TICKS_PER_SECOND / 4, (range + 1) * 6);
			}
		}
		this->setSelectedTab(this->mainTab);
	};

	std::function<void(FormsEvent * e)> updateDelay = [this](Event *) { this->refreshDelayText(); };

	std::function<void(FormsEvent * e)> updateRange = [this](Event *) { this->refreshRangeText(); };

	// Priming controls

	uiTabsRT[2]->findControlTyped<CheckBox>("HIDDEN_CHECK_RIGHT_HAND")->setClickSound(nullptr);
	uiTabsRT[2]->findControlTyped<CheckBox>("HIDDEN_CHECK_RIGHT_HAND")->Enabled = false;
	uiTabsRT[2]
	    ->findControlTyped<GraphicButton>("BUTTON_CANCEL")
	    ->addCallback(FormEventType::ButtonClick, cancelThrow);
	uiTabsRT[2]
	    ->findControlTyped<GraphicButton>("BUTTON_OK")
	    ->addCallback(FormEventType::ButtonClick, finishPriming);
	uiTabsRT[2]
	    ->findControlTyped<ScrollBar>("DELAY_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, updateDelay);
	uiTabsRT[2]
	    ->findControlTyped<ScrollBar>("RANGE_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, updateRange);
	uiTabsRT[2]->findControlTyped<CheckBox>("HIDDEN_CHECK_RIGHT_HAND")->setClickSound(nullptr);
	uiTabsTB[2]->findControlTyped<CheckBox>("HIDDEN_CHECK_RIGHT_HAND")->Enabled = false;
	uiTabsTB[2]
	    ->findControlTyped<GraphicButton>("BUTTON_CANCEL")
	    ->addCallback(FormEventType::ButtonClick, cancelThrow);
	uiTabsTB[2]
	    ->findControlTyped<GraphicButton>("BUTTON_OK")
	    ->addCallback(FormEventType::ButtonClick, finishPriming);
	uiTabsTB[2]
	    ->findControlTyped<ScrollBar>("DELAY_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, updateDelay);
	uiTabsTB[2]
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
		this->setSelectedTab(this->mainTab);
		this->selectionState = BattleSelectionState::Normal;
	};

	// Psi controls

	uiTabsRT[1]
	    ->findControlTyped<GraphicButton>("BUTTON_CANCEL")
	    ->addCallback(FormEventType::ButtonClick, cancelPsi);
	uiTabsRT[1]
	    ->findControlTyped<RadioButton>("BUTTON_CONTROL")
	    ->addCallback(FormEventType::MouseClick, psiControl);
	uiTabsRT[1]
	    ->findControlTyped<RadioButton>("BUTTON_PANIC")
	    ->addCallback(FormEventType::MouseClick, psiPanic);
	uiTabsRT[1]
	    ->findControlTyped<RadioButton>("BUTTON_STUN")
	    ->addCallback(FormEventType::MouseClick, psiStun);
	uiTabsRT[1]
	    ->findControlTyped<RadioButton>("BUTTON_PROBE")
	    ->addCallback(FormEventType::MouseClick, psiProbe);
	uiTabsTB[1]
	    ->findControlTyped<GraphicButton>("BUTTON_CANCEL")
	    ->addCallback(FormEventType::ButtonClick, cancelPsi);
	uiTabsTB[1]
	    ->findControlTyped<RadioButton>("BUTTON_CONTROL")
	    ->addCallback(FormEventType::MouseClick, psiControl);
	uiTabsTB[1]
	    ->findControlTyped<RadioButton>("BUTTON_PANIC")
	    ->addCallback(FormEventType::MouseClick, psiPanic);
	uiTabsTB[1]
	    ->findControlTyped<RadioButton>("BUTTON_STUN")
	    ->addCallback(FormEventType::MouseClick, psiStun);
	uiTabsTB[1]
	    ->findControlTyped<RadioButton>("BUTTON_PROBE")
	    ->addCallback(FormEventType::MouseClick, psiProbe);

	// Hand controls

	uiTabsRT[0]
	    ->findControlTyped<Graphic>("CLICKY_RIGHT_HAND")
	    ->addCallback(FormEventType::MouseClick, clickedRightHand);
	uiTabsRT[0]
	    ->findControlTyped<Graphic>("CLICKY_LEFT_HAND")
	    ->addCallback(FormEventType::MouseClick, clickedLeftHand);
	uiTabsTB[0]
	    ->findControlTyped<Graphic>("CLICKY_RIGHT_HAND")
	    ->addCallback(FormEventType::MouseClick, clickedRightHand);
	uiTabsTB[0]
	    ->findControlTyped<Graphic>("CLICKY_LEFT_HAND")
	    ->addCallback(FormEventType::MouseClick, clickedLeftHand);
	uiTabsRT[0]
	    ->findControlTyped<GraphicButton>("BUTTON_INVENTORY")
	    ->addCallback(FormEventType::MouseClick, openInventory);
	uiTabsRT[0]
	    ->findControlTyped<GraphicButton>("BUTTON_RIGHT_HAND_DROP")
	    ->addCallback(FormEventType::MouseClick, dropRightHand);
	uiTabsRT[0]
	    ->findControlTyped<GraphicButton>("BUTTON_LEFT_HAND_DROP")
	    ->addCallback(FormEventType::MouseClick, dropLeftHand);
	uiTabsTB[0]
	    ->findControlTyped<GraphicButton>("BUTTON_INVENTORY")
	    ->addCallback(FormEventType::MouseClick, openInventory);
	uiTabsTB[0]
	    ->findControlTyped<GraphicButton>("BUTTON_RIGHT_HAND_DROP")
	    ->addCallback(FormEventType::MouseClick, dropRightHand);
	uiTabsTB[0]
	    ->findControlTyped<GraphicButton>("BUTTON_LEFT_HAND_DROP")
	    ->addCallback(FormEventType::MouseClick, dropLeftHand);
	uiTabsRT[0]
	    ->findControlTyped<CheckBox>("BUTTON_RIGHT_HAND_THROW")
	    ->addCallback(FormEventType::MouseClick, throwRightHand);
	uiTabsRT[0]
	    ->findControlTyped<CheckBox>("BUTTON_LEFT_HAND_THROW")
	    ->addCallback(FormEventType::MouseClick, throwLeftHand);
	uiTabsTB[0]
	    ->findControlTyped<CheckBox>("BUTTON_RIGHT_HAND_THROW")
	    ->addCallback(FormEventType::MouseClick, throwRightHand);
	uiTabsTB[0]
	    ->findControlTyped<CheckBox>("BUTTON_LEFT_HAND_THROW")
	    ->addCallback(FormEventType::MouseClick, throwLeftHand);

	// We need these in TB because we will be able to allow pausing then

	baseForm->findControl("BUTTON_SPEED0")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    this->lastSpeed = this->updateSpeed;
		    this->updateSpeed = BattleUpdateSpeed::Pause;
	    });
	baseForm->findControl("BUTTON_SPEED1")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->updateSpeed = BattleUpdateSpeed::Speed1; });
	baseForm->findControl("BUTTON_SPEED2")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->updateSpeed = BattleUpdateSpeed::Speed2; });
	baseForm->findControl("BUTTON_SPEED3")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->updateSpeed = BattleUpdateSpeed::Speed3; });

	for (int i = 0; i < 6; i++)
	{
		unitInfo.emplace_back();
		spottedInfo.emplace_back(0);
		squadInfo.emplace_back();
	}

	switch (battle.mode)
	{
		case Battle::Mode::RealTime:
			baseForm->findControl("BUTTON_RESERVE_AIMED")->setVisible(false);
			baseForm->findControl("BUTTON_RESERVE_SNAP")->setVisible(false);
			baseForm->findControl("BUTTON_RESERVE_AUTO")->setVisible(false);
			baseForm->findControl("BUTTON_RESERVE_KNEEL")->setVisible(false);
			baseForm->findControl("BUTTON_ENDTURN")->setVisible(false);
			break;
		case Battle::Mode::TurnBased:
			baseForm->findControl("BUTTON_SPEED0")->setVisible(false);
			baseForm->findControl("BUTTON_SPEED1")->setVisible(false);
			baseForm->findControl("BUTTON_SPEED2")->setVisible(false);
			baseForm->findControl("BUTTON_SPEED3")->setVisible(false);
			baseForm->findControl("CLOCK")->setVisible(false);
			baseForm->findControl("BUTTON_ENDTURN")
			    ->addCallback(FormEventType::ButtonClick,
			                  [this](Event *) { endTurnRequested = true; });
			break;
	}

	updateLayerButtons();
}

BattleView::~BattleView() = default;

void BattleView::begin()
{
	BattleTileView::begin();
	fw().jukebox->play(JukeBox::PlayList::Tactical);
	uiTabsRT[0]->findControl("BUTTON_LAYER_1")->setVisible(maxZDraw >= 1);
	uiTabsRT[0]->findControl("BUTTON_LAYER_2")->setVisible(maxZDraw >= 2);
	uiTabsRT[0]->findControl("BUTTON_LAYER_3")->setVisible(maxZDraw >= 3);
	uiTabsRT[0]->findControl("BUTTON_LAYER_4")->setVisible(maxZDraw >= 4);
	uiTabsRT[0]->findControl("BUTTON_LAYER_5")->setVisible(maxZDraw >= 5);
	uiTabsRT[0]->findControl("BUTTON_LAYER_6")->setVisible(maxZDraw >= 6);
	uiTabsRT[0]->findControl("BUTTON_LAYER_7")->setVisible(maxZDraw >= 7);
	uiTabsRT[0]->findControl("BUTTON_LAYER_8")->setVisible(maxZDraw >= 8);
	uiTabsRT[0]->findControl("BUTTON_LAYER_9")->setVisible(maxZDraw >= 9);
	uiTabsTB[0]->findControl("BUTTON_LAYER_1")->setVisible(maxZDraw >= 1);
	uiTabsTB[0]->findControl("BUTTON_LAYER_2")->setVisible(maxZDraw >= 2);
	uiTabsTB[0]->findControl("BUTTON_LAYER_3")->setVisible(maxZDraw >= 3);
	uiTabsTB[0]->findControl("BUTTON_LAYER_4")->setVisible(maxZDraw >= 4);
	uiTabsTB[0]->findControl("BUTTON_LAYER_5")->setVisible(maxZDraw >= 5);
	uiTabsTB[0]->findControl("BUTTON_LAYER_6")->setVisible(maxZDraw >= 6);
	uiTabsTB[0]->findControl("BUTTON_LAYER_7")->setVisible(maxZDraw >= 7);
	uiTabsTB[0]->findControl("BUTTON_LAYER_8")->setVisible(maxZDraw >= 8);
	uiTabsTB[0]->findControl("BUTTON_LAYER_9")->setVisible(maxZDraw >= 9);
	uiTabsTB[3]->findControl("BUTTON_LAYER_1")->setVisible(maxZDraw >= 1);
	uiTabsTB[3]->findControl("BUTTON_LAYER_2")->setVisible(maxZDraw >= 2);
	uiTabsTB[3]->findControl("BUTTON_LAYER_3")->setVisible(maxZDraw >= 3);
	uiTabsTB[3]->findControl("BUTTON_LAYER_4")->setVisible(maxZDraw >= 4);
	uiTabsTB[3]->findControl("BUTTON_LAYER_5")->setVisible(maxZDraw >= 5);
	uiTabsTB[3]->findControl("BUTTON_LAYER_6")->setVisible(maxZDraw >= 6);
	uiTabsTB[3]->findControl("BUTTON_LAYER_7")->setVisible(maxZDraw >= 7);
	uiTabsTB[3]->findControl("BUTTON_LAYER_8")->setVisible(maxZDraw >= 8);
	uiTabsTB[3]->findControl("BUTTON_LAYER_9")->setVisible(maxZDraw >= 9);
	if (battle.mode == Battle::Mode::TurnBased)
	{
		auto event = GameBattleEvent(GameEventType::NewTurn, battle.shared_from_this());
		baseForm->findControlTyped<Ticker>("NEWS_TICKER")->addMessage(event.message());
	}
}

void BattleView::resume()
{
	state->skipTurboCalculations = config().getBool("OpenApoc.NewFeature.SkipTurboMovement");
	BattleTileView::resume();
	modifierLAlt = false;
	modifierLCtrl = false;
	modifierLShift = false;
	modifierRAlt = false;
	modifierRCtrl = false;
	modifierRShift = false;
}

void BattleView::render()
{
	BattleTileView::render();
	if (hideDisplay)
	{
		return;
	}

	baseForm->render();

	int pauseIconOffsetX = 0;

	// Item forms
	for (auto &f : itemForms)
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
	if (fw().stageGetCurrent() != shared_from_this())
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

void BattleView::setSelectedTab(sp<Form> tabPtr)
{
	for (auto tab : uiTabsRT)
		tab->setVisible(false);
	for (auto tab : uiTabsTB)
		tab->setVisible(false);
	tabPtr->setVisible(true);
	this->activeTab = tabPtr;
}

void BattleView::update()
{
	bool realTime = battle.mode == Battle::Mode::RealTime;

	// Parent update
	BattleTileView::update();

	// Update turn based stuff
	if (!realTime)
	{
		if (state->current_battle->hotseat)
		{
			if (state->current_battle->currentActiveOrganisation !=
			        state->current_battle->currentPlayer &&
			    state->current_battle->currentActiveOrganisation != state->getCivilian())
			{
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH,
				     mksp<MessageBox>(
				         "Next Turn",
				         format("%s, it is your turn!",
				                state->current_battle->currentActiveOrganisation->name),
				         MessageBox::ButtonOptions::Ok, [this] {
					         state->current_battle->currentPlayer =
					             state->current_battle->currentActiveOrganisation;
				         })});
				updateHiddenForm();
				return;
			}
		}

		// Figure out whether our/not our turn state has changed
		bool notMyTurn = endTurnRequested || battle.turnEndAllowed ||
		                 !battle.interruptUnits.empty() || !battle.interruptQueue.empty() ||
		                 battle.currentActiveOrganisation != battle.currentPlayer;
		if (notMyTurn && activeTab != notMyTurnTab)
		{
			setSelectedTab(notMyTurnTab);
			lastSelectedUnits = battle.battleViewSelectedUnits;
			battle.battleViewSelectedUnits.clear();
			updateTBButtons();
		}
		else if (!notMyTurn && activeTab == notMyTurnTab)
		{
			setSelectedTab(mainTab);
			battle.battleViewSelectedUnits = lastSelectedUnits;
			updateTBButtons();
		}

		// Confirmation for units that have unfinished movement
		if (endTurnRequested && !unitPendingConfirmation &&
		    battle.ticksWithoutAction >= TICKS_TRY_END_TURN)
		{
			for (auto &u : battle.units)
			{
				if (u.second->owner != battle.currentActiveOrganisation ||
				    u.second->missions.empty() ||
				    u.second->missions.front()->type != BattleUnitMission::Type::AcquireTU ||
				    !u.second->missions.front()->allowContinue ||
				    unitsSkipped.find(u.second) != unitsSkipped.end())
				{
					continue;
				}
				unitPendingConfirmation = u.second;
				battle.notifyAction();
				unitsSkipped.insert(unitPendingConfirmation);
				fw().stageQueueCommand({StageCmd::Command::PUSH, executePlanPopup});
				break;
			}
			if (!unitPendingConfirmation)
			{
				battle.turnEndAllowed = true;
				endTurnRequested = false;
				unitsSkipped.clear();
			}
		}
	}

	// Battle update
	if (!hideDisplay && !realTime && activeTab == notMyTurnTab &&
	    battle.currentActiveOrganisation != battle.currentPlayer &&
	    battle.ticksWithoutSeenAction[battle.currentPlayer] > TICKS_HIDE_DISPLAY)
	{
		updateHiddenForm();
	}
	unsigned int ticks = 0;
	switch (updateSpeed)
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
	if (hideDisplay)
	{
		ticks = 16;
	}
	while (ticks > 0)
	{
		int ticksPerUpdate = UPDATE_EVERY_TICK ? 1 : hideDisplay ? 4 : ticks;
		state->update(ticksPerUpdate);
		ticks -= ticksPerUpdate;
		if (hideDisplay)
		{
			if (battle.ticksWithoutSeenAction[battle.currentPlayer] == 0)
			{
				if (battle.lastSeenActionLocation[battle.currentPlayer] !=
				    EventMessage::NO_LOCATION)
				{
					fw().pushEvent(
					    new GameLocationEvent(GameEventType::ZoomView,
					                          battle.lastSeenActionLocation[battle.currentPlayer]));
				}
				hideDisplay = false;
				break;
			}
			else if (battle.currentPlayer == battle.currentActiveOrganisation)
			{
				if (!lastSelectedUnits.empty())
				{
					fw().pushEvent(new GameLocationEvent(GameEventType::ZoomView,
					                                     lastSelectedUnits.front()->position));
				}
				hideDisplay = false;
				break;
			}
		}
	}

	updateSelectedUnits();
	updateSelectionMode();
	updateSoldierButtons();

	if (ticksUntilFireSound > 0)
	{
		ticksUntilFireSound--;
	}
	if (leftThrowDelay > 0)
	{
		leftThrowDelay--;
	}
	if (rightThrowDelay > 0)
	{
		rightThrowDelay--;
	}
	// Update preview calculations in TB mode
	if (!realTime)
	{
		if (previewedPathCost == PreviewedPathCostSpecial::NONE)
		{
			pathPreviewTicksAccumulated++;
			// Show path preview if hovering for over half a second
			if (pathPreviewTicksAccumulated > 30)
			{
				updatePathPreview();
			}
		}
		if (calculatedAttackCost == CalculatedAttackCostSpecial::NONE)
		{
			attackCostTicksAccumulated++;
			if (attackCostTicksAccumulated > 5)
			{
				updateAttackCost();
			}
		}
	}

	if (activeTab == mainTab)
	{
		// Update weapons if required
		auto rightInfo = createItemOverlayInfo(true);
		if (rightInfo != rightHandInfo)
		{
			rightHandInfo = rightInfo;
			updateItemInfo(true);
		}
		auto leftInfo = createItemOverlayInfo(false);
		if (leftInfo != leftHandInfo)
		{
			leftHandInfo = leftInfo;
			updateItemInfo(false);
		}
	}

	// Update units and squads
	if (activeTab != notMyTurnTab)
	{
		for (int i = 0; i < 6; i++)
		{
			auto newUnitInfo = createUnitInfo(i);
			if (newUnitInfo != unitInfo[i])
			{
				unitInfo[i] = newUnitInfo;
				updateUnitInfo(i);
			}

			int newSpottedInfo = std::min(
			    6, newUnitInfo.agent ? (int)newUnitInfo.agent->unit->visibleEnemies.size() : 0);
			if (newSpottedInfo != spottedInfo[i])
			{
				spottedInfo[i] = newSpottedInfo;
				updateSpottedInfo(i);
			}

			auto newSquadInfo = createSquadInfo(i);
			if (newSquadInfo != squadInfo[i])
			{
				squadInfo[i] = newSquadInfo;
				updateSquadInfo(i);
			}
		}
	}

	// Update item forms
	for (auto &f : itemForms)
	{
		f->Enabled = false;
	}
	if ((leftHandInfo.selected || rightHandInfo.selected) &&
	    !battle.battleViewSelectedUnits.empty())
	{
		auto unit = battle.battleViewSelectedUnits.front();
		for (int i = 0; i < 2; i++)
		{
			bool right = i == 0;
			if (right ? !rightHandInfo.selected : !leftHandInfo.selected)
			{
				continue;
			}
			auto item = unit->agent->getFirstItemInSlot(right ? EquipmentSlotType::RightHand
			                                                  : EquipmentSlotType::LeftHand);
			if (!item->inUse)
			{
				continue;
			}
			switch (item->type->type)
			{
				case AEquipmentType::Type::MotionScanner:
				{
					motionScannerForms[right]->Enabled = true;

					auto newMotionInfo =
					    realTime ? createMotionInfo(*item->battleScanner) : createMotionInfo();
					if (motionInfo != newMotionInfo)
					{
						motionInfo = newMotionInfo;
						if (realTime)
						{
							updateMotionInfo(right, *item->battleScanner);
						}
						else
						{
							updateMotionInfo(right, motionInfo.position);
						}
					}

					break;
				}
				case AEquipmentType::Type::MediKit:
				{
					medikitForms[right]->Enabled = true;
					for (auto &c : medikitBodyParts[right])
					{
						c.second[false]->setVisible(false);
						c.second[true]->setVisible(false);
					}
					for (auto &p : bodyParts)
					{
						if (unit->fatalWounds[p] > 0)
						{
							medikitBodyParts[right][p]
							                [unit->isHealing && p == unit->healingBodyPart]
							                    ->setVisible(true);
						}
					}
					break;
				}
				default:
					LogError("Using an item other than the motion scanner / medikit?");
			}
		}
	}

	// Update psi form
	if (activeTab == psiTab)
	{
		auto newPsiInfo = createPsiInfo();
		if (psiInfo != newPsiInfo)
		{
			psiInfo = newPsiInfo;
			updatePsiInfo();
		}
	}

	// Update time display
	if (battle.mode == Battle::Mode::RealTime)
	{
		auto clockControl = baseForm->findControlTyped<Label>("CLOCK");
		clockControl->setText(state->gameTime.getLongTimeString());
	}

	// Call forms->update()
	for (auto &f : itemForms)
	{
		if (f->Enabled)
		{
			f->update();
		}
	}
	baseForm->update();

	// If we have 'follow agent' enabled we clobber any other movement in this frame
	if (followAgent && !battle.battleViewSelectedUnits.empty())
	{
		setScreenCenterTile(battle.battleViewSelectedUnits.front()->tileObject->getPosition());
	}
	// Store screen center for serialisation
	battle.battleViewScreenCenter = centerPos;

	if (battle.missionEndTimer > TICKS_END_MISSION)
	{
		UString message;
		if (battle.playerWon || battle.buildingDisabled)
		{
			if ((battle.loserHasRetreated && battle.playerWon) ||
			    (battle.winnerHasRetreated && !battle.playerWon))
			{
				message = tr("All hostile units have fled the combat zone. You win.");
			}
			else if ((battle.loserHasRetreated && !battle.playerWon) ||
			         (battle.winnerHasRetreated && battle.playerWon) ||
			         (!battle.playerWon && !battle.loserHasRetreated && !battle.winnerHasRetreated))
			{
				message = tr("All your units have fled the combat zone. You win.");
			}
			else
			{
				message = tr("All hostile units are dead or unconscious. You win.");
			}
		}
		else if (battle.loserHasRetreated)
		{
			message = tr("All your units have fled the combat zone. You lose.");
		}
		else if (battle.winnerHasRetreated)
		{
			message = tr("All hostile units have fled the combat zone. You lose.");
		}
		else
		{
			message = tr("All your units are unconscious or dead. You lose.");
		}
		fw().stageQueueCommand(
		    {StageCmd::Command::PUSH, mksp<MessageBox>("", message, MessageBox::ButtonOptions::Ok,
		                                               [this] { endBattle(); })});
	}
}

void BattleView::updateSelectedUnits()
{
	if (activeTab == notMyTurnTab)
	{
		resetPathPreview();
		resetAttackCost();
		return;
	}
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
	lastSelectedUnit =
	    battle.battleViewSelectedUnits.empty() ? nullptr : battle.battleViewSelectedUnits.front();

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
		resetAttackCost();
		setSelectedTab(mainTab);
	}
	else
	{
		auto p = lastSelectedUnit->tileObject->getOwningTile()->position;
		auto f = lastSelectedUnit->goalFacing;
		if (lastSelectedUnitPosition != p)
		{
			resetPathPreview();
			resetAttackCost();
		}
		if (lastSelectedUnitFacing != f)
		{
			resetAttackCost();
		}
		lastSelectedUnitPosition = p;
		lastSelectedUnitFacing = f;
	}
}

void BattleView::updateSelectionMode()
{
	if (battle.battleViewSelectedUnits.empty())
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
	// Reset attack cost
	switch (selectionState)
	{
		case BattleSelectionState::Normal:
		{
			// Might have a hostile unit selected
			// If so, don't reset the attack cost so that the TU cost can be displayed
			if (!lastSelectedUnit)
			{
				resetAttackCost();
			}
			else
			{
				auto player = state->current_battle->currentPlayer;
				auto unit = lastSelectedUnit->tileObject->map.getTile(selectedTilePosition)
				                ->getUnitIfPresent(true, true, false, nullptr, false, true);
				auto u = unit ? unit->getUnit() : nullptr;
				if (!u || player->isRelatedTo(u->owner) != Organisation::Relation::Hostile)
				{
					resetAttackCost();
				}
			}
			break;
		}
		case BattleSelectionState::NormalCtrl:
		case BattleSelectionState::NormalCtrlAlt:
		case BattleSelectionState::ThrowLeft:
		case BattleSelectionState::ThrowRight:
		case BattleSelectionState::PsiControl:
		case BattleSelectionState::PsiPanic:
		case BattleSelectionState::PsiStun:
		case BattleSelectionState::PsiProbe:
		case BattleSelectionState::TeleportLeft:
		case BattleSelectionState::TeleportRight:
			resetAttackCost();
			break;
		case BattleSelectionState::NormalAlt:
		case BattleSelectionState::FireAny:
		case BattleSelectionState::FireLeft:
		case BattleSelectionState::FireRight:
			// Fine, don't need to
			break;
	}
	// Change cursor
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
	bool reserve_aimed_set = false;
	bool reserve_snap_set = false;
	bool reserve_auto_set = false;
	bool reserve_kneel_set = false;
	bool reserve_shot_unset = false;
	bool reserve_kneel_unset = false;
	for (auto &u : battle.battleViewSelectedUnits)
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
		switch (u->reserve_shot_mode)
		{
			case ReserveShotMode::Aimed:
				reserve_aimed_set = true;
				break;
			case ReserveShotMode::Snap:
				reserve_snap_set = true;
				break;
			case ReserveShotMode::Auto:
				reserve_auto_set = true;
				break;
			case ReserveShotMode::None:
				reserve_shot_unset = true;
				break;
		}
		if (u->reserve_kneel_mode == KneelingMode::Kneeling)
		{
			reserve_kneel_set = true;
		}
		else
		{
			reserve_kneel_unset = true;
		}
	}

	baseForm->findControlTyped<TriStateBox>("BUTTON_CEASE_FIRE")
	    ->setState(cease_fire && at_will ? 3 : (cease_fire ? 2 : 1));
	baseForm->findControlTyped<CheckBox>("BUTTON_AIMED")->setChecked(aimed);
	baseForm->findControlTyped<CheckBox>("BUTTON_SNAP")->setChecked(snap);
	baseForm->findControlTyped<CheckBox>("BUTTON_AUTO")->setChecked(auto_fire);
	baseForm->findControlTyped<TriStateBox>("BUTTON_KNEEL")
	    ->setState(kneeling && not_kneeling ? 3 : (kneeling ? 2 : 1));
	baseForm->findControlTyped<CheckBox>("BUTTON_PRONE")->setChecked(prone);
	baseForm->findControlTyped<CheckBox>("BUTTON_WALK")->setChecked(walk);
	baseForm->findControlTyped<CheckBox>("BUTTON_RUN")->setChecked(run);
	baseForm->findControlTyped<CheckBox>("BUTTON_EVASIVE")->setChecked(evasive);
	baseForm->findControlTyped<CheckBox>("BUTTON_NORMAL")->setChecked(normal);
	baseForm->findControlTyped<CheckBox>("BUTTON_AGGRESSIVE")->setChecked(aggressive);

	baseForm->findControlTyped<TriStateBox>("BUTTON_RESERVE_AIMED")
	    ->setState(reserve_aimed_set && reserve_shot_unset ? 3 : (reserve_aimed_set ? 2 : 1));
	baseForm->findControlTyped<TriStateBox>("BUTTON_RESERVE_SNAP")
	    ->setState(reserve_snap_set && reserve_shot_unset ? 3 : (reserve_snap_set ? 2 : 1));
	baseForm->findControlTyped<TriStateBox>("BUTTON_RESERVE_AUTO")
	    ->setState(reserve_auto_set && reserve_shot_unset ? 3 : (reserve_auto_set ? 2 : 1));
	baseForm->findControlTyped<TriStateBox>("BUTTON_RESERVE_KNEEL")
	    ->setState(reserve_kneel_set && reserve_kneel_unset ? 3 : (reserve_kneel_set ? 2 : 1));

	bool throwing = !battle.battleViewSelectedUnits.empty() &&
	                battle.battleViewSelectedUnits.front()->isThrowing();

	mainTab->findControlTyped<CheckBox>("BUTTON_LEFT_HAND_THROW")
	    ->setChecked(selectionState == BattleSelectionState::ThrowLeft || leftThrowDelay > 0 ||
	                 throwing);
	mainTab->findControlTyped<CheckBox>("BUTTON_RIGHT_HAND_THROW")
	    ->setChecked(selectionState == BattleSelectionState::ThrowRight || rightThrowDelay > 0 ||
	                 throwing);
}

void BattleView::updateTBButtons()
{
	bool visible = activeTab != notMyTurnTab;
	baseForm->findControlTyped<CheckBox>("BUTTON_FOLLOW_AGENT")->setVisible(visible);
	baseForm->findControlTyped<GraphicButton>("BUTTON_SHOW_OPTIONS")->setVisible(visible);
	baseForm->findControlTyped<TriStateBox>("BUTTON_CEASE_FIRE")->setVisible(visible);
	baseForm->findControlTyped<CheckBox>("BUTTON_AIMED")->setVisible(visible);
	baseForm->findControlTyped<CheckBox>("BUTTON_SNAP")->setVisible(visible);
	baseForm->findControlTyped<CheckBox>("BUTTON_AUTO")->setVisible(visible);
	baseForm->findControlTyped<TriStateBox>("BUTTON_KNEEL")->setVisible(visible);
	baseForm->findControlTyped<CheckBox>("BUTTON_PRONE")->setVisible(visible);
	baseForm->findControlTyped<CheckBox>("BUTTON_WALK")->setVisible(visible);
	baseForm->findControlTyped<CheckBox>("BUTTON_RUN")->setVisible(visible);
	baseForm->findControlTyped<CheckBox>("BUTTON_EVASIVE")->setVisible(visible);
	baseForm->findControlTyped<CheckBox>("BUTTON_NORMAL")->setVisible(visible);
	baseForm->findControlTyped<CheckBox>("BUTTON_AGGRESSIVE")->setVisible(visible);
	baseForm->findControlTyped<TriStateBox>("BUTTON_RESERVE_AIMED")->setVisible(visible);
	baseForm->findControlTyped<TriStateBox>("BUTTON_RESERVE_SNAP")->setVisible(visible);
	baseForm->findControlTyped<TriStateBox>("BUTTON_RESERVE_AUTO")->setVisible(visible);
	baseForm->findControlTyped<TriStateBox>("BUTTON_RESERVE_KNEEL")->setVisible(visible);
	baseForm->findControlTyped<GraphicButton>("BUTTON_ENDTURN")->setVisible(visible);
	baseForm->findControlTyped<RadioButton>("BUTTON_MOVE_GROUP")->setVisible(visible);
	baseForm->findControlTyped<RadioButton>("BUTTON_MOVE_INDIVIDUALLY")->setVisible(visible);
}

void BattleView::updateHiddenForm()
{
	hideDisplay = true;
	hiddenForm->findControlTyped<Label>("TEXT_TURN")->setText(format("%d", battle.currentTurn));
	hiddenForm->findControlTyped<Label>("TEXT_SIDE")
	    ->setText(battle.currentActiveOrganisation->name);
	bool player = state->current_battle->hotseat &&
	              state->current_battle->currentActiveOrganisation != state->getCivilian();
	hiddenForm->findControlTyped<Label>("TEXT_PLAYER")->setText(player ? "Player" : "Computer");
	hiddenForm->findControlTyped<Graphic>("HIDDEN_IMAGE")
	    ->setImage(fw().data->loadImage(pickRandom(state->rng, HIDDEN_BACKGROUNDS)));
	updateHiddenBar();
}

void BattleView::refreshDelayText()
{
	int delay = primingTab->findControlTyped<ScrollBar>("DELAY_SLIDER")->getValue();
	LogWarning("Delay %d", delay);
	UString text;
	if (delay == 0)
	{
		text = format(tr("Activates now."));
	}
	else
	{
		if (battle.mode == Battle::Mode::TurnBased)
		{
			if (delay == 1)
			{
				text = format(tr("Activates at end of turn."));
			}
			else
			{
				text = format("%s %d", tr("Turns before activation:"), delay - 1);
			}
		}
		else
		{
			text = format(tr("Delay = %i"), (int)((float)delay / 4.0f));
		}
	}
	primingTab->findControlTyped<Label>("DELAY_TEXT")->setText(text);
}

void BattleView::refreshRangeText()
{
	int range = primingTab->findControlTyped<ScrollBar>("RANGE_SLIDER")->getValue();

	UString text = format(tr("Range = %2.1fm."), ((float)(range + 1) * 1.5f));
	primingTab->findControlTyped<Label>("RANGE_TEXT")->setText(text);
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

void BattleView::updatePathPreview()
{
	auto target = selectedTilePosition;
	if (!lastSelectedUnit)
	{
		LogError("Trying to update path preview with no unit selected!?");
		return;
	}

	if (!lastSelectedUnit->canMove())
		return;
	auto &map = lastSelectedUnit->tileObject->map;
	auto to = map.getTile(target);

	// Standard check for passability
	while (true)
	{
		auto u = to->getUnitIfPresent(true, true, false, nullptr, false, true);
		auto unit = u ? u->getUnit() : nullptr;
		if (unit && unit->owner != battle.currentPlayer &&
		    battle.visibleUnits[battle.currentPlayer].find({&*state, unit->id}) ==
		        battle.visibleUnits[battle.currentPlayer].end())
		{
			unit = nullptr;
		}
		if (!to->getPassable(lastSelectedUnit->isLarge(),
		                     lastSelectedUnit->agent->type->bodyType->maxHeight) ||
		    unit)
		{
			previewedPathCost = static_cast<int>(PreviewedPathCostSpecial::UNREACHABLE);
			return;
		}
		if (lastSelectedUnit->canFly() || to->getCanStand(lastSelectedUnit->isLarge()))
		{
			break;
		}
		target.z--;
		if (target.z == -1)
		{
			LogError("Solid ground missing on level 0? Reached %d %d %d", target.x, target.y,
			         target.z);
			return;
		}
		to = map.getTile(target);
	}

	// Cost to move is 1.5x if prone and 0.5x if running, to keep things in integer
	// we use a value that is then divided by 2
	float cost = 0.0f;
	int cost_multiplier_x_2 = 2;
	if (lastSelectedUnit->agent->canRun() &&
	    lastSelectedUnit->movement_mode == MovementMode::Running)
	{
		cost_multiplier_x_2 = 1;
	}
	if (lastSelectedUnit->movement_mode == MovementMode::Prone)
	{
		cost_multiplier_x_2 = 3;
	}

	// Get path
	float maxCost =
	    (float)lastSelectedUnit->agent->modified_stats.time_units * 2 / cost_multiplier_x_2;
	pathPreview = map.findShortestPath(lastSelectedUnit->goalPosition, target, 1000,
	                                   BattleUnitTileHelper{map, *lastSelectedUnit}, false, false,
	                                   true, false, &cost, maxCost);
	if (pathPreview.empty())
	{
		LogError("Empty path returned for path preview!?");
		return;
	}
	// If we have not reached the target - then show "Too Far"
	// Otherwise, show amount of TUs remaining at arrival
	if (pathPreview.back() != target)
	{
		previewedPathCost = static_cast<int>(PreviewedPathCostSpecial::TOO_FAR);
	}
	else
	{
		previewedPathCost = (int)roundf(cost * cost_multiplier_x_2 / 2);
		previewedPathCost = lastSelectedUnit->agent->modified_stats.time_units - previewedPathCost;
		if (previewedPathCost < 0)
		{
			// Sometimes it might happen that we barely miss goal after all calculations
			// In this case, properly display "Too far" and subtract cost
			previewedPathCost = static_cast<int>(PreviewedPathCostSpecial::TOO_FAR);
			pathPreview.pop_back();
		}
	}
	if (pathPreview.front() == (Vec3<int>)lastSelectedUnit->position)
	{
		pathPreview.pop_front();
	}
}

void BattleView::updateAttackCost()
{
	auto target = selectedTilePosition;
	if (!lastSelectedUnit)
	{
		LogError("Trying to update path attack cost with no unit selected!?");
		return;
	}
	WeaponStatus status;
	switch (selectionState)
	{
		case BattleSelectionState::FireLeft:
			status = WeaponStatus::FiringLeftHand;
			break;
		case BattleSelectionState::FireRight:
			status = WeaponStatus::FiringRightHand;
			break;
		case BattleSelectionState::Normal:
		case BattleSelectionState::NormalAlt:
		case BattleSelectionState::FireAny:
			status = WeaponStatus::FiringBothHands;
			break;
		default:
			status = WeaponStatus::NotFiring;
			break;
	}
	if (status == WeaponStatus::FiringBothHands)
	{
		// Right hand has priority
		auto rhItem = lastSelectedUnit->agent->getFirstItemInSlot(EquipmentSlotType::RightHand);
		if (rhItem && rhItem->canFire(*state))
		{
			status = WeaponStatus::FiringRightHand;
		}
		else
		{
			// We don't care what's in the left hand,
			// we will just cancel firing in update() if there's nothing to fire
			status = WeaponStatus::FiringLeftHand;
		}
	}
	auto weapon = (status == WeaponStatus::FiringRightHand)
	                  ? lastSelectedUnit->agent->getFirstItemInSlot(EquipmentSlotType::RightHand)
	                  : lastSelectedUnit->agent->getFirstItemInSlot(EquipmentSlotType::LeftHand);
	if (!weapon)
	{
		calculatedAttackCost = static_cast<int>(CalculatedAttackCostSpecial::NO_WEAPON);
	}
	else if (!weapon->canFire(*state, target))
	{
		calculatedAttackCost =
		    static_cast<int>(weapon->type->launcher ? CalculatedAttackCostSpecial::NO_ARC
		                                            : CalculatedAttackCostSpecial::OUT_OF_RANGE);
	}
	else
	{
		calculatedAttackCost =
		    lastSelectedUnit->getAttackCost(*state, *weapon, selectedTilePosition);
	}
}

void BattleView::updateSquadIndex(StateRef<BattleUnit> u)
{
	if (u->squadNumber != -1)
	{
		battle.battleViewSquadIndex = u->squadNumber;
	}
}

void BattleView::debugVortex()
{
	auto vortex = StateRef<AEquipmentType>(state.get(), "AEQUIPMENTTYPE_VORTEX_MINE");
	state->current_battle->addExplosion(
	    *state, selectedTilePosition, vortex->explosion_graphic, vortex->damage_type,
	    vortex->damage, vortex->explosion_depletion_rate, state->current_battle->currentPlayer);
}

void BattleView::debugShot(Vec3<float> velocity)
{
	auto blaster = StateRef<AEquipmentType>(state.get(), "AEQUIPMENTTYPE_DEBUGGER_CANNON");
	fw().soundBackend->playSample(blaster->fire_sfx, selectedTilePosition);
	velocity *= blaster->speed * PROJECTILE_VELOCITY_MULTIPLIER;
	Vec3<float> position = {0.5f, 0.5f, 0.5f};
	position += selectedTilePosition;
	auto p = mksp<Projectile>(
	    blaster->guided ? Projectile::Type::Missile : Projectile::Type::Beam,
	    StateRef<BattleUnit>(state.get(), state->current_battle->units.begin()->first),
	    StateRef<BattleUnit>(), position + velocity, position, velocity, blaster->turn_rate,
	    blaster->ttl, blaster->damage, blaster->projectile_delay, blaster->explosion_depletion_rate,
	    blaster->tail_size, blaster->projectile_sprites, blaster->impact_sfx,
	    blaster->explosion_graphic, blaster->damage_type);
	state->current_battle->map->addObjectToMap(p);
	state->current_battle->projectiles.insert(p);
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
	temp.position = u->position;
	int facingDelta = strafe ? BattleUnitMission::getFacingDelta(
	                               u->facing, BattleUnitMission::getFacing(temp, target))
	                         : 0;

	if (battle.battleViewGroupMove && battle.battleViewSelectedUnits.size() > 1 && !runAway)
	{
		battle.groupMove(*state, battle.battleViewSelectedUnits, target, facingDelta,
		                 demandGiveWay);
	}
	else
	{
		for (auto &unit : battle.battleViewSelectedUnits)
		{
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
				LogInfo("BattleUnit \"%s\" going to location %s", unit->agent->name, target);
			}
			else
			{
				LogInfo("BattleUnit \"%s\" could not receive order to move", unit->agent->name);
			}
		}
	}
}

void BattleView::orderTurn(Vec3<int> target)
{
	for (auto &unit : battle.battleViewSelectedUnits)
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
	if (battle.battleViewSelectedUnits.empty())
	{
		return;
	}
	auto unit = battle.battleViewSelectedUnits.front();
	auto item = unit->agent->getFirstItemInSlot(right ? EquipmentSlotType::RightHand
	                                                  : EquipmentSlotType::LeftHand);
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
	if (battle.battleViewSelectedUnits.empty())
	{
		return;
	}
	auto unit = battle.battleViewSelectedUnits.front();
	auto item = unit->agent->getFirstItemInSlot(right ? EquipmentSlotType::RightHand
	                                                  : EquipmentSlotType::LeftHand);

	if (!item)
	{
		return;
	}
	if (!item->canBeUsed(*state))
	{
		auto message_box = mksp<MessageBox>(
		    tr("Alien Artifact"), tr("You must research Alien technology before you can use it."),
		    MessageBox::ButtonOptions::Ok);
		fw().stageQueueCommand({StageCmd::Command::PUSH, message_box});
		return;
	}

	switch (item->type->type)
	{
		case AEquipmentType::Type::Weapon:
			// Weapon has no automatic mode
			if (!item->canFire(*state) || automatic)
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
				selectionState =
				    right ? BattleSelectionState::ThrowRight : BattleSelectionState::ThrowLeft;
			}
			else
			{
				if (item->primed)
				{
					break;
				}
				setSelectedTab(primingTab);
				refreshDelayText();
				refreshRangeText();
				activeTab->findControlTyped<CheckBox>("HIDDEN_CHECK_RIGHT_HAND")->setChecked(right);

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
				activeTab->findControl("RANGE_TEXT")->setVisible(range);
				activeTab->findControl("RANGE_SLIDER")->setVisible(range);
				activeTab->findControl("RANGE_SLIDER_LEFT")->setVisible(range);
				activeTab->findControl("RANGE_SLIDER_MIDDLE")->setVisible(range);
				activeTab->findControl("RANGE_SLIDER_RIGHT")->setVisible(range);
			}
			break;
		case AEquipmentType::Type::MindBender:
			// Mind bender does not care for automatic mode
			selectionState = BattleSelectionState::Normal;
			setSelectedTab(psiTab);
			activeTab->findControlTyped<RadioButton>("BUTTON_CONTROL")->setChecked(false);
			activeTab->findControlTyped<RadioButton>("BUTTON_PANIC")->setChecked(false);
			activeTab->findControlTyped<RadioButton>("BUTTON_STUN")->setChecked(false);
			activeTab->findControlTyped<RadioButton>("BUTTON_PROBE")->setChecked(false);
			activeTab->findControlTyped<CheckBox>("RIGHTHANDUSED")->setChecked(right);
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

void BattleView::openAgentInventory()
{
	if (battle.battleViewSelectedUnits.empty())
	{
		return;
	}
	fw().stageQueueCommand(
	    {StageCmd::Command::PUSH,
	     mksp<AEquipScreen>(state, battle.battleViewSelectedUnits.front()->agent)});
}

void BattleView::orderDrop(bool right)
{
	if (battle.battleViewSelectedUnits.empty())
	{
		return;
	}
	auto unit = battle.battleViewSelectedUnits.front();

	if (!unit->agent->type->inventory)
	{
		return;
	}

	auto item = unit->agent->getFirstItemInSlot(right ? EquipmentSlotType::RightHand
	                                                  : EquipmentSlotType::LeftHand);
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
		if (!unit->spendTU(*state, unit->getPickupCost()))
		{
			return;
		}
		auto item = items.front();
		unit->agent->addEquipment(
		    *state, item->item, right ? EquipmentSlotType::RightHand : EquipmentSlotType::LeftHand);
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
					updateSquadIndex(u);
				}
			}
			else
			{
				// Unit not in selection => replace selection with unit
				battle.battleViewSelectedUnits.clear();
				battle.battleViewSelectedUnits.push_back(u);
				updateSquadIndex(u);
			}
		}
		// Unit is selected
		else
		{
			// Unit in selection and additive => move unit to front
			if (additive)
			{
				battle.battleViewSelectedUnits.erase(pos);
				battle.battleViewSelectedUnits.push_front(u);
				updateSquadIndex(u);
			}
			// If not additive and in selection - select only this unit
			else if (battle.battleViewSelectedUnits.size() > 1)
			{
				battle.battleViewSelectedUnits.clear();
				battle.battleViewSelectedUnits.push_front(u);
				updateSquadIndex(u);
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
	if (battle.battleViewSelectedUnits.empty())
	{
		return;
	}
	auto unit = battle.battleViewSelectedUnits.front();
	auto item = unit->agent->getFirstItemInSlot(right ? EquipmentSlotType::RightHand
	                                                  : EquipmentSlotType::LeftHand);

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
	bool atGround = modifier || !config().getBool("OpenApoc.NewFeature.AllowForceFiringParallel");
	for (auto &unit : battle.battleViewSelectedUnits)
	{
		unit->startAttacking(*state, target, status, atGround);
	}
}

void BattleView::orderFire(StateRef<BattleUnit> shooter, Vec3<int> target, WeaponStatus status)
{
	shooter->startAttacking(*state, target, status);
}

void BattleView::orderFire(StateRef<BattleUnit> u, WeaponStatus status, bool forced)
{
	if (battle.battleViewSelectedUnits.empty())
	{
		return;
	}
	bool noLOF = false;
	if (status == WeaponStatus::FiringBothHands)
	{
		for (auto &unit : battle.battleViewSelectedUnits)
		{
			if (unit->hasLineToUnit(u))
			{
				unit->startAttacking(*state, u, status);
			}
			else
			{
				if (forced)
				{
					orderFire(unit, u->position, status);
				}
				else
				{
					noLOF = true;
				}
			}
		}
	}
	else
	{
		auto unit = battle.battleViewSelectedUnits.front();
		if (unit->hasLineToUnit(u))
		{
			unit->startAttacking(*state, u, status);
		}
		else
		{
			if (forced)
			{
				orderFire(unit, u->position, status);
			}
			else
			{
				noLOF = true;
			}
		}
	}
	if (noLOF)
	{
		u->sendAgentEvent(*state, GameEventType::NoLOF, true);
	}
}

void BattleView::orderFocus(StateRef<BattleUnit> u)
{
	for (auto &unit : battle.battleViewSelectedUnits)
	{
		unit->setFocus(*state, u);
	}
}

void BattleView::orderCancelPsi()
{
	if (battle.battleViewSelectedUnits.empty())
	{
		return;
	}

	battle.battleViewSelectedUnits.front()->stopAttackPsi(*state);
}

void BattleView::orderPsiAttack(StateRef<BattleUnit> u, PsiStatus status, bool right)
{
	if (battle.battleViewSelectedUnits.empty())
	{
		return;
	}

	auto unit = battle.battleViewSelectedUnits.front();
	auto item = unit->agent->getFirstItemInSlot(right ? EquipmentSlotType::RightHand
	                                                  : EquipmentSlotType::LeftHand);

	if (unit->startAttackPsi(*state, u, status, item->type))
	{
		activeTab->findControlTyped<RadioButton>("BUTTON_CONTROL")->setChecked(false);
		activeTab->findControlTyped<RadioButton>("BUTTON_PANIC")->setChecked(false);
		activeTab->findControlTyped<RadioButton>("BUTTON_STUN")->setChecked(false);
		activeTab->findControlTyped<RadioButton>("BUTTON_PROBE")->setChecked(false);
		selectionState = BattleSelectionState::Normal;
	}
}

void BattleView::orderHeal(BodyPart part)
{
	auto unit = battle.battleViewSelectedUnits.front();

	unit->useMedikit(*state, part);
}

void BattleView::eventOccurred(Event *e)
{
	baseForm->eventOccured(e);
	bool eventWithin = false;
	for (auto &f : itemForms)
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
	// Exclude mouse down events that are over the form
	if (eventWithin || activeTab->eventIsWithin(e) || baseForm->eventIsWithin(e))
	{
		// We pass this event so that scroll can work
		if (e->type() != EVENT_MOUSE_MOVE)
		{
			return;
		}
	}

	switch (e->type())
	{
		case EVENT_MOUSE_MOVE:
		{
			Vec2<float> screenOffset = {getScreenOffset().x, getScreenOffset().y};
			// Offset by 4 since ingame 4 is the typical height of the ground, and game displays
			// cursor
			// on top of the ground
			setSelectedTilePosition(screenToTileCoords(
			    Vec2<float>((float)e->mouse().X, (float)e->mouse().Y + 4) - screenOffset,
			    (float)getZLevel() - 1.0f));
			// do not return, pass on to allow scrolling
			break;
		}
		case EVENT_KEY_DOWN:
			if (handleKeyDown(e))
			{
				return;
			}
			break;
		case EVENT_KEY_UP:
			if (handleKeyUp(e))
			{
				return;
			}
			break;
		case EVENT_MOUSE_DOWN:
			if (handleMouseDown(e))
			{
				return;
			}
			break;
		case EVENT_GAME_STATE:
			if (handleGameStateEvent(e))
			{
				return;
			}
			break;
		default:
			// Other events aren't handled
			break;
	}
	BattleTileView::eventOccurred(e);
}

bool BattleView::handleKeyDown(Event *e)
{
	// Common keys active in both debug and normal mode
	switch (e->keyboard().KeyCode)
	{
		case SDLK_RSHIFT:
			modifierRShift = true;
			return true;
		case SDLK_LSHIFT:
			modifierLShift = true;
			return true;
		case SDLK_RALT:
			modifierRAlt = true;
			return true;
		case SDLK_LALT:
			modifierLAlt = true;
			return true;
		case SDLK_RCTRL:
			modifierRCtrl = true;
			return true;
		case SDLK_LCTRL:
			modifierLCtrl = true;
			return true;
		case SDLK_ESCAPE:
			if (activeTab != notMyTurnTab)
			{
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH, mksp<InGameOptions>(state->shared_from_this())});
			}
			return true;
		case SDLK_TAB:
			baseForm->findControl("BUTTON_TOGGLE_STRATMAP")->click();
			return true;
		case SDLK_PAGEUP:
			setZLevel(getZLevel() + 1);
			setSelectedTilePosition(
			    {selectedTilePosition.x, selectedTilePosition.y, selectedTilePosition.z + 1});
			updateLayerButtons();
			return true;
		case SDLK_PAGEDOWN:
			setZLevel(getZLevel() - 1);
			setSelectedTilePosition(
			    {selectedTilePosition.x, selectedTilePosition.y, selectedTilePosition.z - 1});
			updateLayerButtons();
			return true;
		case SDLK_SPACE:
			if (updateSpeed != BattleUpdateSpeed::Pause)
				setUpdateSpeed(BattleUpdateSpeed::Pause);
			else
				setUpdateSpeed(lastSpeed);
			return true;
		default:
			break;
	}
	// Debug keys (cheats)
	if (debugHotkeyMode)
	{
		switch (e->keyboard().KeyCode)
		{
			// Force re-link supports
			case SDLK_f:
			{
				auto t = getSelectedTilePosition();
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
						auto set = mksp<std::set<SupportedMapPart *>>();
						set->insert(mp.get());
						mp->queueCollapse();
						SupportedMapPart::attemptReLinkSupports(set);
					}
				}
				return true;
			}
			// Reveal map
			case SDLK_r:
			{
				revealWholeMap = !revealWholeMap;
				return true;
			}
			//  Reset ai movement order
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
				return true;
			}
			// Stun units
			case SDLK_s:
			{
				bool inverse = modifierLShift || modifierRShift;
				bool local = !(modifierLCtrl || modifierRCtrl);
				for (auto &u : battle.units)
				{
					if (u.second->isDead() || u.second->retreated)
					{
						continue;
					}

					if (((local && u.second->tileObject->getOwningTile()->position ==
					                   selectedTilePosition) ||
					     (!local && glm::length(u.second->position -
					                            (Vec3<float>)selectedTilePosition) < 5.0f)) ==
					    !inverse)
					{
						u.second->applyDamageDirect(*state, 9001, false, BodyPart::Helmet,
						                            u.second->agent->getHealth() + 4);
					}
				}
				return true;
			}
			// Retreat units
			case SDLK_k:
			{
				bool inverse = modifierLShift || modifierRShift;
				bool local = !(modifierLCtrl || modifierRCtrl);
				for (auto &u : battle.units)
				{
					if (u.second->isDead() || u.second->retreated)
					{
						continue;
					}

					if (((local && u.second->tileObject->getOwningTile()->position ==
					                   selectedTilePosition) ||
					     (!local && glm::length(u.second->position -
					                            (Vec3<float>)selectedTilePosition) < 5.0f)) ==
					    !inverse)
					{
						if (!u.second->retreated)
						{
							u.second->retreat(*state);
						}
					}
				}
				return true;
			}
			// Panic / Amplify psi
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
				return true;
			}
			// Heal everybody
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
				return true;
			}
			// Restore TUs
			case SDLK_t:
			{
				LogWarning("Restoring TU");
				for (auto &u : battle.units)
				{
					if (!u.second->isConscious() ||
					    u.second->owner != battle.currentActiveOrganisation)
					{
						continue;
					}
					u.second->agent->modified_stats.time_units = u.second->initialTU;
				}
				return true;
			}
			// End turn TB
			case SDLK_e:
				if (battle.mode == Battle::Mode::TurnBased)
				{
					battle.interruptUnits.clear();
					for (auto &u : battle.units)
					{
						if (u.second->owner != battle.currentActiveOrganisation ||
						    !u.second->isConscious())
						{
							continue;
						}
						u.second->cancelMissions(*state);
					}
					battle.endTurn(*state);
				}
				return true;
			// Blow debug vortex
			case SDLK_KP_0:
				debugVortex();
				return true;
			// Fire debug shot
			case SDLK_KP_1:
				debugShot({0, 1, 0});
				return true;
			case SDLK_KP_2:
				debugShot({1, 1, 0});
				return true;
			case SDLK_KP_3:
				debugShot({1, 0, 0});
				return true;
			case SDLK_KP_6:
				debugShot({1, -1, 0});
				return true;
			case SDLK_KP_9:
				debugShot({0, -1, 0});
				return true;
			case SDLK_KP_8:
				debugShot({-1, -1, 0});
				return true;
			case SDLK_KP_7:
				debugShot({-1, 0, 0});
				return true;
			case SDLK_KP_4:
				debugShot({-1, 1, 0});
				return true;
			case SDLK_KP_5:
				debugShot({0, 0, -1});
				return true;
			default:
				break;
		}
	}
	// Normal game hotkeys
	else
	{
		switch (e->keyboard().KeyCode)
		{
			case SDLK_v:
				baseForm->findControl("BUTTON_LAYERING")->click();
				return true;
			case SDLK_c:
				baseForm->findControl("BUTTON_FOLLOW_AGENT")->click();
				return true;
			case SDLK_F9:
				baseForm->findControl("BUTTON_EVASIVE")->click();
				return true;
			case SDLK_F10:
				baseForm->findControl("BUTTON_NORMAL")->click();
				return true;
			case SDLK_F11:
				baseForm->findControl("BUTTON_AGGRESSIVE")->click();
				return true;
			case SDLK_F2:
				baseForm->findControl("BUTTON_PRONE")->click();
				return true;
			case SDLK_F3:
				baseForm->findControl("BUTTON_WALK")->click();
				return true;
			case SDLK_F4:
				baseForm->findControl("BUTTON_RUN")->click();
				return true;
			case SDLK_F5:
				baseForm->findControl("BUTTON_CEASE_FIRE")->click();
				return true;
			case SDLK_F6:
				baseForm->findControl("BUTTON_AIMED")->click();
				return true;
			case SDLK_F7:
				baseForm->findControl("BUTTON_SNAP")->click();
				return true;
			case SDLK_F8:
				baseForm->findControl("BUTTON_AUTO")->click();
				return true;
			case SDLK_BACKSPACE:
				baseForm->findControl("BUTTON_KNEEL")->click();
				return true;
			case SDLK_m:
				baseForm->findControl("BUTTON_SHOW_LOG")->click();
				return true;
			case SDLK_HOME:
				baseForm->findControl("BUTTON_ZOOM_EVENT")->click();
				return true;
			case SDLK_1:
				if (modifierLShift || modifierRShift)
				{
					baseForm->findControl("UNIT_1")->click();
				}
				else if (modifierLAlt || modifierRAlt)
				{
					baseForm->findControl("UNIT_1_HOSTILES")->click();
				}
				else
				{
					baseForm->findControl("SQUAD_1_OVERLAY")->click();
				}
				return true;
			case SDLK_2:
				if (modifierLShift || modifierRShift)
				{
					baseForm->findControl("UNIT_2")->click();
				}
				else if (modifierLAlt || modifierRAlt)
				{
					baseForm->findControl("UNIT_2_HOSTILES")->click();
				}
				else
				{
					baseForm->findControl("SQUAD_2_OVERLAY")->click();
				}
				return true;
			case SDLK_3:
				if (modifierLShift || modifierRShift)
				{
					baseForm->findControl("UNIT_3")->click();
				}
				else if (modifierLAlt || modifierRAlt)
				{
					baseForm->findControl("UNIT_3_HOSTILES")->click();
				}
				else
				{
					baseForm->findControl("SQUAD_3_OVERLAY")->click();
				}
				return true;
			case SDLK_4:
				if (modifierLShift || modifierRShift)
				{
					baseForm->findControl("UNIT_4")->click();
				}
				else if (modifierLAlt || modifierRAlt)
				{
					baseForm->findControl("UNIT_4_HOSTILES")->click();
				}
				else
				{
					baseForm->findControl("SQUAD_4_OVERLAY")->click();
				}
				return true;
			case SDLK_5:
				if (modifierLShift || modifierRShift)
				{
					baseForm->findControl("UNIT_5")->click();
				}
				else if (modifierLAlt || modifierRAlt)
				{
					baseForm->findControl("UNIT_5_HOSTILES")->click();
				}
				else
				{
					baseForm->findControl("SQUAD_5_OVERLAY")->click();
				}
				return true;
			case SDLK_6:
				if (modifierLShift || modifierRShift)
				{
					baseForm->findControl("UNIT_6")->click();
				}
				else if (modifierLAlt || modifierRAlt)
				{
					baseForm->findControl("UNIT_6_HOSTILES")->click();
				}
				else
				{
					baseForm->findControl("SQUAD_6_OVERLAY")->click();
				}
				return true;
			case SDLK_RETURN:
			case SDLK_KP_ENTER:
				if (activeTab == mainTab)
				{
					activeTab->findControl("BUTTON_INVENTORY")->click();
				}
				return true;
			case SDLK_LEFTBRACKET:
				if (activeTab == mainTab)
				{
					activeTab->findControl("BUTTON_LEFT_HAND_THROW")->click();
				}
				return true;
			case SDLK_RIGHTBRACKET:
				if (activeTab == mainTab)
				{
					activeTab->findControl("BUTTON_RIGHT_HAND_THROW")->click();
				}
				return true;
			case SDLK_BACKSLASH:
				if (activeTab == mainTab)
				{
					activeTab->findControl("BUTTON_LEFT_HAND_DROP")->click();
				}
				return true;
			case SDLK_QUOTE:
				if (activeTab == mainTab)
				{
					activeTab->findControl("BUTTON_RIGHT_HAND_DROP")->click();
				}
				return true;
			case SDLK_y:
				if (activeTab == primingTab)
				{
					activeTab->findControl("BUTTON_OK")->click();
				}
				return true;
			case SDLK_n:
				if (activeTab == primingTab)
				{
					activeTab->findControl("BUTTON_CANCEL")->click();
				}
				return true;
			case SDLK_e:
				if (battle.mode == Battle::Mode::TurnBased)
				{
					baseForm->findControl("BUTTON_ENDTURN")->click();
				}
				return true;
			case SDLK_s:
				if (activeTab == notMyTurnTab)
				{
					return true;
				}
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH, mksp<SaveMenu>(SaveMenuAction::Save, state)});
				return true;
			case SDLK_l:
				if (activeTab == notMyTurnTab)
				{
					return true;
				}
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH, mksp<SaveMenu>(SaveMenuAction::Load, state)});
				return true;
			case SDLK_j:
			{
				if (!battle.battleViewSelectedUnits.empty())
				{
					auto t = getSelectedTilePosition();
					orderJump(t);
				}
				return true;
			}
		}
	}
	return false;
}

bool BattleView::handleKeyUp(Event *e)
{
	switch (e->keyboard().KeyCode)
	{
		case SDLK_RSHIFT:
			modifierRShift = false;
			return true;
		case SDLK_LSHIFT:
			modifierLShift = false;
			return true;
		case SDLK_RALT:
			modifierRAlt = false;
			return true;
		case SDLK_LALT:
			modifierLAlt = false;
			return true;
		case SDLK_RCTRL:
			modifierRCtrl = false;
			return true;
		case SDLK_LCTRL:
			modifierLCtrl = false;
			return true;
	}
	return false;
}

bool BattleView::handleMouseDown(Event *e)
{
	if (activeTab == notMyTurnTab)
	{
		return true;
	}

	if (!debugHotkeyMode && Event::isPressed(e->mouse().Button, Event::MouseButton::Middle))
	{
		Vec2<float> screenOffset = {getScreenOffset().x, getScreenOffset().y};
		auto clickTile =
		    screenToTileCoords(Vec2<float>{e->mouse().X, e->mouse().Y} - screenOffset, 0.0f);
		setScreenCenterTile(Vec2<float>{clickTile.x, clickTile.y});
		return true;
	}
	// CHEAT - move unit to mouse
	if (Event::isPressed(e->mouse().Button, Event::MouseButton::Middle) && debugHotkeyMode)
	{
		if (!battle.battleViewSelectedUnits.empty())
		{
			selectionState = BattleSelectionState::TeleportLeft;
		}
		return true;
	}
	if (Event::isPressed(e->mouse().Button, Event::MouseButton::Left) ||
	    Event::isPressed(e->mouse().Button, Event::MouseButton::Right))
	{
		auto buttonPressed = Event::isPressed(e->mouse().Button, Event::MouseButton::Left)
		                         ? Event::MouseButton::Left
		                         : Event::MouseButton::Right;

		auto player = state->current_battle->currentPlayer;
		// If a click has not been handled by a form it's in the map.
		auto t = getSelectedTilePosition();
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
			for (auto &u : objsOccupying)
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
			for (auto &u : objsPresent)
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
		if (!attackTarget && unitPresent &&
		    (unitPresent->owner == player ||
		     battle.visibleUnits[player].find({&*state, unitPresent->id}) !=
		         battle.visibleUnits[player].end()))
		{
			attackTarget = unitPresent;
		}
		// Determine selection/deselection target
		sp<BattleUnit> selectionTarget = nullptr;
		if (!selectionTarget)
		{
			for (auto &u : objsOccupying)
			{
				if (player == u->owner && u->moraleState == MoraleState::Normal &&
				    u->agent->type->allowsDirectControl)
				{
					selectionTarget = u;
					break;
				}
			}
		}
		if (!selectionTarget)
		{
			for (auto &u : objsPresent)
			{
				if (player == u->owner && u->moraleState == MoraleState::Normal &&
				    u->agent->type->allowsDirectControl)
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
						// Show psi probe screen if under probe
						else if (!unitOccupying->psiAttackers.empty())
						{
							for (auto &pa : unitOccupying->psiAttackers)
							{
								if (pa.second == PsiStatus::Probe)
								{
									StateRef<BattleUnit> attacker = {&*state, pa.first};
									if (attacker->owner == state->current_battle->currentPlayer)
									{
										fw().stageQueueCommand(
										    {StageCmd::Command::PUSH,
										     mksp<AEquipScreen>(state, unitOccupying->agent)});
									}
								}
							}
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
										orderFire({&*state, attackTarget->id},
										          WeaponStatus::FiringBothHands,
										          modifierLShift || modifierRShift);
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
					debug += format("\n STAND %d PASS %d", (int)tile->canStand,
					                (int)tile->getPassable());
					for (auto &o : tile->ownedObjects)
					{
						if (o->getType() == TileObject::Type::Ground ||
						    o->getType() == TileObject::Type::Feature ||
						    o->getType() == TileObject::Type::LeftWall ||
						    o->getType() == TileObject::Type::RightWall)
						{
							auto mp =
							    std::static_pointer_cast<TileObjectBattleMapPart>(o)->getOwner();
							debug += format(
							    "\n[%s] SBT %d STATUS %s\nFIRE Res=%d Tim=%d Burned=%D",
							    mp->type.id, mp->type->getVanillaSupportedById(),
							    !mp->isAlive()
							        ? "DEAD "
							        : (mp->damaged ? "DAMAGED"
							                       : (mp->providesHardSupport ? "HARD " : "SOFT ")),
							    mp->type->fire_resist, mp->type->fire_burn_time,
							    mp->burnTicksAccumulated);
							for (int x = t.x - 1; x <= t.x + 1; x++)
							{
								for (int y = t.y - 1; y <= t.y + 1; y++)
								{
									for (int z = t.z - 1; z <= t.z + 1; z++)
									{
										if (x < 0 || x >= map.size.x || y < 0 || y >= map.size.y ||
										    z < 0 || z >= map.size.z)
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
													if (p.first == t && p.second == mp->type->type)
													{
														debug +=
														    format("\nSupported by %s at %d %d %d",
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
							auto h =
							    std::static_pointer_cast<TileObjectBattleHazard>(o)->getHazard();
							debug +=
							    format("\nHazard %s %s Pow=%d Age=%d LT=%d  ", h->damageType.id,
							           h->damageType->hazardType.id, h->power, h->age, h->lifetime);
						}
					}

					auto uto = tile->getUnitIfPresent();
					if (uto)
					{
						auto u = uto->getUnit();
						debug += format("\nContains unit %s.", u->id.c_str());
						debug += format("\nMorale state: %d", (int)u->moraleState);
						debug += format("\nPosition: %f, %f, %f", u->position.x, u->position.y,
						                u->position.z);
						debug += format("\nGoal: %f, %f, %f", u->goalPosition.x, u->goalPosition.y,
						                u->goalPosition.z);
						debug += format("\nCurrent movement: %d, falling: %d",
						                (int)u->current_movement_state, (int)u->falling);
						debug += format("\nItems [%d]:", (int)u->agent->equipment.size());
						for (auto &e : u->agent->equipment)
						{
							debug += format("\n%s", e->type.id);
						}
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
						bool modified = (modifierLAlt || modifierRAlt);
						if (modified)
						{
							orderFire(t, status, modified);
						}
						else if (attackTarget)
						{
							orderFire({&*state, attackTarget->id}, status,
							          modifierRShift || modifierLShift);
						}
						else
						{
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
							    activeTab->findControlTyped<CheckBox>("RIGHTHANDUSED")->isChecked();
							switch (selectionState)
							{
								case BattleSelectionState::PsiControl:
									orderPsiAttack({&*state, attackTarget->id}, PsiStatus::Control,
									               right);
									break;
								case BattleSelectionState::PsiPanic:
									orderPsiAttack({&*state, attackTarget->id}, PsiStatus::Panic,
									               right);
									break;
								case BattleSelectionState::PsiStun:
									orderPsiAttack({&*state, attackTarget->id}, PsiStatus::Stun,
									               right);
									break;
								case BattleSelectionState::PsiProbe:
									orderPsiAttack({&*state, attackTarget->id}, PsiStatus::Probe,
									               right);
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
						if (activeTab != psiTab)
						{
							LogError("How come are we in psi mode but not in psi tab?");
						}
						else
						{
							activeTab->findControlTyped<RadioButton>("BUTTON_CONTROL")
							    ->setChecked(false);
							activeTab->findControlTyped<RadioButton>("BUTTON_PANIC")
							    ->setChecked(false);
							activeTab->findControlTyped<RadioButton>("BUTTON_STUN")
							    ->setChecked(false);
							activeTab->findControlTyped<RadioButton>("BUTTON_PROBE")
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
		return true;
	}
	return true;
}

bool BattleView::handleGameStateEvent(Event *e)
{
	auto gameEvent = dynamic_cast<GameEvent *>(e);
	if (!gameEvent)
	{
		LogError("Invalid game state event");
		return true;
	}
	if (!gameEvent->message().empty())
	{
		state->logEvent(gameEvent);
		baseForm->findControlTyped<Ticker>("NEWS_TICKER")->addMessage(gameEvent->message());
		if (battle.mode == Battle::Mode::RealTime)
		{
			bool pause = false;
			if (GameEvent::optionsMap.find(gameEvent->type) != GameEvent::optionsMap.end())
			{
				pause = config().getBool(GameEvent::optionsMap.at(gameEvent->type));
			}
			if (pause)
			{
				fw().stageQueueCommand({StageCmd::Command::PUSH,
				                        mksp<NotificationScreen>(state, *this, gameEvent->message(),
				                                                 gameEvent->type)});
			}
		}
	}
	switch (gameEvent->type)
	{
		case GameEventType::ZoomView:
			if (GameLocationEvent *gle = dynamic_cast<GameLocationEvent *>(gameEvent))
			{
				zoomAt(gle->location);
			}
			break;
		case GameEventType::AgentPsiProbed:
		{
			auto gameAgentEvent = dynamic_cast<GameAgentEvent *>(e);
			fw().stageQueueCommand(
			    {StageCmd::Command::PUSH, mksp<AEquipScreen>(state, gameAgentEvent->agent)});
			break;
		}
		case GameEventType::HostileDied:
		case GameEventType::AgentDiedBattle:
		case GameEventType::AgentUnconscious:
			// FIXME: there is no event on BattleMapPart destruction, so we cannot detect this and
			// update the path preview
			// a battleunit has died, potentially unblocking/changing the previewed path
			// but only update if there's a path stored
			if (previewedPathCost != PreviewedPathCostSpecial::NONE)
			{
				updatePathPreview();
			}
		default:
			break;
	}
	return true;
}

void BattleView::endBattle()
{
	Battle::finishBattle(*state);
	fw().stageQueueCommand({StageCmd::Command::REPLACEALL, mksp<BattleDebriefing>(state)});
}

void BattleView::updateLayerButtons()
{
	switch (getZLevel())
	{
		case 1:
			mainTab->findControlTyped<RadioButton>("BUTTON_LAYER_1")->setChecked(true);
			break;
		case 2:
			mainTab->findControlTyped<RadioButton>("BUTTON_LAYER_2")->setChecked(true);
			break;
		case 3:
			mainTab->findControlTyped<RadioButton>("BUTTON_LAYER_3")->setChecked(true);
			break;
		case 4:
			mainTab->findControlTyped<RadioButton>("BUTTON_LAYER_4")->setChecked(true);
			break;
		case 5:
			mainTab->findControlTyped<RadioButton>("BUTTON_LAYER_5")->setChecked(true);
			break;
		case 6:
			mainTab->findControlTyped<RadioButton>("BUTTON_LAYER_6")->setChecked(true);
			break;
		case 7:
			mainTab->findControlTyped<RadioButton>("BUTTON_LAYER_7")->setChecked(true);
			break;
		case 8:
			mainTab->findControlTyped<RadioButton>("BUTTON_LAYER_8")->setChecked(true);
			break;
		case 9:
			mainTab->findControlTyped<RadioButton>("BUTTON_LAYER_9")->setChecked(true);
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
		activeTab->findControlTyped<Graphic>("IMAGE_" + name + "_HAND")
		    ->setImage(info.itemType->equipscreen_sprite);
		activeTab->findControlTyped<Graphic>("IMAGE_" + name + "_HAND")->ToolTipText =
		    tr(info.itemType->name);
		if (info.damageType)
		{
			activeTab->findControlTyped<Graphic>("IMAGE_" + name + "_DAMAGETYPE")
			    ->setImage(info.damageType->icon_sprite);
			activeTab->findControlTyped<Graphic>("IMAGE_" + name + "_DAMAGETYPE")->ToolTipText =
			    tr(info.damageType->name);
		}
		else
		{
			activeTab->findControlTyped<Graphic>("IMAGE_" + name + "_DAMAGETYPE")
			    ->setImage(nullptr);
			activeTab->findControlTyped<Graphic>("IMAGE_" + name + "_DAMAGETYPE")->ToolTipText =
			    tr("None");
		}
	}
	else
	{
		activeTab->findControlTyped<Graphic>("IMAGE_" + name + "_HAND")->setImage(nullptr);
		activeTab->findControlTyped<Graphic>("IMAGE_" + name + "_HAND")->ToolTipText = tr("Empty");
		activeTab->findControlTyped<Graphic>("IMAGE_" + name + "_DAMAGETYPE")->setImage(nullptr);
		activeTab->findControlTyped<Graphic>("IMAGE_" + name + "_DAMAGETYPE")->ToolTipText =
		    tr("None");
	}

	// Selection bracket
	if (info.selected)
	{
		activeTab->findControlTyped<Graphic>("IMAGE_" + name + "_HAND_SELECTED")
		    ->setImage(selectedItemOverlay);
	}
	else
	{
		activeTab->findControlTyped<Graphic>("IMAGE_" + name + "_HAND_SELECTED")->setImage(nullptr);
	}

	constexpr int maxAccuracy = 50;

	auto overlay = mksp<RGBImage>(Vec2<int>{maxAccuracy, 95});
	{
		RGBImageLock l(overlay);

		// Draw accuracy
		if (info.accuracy / 2 > 0)
		{
			int accuracy = std::min(info.accuracy, maxAccuracy);
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
					l.set({maxAccuracy - 1 - x, y}, accuracyColors[x * colorsCount / accuracy]);
					l.set({maxAccuracy - 1 - x, y + 1}, accuracyColors[x * colorsCount / accuracy]);
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
	activeTab->findControlTyped<Graphic>("OVERLAY_" + name + "_HAND")->setImage(overlay);
}

void BattleView::updatePsiInfo()
{
	activeTab->findControlTyped<Graphic>("OVERLAY_CONTROL")
	    ->setImage(psiInfo.status == PsiStatus::Control ? selectedPsiOverlay : nullptr);
	activeTab->findControlTyped<Graphic>("OVERLAY_PANIC")
	    ->setImage(psiInfo.status == PsiStatus::Panic ? selectedPsiOverlay : nullptr);
	activeTab->findControlTyped<Graphic>("OVERLAY_STUN")
	    ->setImage(psiInfo.status == PsiStatus::Stun ? selectedPsiOverlay : nullptr);
	activeTab->findControlTyped<Graphic>("OVERLAY_PROBE")
	    ->setImage(psiInfo.status == PsiStatus::Probe ? selectedPsiOverlay : nullptr);

	activeTab->findControlTyped<Label>("PSI_ENERGY_LABEL")
	    ->setText(format("%d", psiInfo.curEnergy));
	activeTab->findControlTyped<Label>("PSI_ATTACK_LABEL")
	    ->setText(format("%d", psiInfo.curAttack));
	activeTab->findControlTyped<Label>("PSI_DEFENSE_LABEL")
	    ->setText(format("%d", psiInfo.curDefense));

	// FIXME: Maybe pre-draw all 100 of them?

	activeTab->findControlTyped<Graphic>("PSI_ENERGY_BAR")
	    ->setImage(drawPsiBar(psiInfo.curEnergy, psiInfo.maxEnergy));
	activeTab->findControlTyped<Graphic>("PSI_ATTACK_BAR")
	    ->setImage(drawPsiBar(psiInfo.curAttack, psiInfo.maxAttack));
	activeTab->findControlTyped<Graphic>("PSI_DEFENSE_BAR")
	    ->setImage(drawPsiBar(psiInfo.curDefense, psiInfo.maxDefense));
}

void BattleView::updateMotionInfo(bool right, BattleScanner &scanner)
{
	motionScannerUnit[right]->setImage(motionScannerDirectionIcons[motionInfo.direction]);
	motionScannerData[right]->setImage(drawMotionScanner(scanner));
}

void BattleView::updateMotionInfo(bool right, Vec2<int> position)
{
	motionScannerUnit[right]->setImage(motionScannerDirectionIcons[motionInfo.direction]);
	motionScannerData[right]->setImage(drawMotionScanner(position));
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

sp<RGBImage> BattleView::drawMotionScanner(BattleScanner &scanner)
{
	static const std::vector<Colour> colors = {{
	                                               0,
	                                               0,
	                                               0,
	                                           },
	                                           {16, 16, 16, 255},
	                                           {32, 32, 32, 255},
	                                           {48, 48, 48, 255},
	                                           {64, 64, 64, 255},
	                                           {80, 80, 80, 255},
	                                           {96, 96, 96, 255},
	                                           {112, 112, 112, 255},
	                                           {128, 128, 128, 255},
	                                           {144, 144, 144, 255},
	                                           {160, 160, 160, 255},
	                                           {176, 176, 176, 255},
	                                           {192, 192, 192, 255},
	                                           {208, 208, 208, 255},
	                                           {224, 224, 224, 255},
	                                           {240, 240, 240, 255}};

	auto scannerDisplay = mksp<RGBImage>(Vec2<int>{MOTION_SCANNER_X * 2, MOTION_SCANNER_Y * 2});
	{
		RGBImageLock l(scannerDisplay);

		for (int x = 0; x < MOTION_SCANNER_X; x++)
		{
			for (int y = 0; y < MOTION_SCANNER_Y; y++)
			{
				auto &color =
				    colors.at(std::min(15, scanner.movementTicks[y * MOTION_SCANNER_X + x] * 16 /
				                               (int)TICKS_SCANNER_REMAIN_LIT));
				for (int i = 0; i <= 1; i++)
				{
					for (int j = 0; j <= 1; j++)
					{
						l.set({2 * x + i, 2 * y + j}, color);
					}
				}
			}
		}
	}
	return scannerDisplay;
}

sp<RGBImage> BattleView::drawMotionScanner(Vec2<int> position)
{
	static const std::vector<Colour> colors = {{
	                                               0,
	                                               0,
	                                               0,
	                                           },
	                                           {16, 16, 16, 255},
	                                           {32, 32, 32, 255},
	                                           {48, 48, 48, 255},
	                                           {64, 64, 64, 255},
	                                           {80, 80, 80, 255},
	                                           {96, 96, 96, 255},
	                                           {112, 112, 112, 255},
	                                           {128, 128, 128, 255},
	                                           {144, 144, 144, 255},
	                                           {160, 160, 160, 255},
	                                           {176, 176, 176, 255},
	                                           {192, 192, 192, 255},
	                                           {208, 208, 208, 255},
	                                           {224, 224, 224, 255},
	                                           {240, 240, 240, 255}};
	static const Vec2<int> midPos = {MOTION_SCANNER_X / 2, MOTION_SCANNER_Y / 2};
	auto scannerDisplay = mksp<RGBImage>(Vec2<int>{MOTION_SCANNER_X * 2, MOTION_SCANNER_Y * 2});
	{
		RGBImageLock l(scannerDisplay);

		auto movementTicks = std::vector<int>(MOTION_SCANNER_X * MOTION_SCANNER_Y);

		for (auto &u : battle.units)
		{
			if (!u.second->isConscious() || u.second->tilesMoved == 0)
			{
				continue;
			}
			auto pos = Vec2<int>(u.second->position.x, u.second->position.y) - position + midPos;
			if (pos.x < 0 || pos.y < 0 || pos.x >= MOTION_SCANNER_X || pos.y >= MOTION_SCANNER_Y)
			{
				continue;
			}
			if (movementTicks[pos.y * MOTION_SCANNER_X + pos.x] < u.second->tilesMoved)
			{
				movementTicks[pos.y * MOTION_SCANNER_X + pos.x] = u.second->tilesMoved;
			}
		}

		for (int x = 0; x < MOTION_SCANNER_X; x++)
		{
			for (int y = 0; y < MOTION_SCANNER_Y; y++)
			{
				if (movementTicks[y * MOTION_SCANNER_X + x] > 0)
				{
					movementTicks[y * MOTION_SCANNER_X + x] =
					    5 + movementTicks[y * MOTION_SCANNER_X + x] * 2 / 3;
				}
				auto &color = colors.at(std::min(15, movementTicks[y * MOTION_SCANNER_X + x]));
				for (int i = 0; i <= 1; i++)
				{
					for (int j = 0; j <= 1; j++)
					{
						l.set({2 * x + i, 2 * y + j}, color);
					}
				}
			}
		}
	}
	return scannerDisplay;
}

AgentInfo BattleView::createUnitInfo(int index)
{
	sp<Agent> a;
	if (battle.battleViewSquadIndex != -1 &&
	    battle.forces[battle.currentPlayer].squads[battle.battleViewSquadIndex].getNumUnits() >
	        index)
	{
		a = battle.forces[battle.currentPlayer]
		        .squads[battle.battleViewSquadIndex]
		        .units[index]
		        ->agent;
	}
	return ControlGenerator::createAgentInfo(*state, a);
}

void BattleView::updateUnitInfo(int index)
{
	AgentInfo info = unitInfo[index];
	auto baseControl = baseForm->findControlTyped<Graphic>(format("UNIT_%d", index + 1));
	baseControl->Controls.clear();
	if (!info.agent)
	{
		baseControl->setImage(nullptr);
		return;
	}
	ControlGenerator::fillAgentControl(*state, baseControl, info);
}

void BattleView::updateSpottedInfo(int index)
{
	if (spottedInfo[index] == 0)
	{
		baseForm->findControlTyped<Graphic>(format("UNIT_%d_HOSTILES", index + 1))
		    ->setImage(nullptr);
	}
	else
	{
		baseForm->findControlTyped<Graphic>(format("UNIT_%d_HOSTILES", index + 1))
		    ->setImage(unitHostiles[spottedInfo[index]]);
	}
}

SquadInfo BattleView::createSquadInfo(int index)
{
	SquadInfo s;
	s.selectedMode = 0;
	s.units = battle.forces[battle.currentPlayer].squads[index].getNumUnits();
	if (s.units == 0)
	{
		return s;
	}
	if (battle.battleViewSquadIndex == index)
	{
		s.selectedMode = 2;
	}
	else
	{
		for (auto &u : battle.battleViewSelectedUnits)
		{
			if (u->squadNumber == index)
			{
				s.selectedMode = 1;
			}
		}
	}
	return s;
}

void BattleView::updateSquadInfo(int index)
{
	SquadInfo info = squadInfo[index];

	baseForm->findControlTyped<Graphic>(format("SQUAD_%d", index + 1))
	    ->setImage(squadNumber[info.units]);
	baseForm->findControlTyped<Graphic>(format("SQUAD_%d_OVERLAY", index + 1))
	    ->setImage(squadOverlay[info.selectedMode]);
}

void BattleView::finish()
{
	BattleTileView::finish();
	fw().getCursor().CurrentType = ApocCursor::CursorType::Normal;
}

AgentEquipmentInfo BattleView::createItemOverlayInfo(bool rightHand)
{
	AgentEquipmentInfo a;
	if (battle.battleViewSelectedUnits.empty())
	{
		return a;
	}
	auto u = battle.battleViewSelectedUnits.front();
	sp<AEquipment> e = nullptr;
	if (rightHand)
	{
		e = u->agent->getFirstItemInSlot(EquipmentSlotType::RightHand);
	}
	else
	{
		e = u->agent->getFirstItemInSlot(EquipmentSlotType::LeftHand);
	}
	if (e)
	{
		a.itemType = e->type;
		if (a.itemType && a.itemType->research_dependency.satisfied())
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
			a.accuracy =
			    std::max(0, e->getAccuracy(u->current_body_state, u->current_movement_state,
			                               u->fire_aiming_mode,
			                               a.itemType->type != AEquipmentType::Type::Weapon) /
			                    2);
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

MotionScannerInfo BattleView::createMotionInfo(BattleScanner &scanner)
{
	static const std::map<Vec2<int>, int> offset_dir_map = {
	    {{0, -1}, 0}, {{1, -1}, 1}, {{1, 0}, 2},  {{1, 1}, 3},
	    {{0, 1}, 4},  {{-1, 1}, 5}, {{-1, 0}, 6}, {{-1, -1}, 7},
	};

	MotionScannerInfo a;

	a.direction = offset_dir_map.at(battle.battleViewSelectedUnits.front()->facing);
	a.id = scanner.holder.id;
	a.version = scanner.version;

	return a;
}

MotionScannerInfo BattleView::createMotionInfo()
{
	static const std::map<Vec2<int>, int> offset_dir_map = {
	    {{0, -1}, 0}, {{1, -1}, 1}, {{1, 0}, 2},  {{1, 1}, 3},
	    {{0, 1}, 4},  {{-1, 1}, 5}, {{-1, 0}, 6}, {{-1, -1}, 7},
	};

	MotionScannerInfo a;

	auto u = battle.battleViewSelectedUnits.front();
	a.direction = offset_dir_map.at(u->facing);
	a.id = u.id;
	a.position = {u->position.x, u->position.y};
	// Update every other second
	a.version = state->gameTime.getTicks() / TICKS_PER_SECOND / 2;

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

bool MotionScannerInfo::operator==(const MotionScannerInfo &other) const
{
	return (this->direction == other.direction && this->id == other.id &&
	        this->version == other.version && this->position == other.position);
}

bool MotionScannerInfo::operator!=(const MotionScannerInfo &other) const
{
	return !(*this == other);
}

void BattleView::zoomAt(Vec3<int> location)
{
	setScreenCenterTile(location);
	setZLevel(location.z + 1);
	updateLayerButtons();
}

void BattleView::zoomLastEvent()
{
	if (!state->messages.empty())
	{
		auto message = state->messages.back();
		if (message.location != EventMessage::NO_LOCATION)
		{
			zoomAt(message.location);
		}
	}
}
bool SquadInfo::operator==(const SquadInfo &other) const
{
	return this->selectedMode == other.selectedMode && this->units == other.units;
}
bool SquadInfo::operator!=(const SquadInfo &other) const { return !(*this == other); }
}; // namespace OpenApoc
