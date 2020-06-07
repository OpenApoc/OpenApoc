#include "game/ui/general/ingameoptions.h"
#include "forms/checkbox.h"
#include "forms/form.h"
#include "forms/label.h"
#include "forms/listbox.h"
#include "forms/scrollbar.h"
#include "forms/textbutton.h"
#include "forms/ui.h"
#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "framework/sound.h"
#include "game/state/battle/battle.h"
#include "game/state/gamestate.h"
#include "game/ui/battle/battledebriefing.h"
#include "game/ui/general/cheatoptions.h"
#include "game/ui/general/mainmenu.h"
#include "game/ui/general/messagebox.h"
#include "game/ui/general/savemenu.h"
#include "game/ui/skirmish/skirmish.h"
#include "game/ui/tileview/cityview.h"
#include <list>

namespace OpenApoc
{
namespace
{
std::list<std::pair<UString, UString>> battleNotificationList = {
    {"Notifications.Battle", "HostileSpotted"},
    {"Notifications.Battle", "HostileDied"},
    {"Notifications.Battle", "UnknownDied"},
    {"Notifications.Battle", "AgentDiedBattle"},
    {"Notifications.Battle", "AgentBrainsucked"},
    {"Notifications.Battle", "AgentCriticallyWounded"},
    {"Notifications.Battle", "AgentBadlyInjured"},
    {"Notifications.Battle", "AgentInjured"},
    {"Notifications.Battle", "AgentUnderFire"},
    {"Notifications.Battle", "AgentUnconscious"},
    {"Notifications.Battle", "AgentLeftCombat"},
    {"Notifications.Battle", "AgentFrozen"},
    {"Notifications.Battle", "AgentBerserk"},
    {"Notifications.Battle", "AgentPanicked"},
    {"Notifications.Battle", "AgentPanicOver"},
    {"Notifications.Battle", "AgentPsiAttacked"},
    {"Notifications.Battle", "AgentPsiControlled"},
    {"Notifications.Battle", "AgentPsiOver"},
};

std::list<std::pair<UString, UString>> cityNotificationList = {
    {"Notifications.City", "UfoSpotted"},
    {"Notifications.City", "VehicleLightDamage"},
    {"Notifications.City", "VehicleModerateDamage"},
    {"Notifications.City", "VehicleHeavyDamage"},
    {"Notifications.City", "VehicleDestroyed"},
    {"Notifications.City", "VehicleEscaping"},
    {"Notifications.City", "VehicleNoAmmo"},
    {"Notifications.City", "VehicleLowFuel"},
    {"Notifications.City", "AgentDiedCity"},
    {"Notifications.City", "AgentArrived"},
    {"Notifications.City", "CargoArrived"},
    {"Notifications.City", "TransferArrived"},
    {"Notifications.City", "RecoveryArrived"},
    {"Notifications.City", "VehicleRepaired"},
    {"Notifications.City", "VehicleRearmed"},
    {"Notifications.City", "NotEnoughAmmo"},
    {"Notifications.City", "VehicleRefuelled"},
    {"Notifications.City", "NotEnoughFuel"},
    {"Notifications.City", "UnauthorizedVehicle"},
};

std::list<std::pair<UString, UString>> openApocList = {
    {"OpenApoc.NewFeature", "UFODamageModel"},
    {"OpenApoc.NewFeature", "InstantExplosionDamage"},
    {"OpenApoc.NewFeature", "GravliftSounds"},
    {"OpenApoc.NewFeature", "NoInstantThrows"},
    {"OpenApoc.NewFeature", "PayloadExplosion"},
    {"OpenApoc.NewFeature", "DisplayUnitPaths"},
    {"OpenApoc.NewFeature", "AdditionalUnitIcons"},
    {"OpenApoc.NewFeature", "AllowForceFiringParallel"},
    {"OpenApoc.NewFeature", "RequireLOSToMaintainPsi"},
    {"OpenApoc.NewFeature", "AdvancedInventoryControls"},
    {"OpenApoc.NewFeature", "EnableAgentTemplates"},
    {"OpenApoc.NewFeature", "FerryChecksRelationshipWhenBuying"},
    {"OpenApoc.NewFeature", "AllowManualCityTeleporters"},
    {"OpenApoc.NewFeature", "AllowManualCargoFerry"},
    {"OpenApoc.NewFeature", "AllowSoldierTaxiUse"},
    {"OpenApoc.NewFeature", "AllowAttackingOwnedVehicles"},
    {"OpenApoc.NewFeature", "CallExistingFerry"},
    {"OpenApoc.NewFeature", "AlternateVehicleShieldSound"},
    {"OpenApoc.NewFeature", "StoreDroppedEquipment"},
    {"OpenApoc.NewFeature", "EnforceCargoLimits"},
    {"OpenApoc.NewFeature", "AllowNearbyVehicleLootPickup"},
    {"OpenApoc.NewFeature", "AllowBuildingLootDeposit"},
    {"OpenApoc.NewFeature", "ArmoredRoads"},
    {"OpenApoc.NewFeature", "CrashingGroundVehicles"},
    {"OpenApoc.NewFeature", "OpenApocCityControls"},
    {"OpenApoc.NewFeature", "CollapseRaidedBuilding"},
    {"OpenApoc.NewFeature", "ScrambleOnUnintentionalHit"},
    {"OpenApoc.NewFeature", "MarketOnRight"},
    {"OpenApoc.NewFeature", "CrashingDimensionGate"},
    {"OpenApoc.NewFeature", "SkipTurboMovement"},
    {"OpenApoc.NewFeature", "CrashingOutOfFuel"},
    {"OpenApoc.NewFeature", "RunAndKneel"},
    {"OpenApoc.NewFeature", "SeedRng"},
    {"OpenApoc.NewFeature", "AutoReload"},

    {"OpenApoc.Mod", "StunHostileAction"},
    {"OpenApoc.Mod", "RaidHostileAction"},
    {"OpenApoc.Mod", "CrashingVehicles"},
    {"OpenApoc.Mod", "InvulnerableRoads"},
    {"OpenApoc.Mod", "ATVTank"},
    {"OpenApoc.Mod", "ATVAPC"},
    {"OpenApoc.Mod", "BSKLauncherSound"},
};

std::vector<UString> listNames = {tr("Message Toggles"), tr("OpenApoc Features")};
} // namespace

InGameOptions::InGameOptions(sp<GameState> state)
    : Stage(), menuform(ui().getForm("ingameoptions")), state(state)
{
}

InGameOptions::~InGameOptions() {}

void InGameOptions::saveList()
{
	auto listControl = menuform->findControlTyped<ListBox>("NOTIFICATIONS_LIST");
	for (auto &c : listControl->Controls)
	{
		auto name = c->getData<UString>();
		config().set(*name, std::dynamic_pointer_cast<CheckBox>(c)->isChecked());
	}
}

void InGameOptions::loadList(int id)
{
	saveList();
	curId = id;
	menuform->findControlTyped<Label>("LIST_NAME")->setText(listNames[curId]);
	std::list<std::pair<UString, UString>> *notificationList = nullptr;
	switch (curId)
	{
		case 0:
			notificationList =
			    state->current_battle ? &battleNotificationList : &cityNotificationList;
			break;
		case 1:
			notificationList = &openApocList;
			break;
	}
	auto listControl = menuform->findControlTyped<ListBox>("NOTIFICATIONS_LIST");
	listControl->clear();
	auto font = ui().getFont("smalfont");
	for (auto &p : *notificationList)
	{
		auto checkBox = mksp<CheckBox>(fw().data->loadImage("BUTTON_CHECKBOX_TRUE"),
		                               fw().data->loadImage("BUTTON_CHECKBOX_FALSE"));
		checkBox->Size = {240, listControl->ItemSize};
		UString full_name = p.first + "." + p.second;
		checkBox->setData(mksp<UString>(full_name));
		checkBox->setChecked(config().getBool(full_name));
		auto label = checkBox->createChild<Label>(tr(config().describe(p.first, p.second)), font);
		label->Size = {216, listControl->ItemSize};
		label->Location = {24, 0};
		listControl->addItem(checkBox);
	}
}

void InGameOptions::loadNextList()
{
	curId++;
	if (curId > 1)
	{
		curId = 0;
	}
	loadList(curId);
}

void InGameOptions::begin()
{
	/* Initialise all initial values */

	menuform->findControlTyped<ScrollBar>("GLOBAL_GAIN_SLIDER")
	    ->setValue(config().getInt("Framework.Audio.GlobalGain"));

	menuform->findControlTyped<ScrollBar>("MUSIC_GAIN_SLIDER")
	    ->setValue(config().getInt("Framework.Audio.MusicGain"));

	menuform->findControlTyped<ScrollBar>("SAMPLE_GAIN_SLIDER")
	    ->setValue(config().getInt("Framework.Audio.SampleGain"));

	menuform->findControlTyped<CheckBox>("AUTO_SCROLL")
	    ->setChecked(config().getBool("Options.Misc.AutoScroll"));
	menuform->findControlTyped<CheckBox>("TOOL_TIPS")
	    ->setChecked(config().getInt("Options.Misc.ToolTipDelay") > 0);
	menuform->findControlTyped<CheckBox>("ACTION_MUSIC")
	    ->setChecked(config().getBool("Options.Misc.ActionMusic"));
	menuform->findControlTyped<CheckBox>("AUTO_EXECUTE_ORDERS")
	    ->setChecked(config().getBool("Options.Misc.AutoExecute"));

	if (state->current_battle)
	{
		menuform->findControlTyped<TextButton>("BUTTON_EXIT_BATTLE")->setVisible(true);
		menuform->findControlTyped<TextButton>("BUTTON_SKIRMISH")->setVisible(false);
	}
	else
	{
		menuform->findControlTyped<TextButton>("BUTTON_EXIT_BATTLE")->setVisible(false);
		menuform->findControlTyped<TextButton>("BUTTON_SKIRMISH")->setVisible(true);
	}

	menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());

	loadList(0);
}

void InGameOptions::pause() {}

void InGameOptions::resume() {}

void InGameOptions::finish()
{
	/* Store persistent options */

	config().set("Framework.Audio.GlobalGain",
	             menuform->findControlTyped<ScrollBar>("GLOBAL_GAIN_SLIDER")->getValue());
	config().set("Framework.Audio.MusicGain",
	             menuform->findControlTyped<ScrollBar>("MUSIC_GAIN_SLIDER")->getValue());
	config().set("Framework.Audio.SampleGain",
	             menuform->findControlTyped<ScrollBar>("SAMPLE_GAIN_SLIDER")->getValue());

	config().set("Options.Misc.AutoScroll",
	             menuform->findControlTyped<CheckBox>("AUTO_SCROLL")->isChecked());
	config().set("Options.Misc.ToolTipDelay",
	             menuform->findControlTyped<CheckBox>("TOOL_TIPS")->isChecked() ? 500 : 0);
	config().set("Options.Misc.ActionMusic",
	             menuform->findControlTyped<CheckBox>("ACTION_MUSIC")->isChecked());
	config().set("Options.Misc.AutoExecute",
	             menuform->findControlTyped<CheckBox>("AUTO_EXECUTE_ORDERS")->isChecked());

	saveList();
}

void InGameOptions::eventOccurred(Event *e)
{
	menuform->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE || e->keyboard().KeyCode == SDLK_RETURN ||
		    e->keyboard().KeyCode == SDLK_KP_ENTER)
		{
			menuform->findControl("BUTTON_OK")->click();
			return;
		}
	}

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().EventFlag == FormEventType::ButtonClick)
	{
		if (e->forms().RaisedBy->Name == "BUTTON_OK")
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
			return;
		}
		if (e->forms().RaisedBy->Name == "BUTTON_ABANDONGAME")
		{
			fw().stageQueueCommand({StageCmd::Command::REPLACEALL, mksp<MainMenu>()});
			return;
		}
		if (e->forms().RaisedBy->Name == "BUTTON_QUIT")
		{
			fw().stageQueueCommand({StageCmd::Command::QUIT});
			return;
		}
		if (e->forms().RaisedBy->Name == "BUTTON_SAVEGAME")
		{
			fw().stageQueueCommand(
			    {StageCmd::Command::PUSH, mksp<SaveMenu>(SaveMenuAction::Save, state)});
			return;
		}
		if (e->forms().RaisedBy->Name == "BUTTON_DELETESAVEDGAME")
		{
			fw().stageQueueCommand(
			    {StageCmd::Command::PUSH, mksp<SaveMenu>(SaveMenuAction::Delete, state)});
			return;
		}
		if (e->forms().RaisedBy->Name == "BUTTON_LOADGAME")
		{
			fw().stageQueueCommand(
			    {StageCmd::Command::PUSH, mksp<SaveMenu>(SaveMenuAction::Load, state)});
			return;
		}
		if (e->forms().RaisedBy->Name == "BUTTON_NEXT_LIST")
		{
			loadNextList();
			return;
		}
		if (e->forms().RaisedBy->Name == "BUTTON_CHEATS")
		{
			fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<CheatOptions>(state)});
			return;
		}
		if (e->forms().RaisedBy->Name == "BUTTON_EXIT_BATTLE")
		{
			int unitsLost = state->current_battle->killStrandedUnits(
			    *state, state->current_battle->currentPlayer, true);
			fw().stageQueueCommand(
			    {StageCmd::Command::PUSH,
			     mksp<MessageBox>(tr("Abort Mission"),
			                      format("%s %d", tr("Units Lost :"), unitsLost),
			                      MessageBox::ButtonOptions::YesNo, [this] {
				                      state->current_battle->abortMission(*state);
				                      Battle::finishBattle(*state);
				                      fw().stageQueueCommand({StageCmd::Command::REPLACEALL,
				                                              mksp<BattleDebriefing>(state)});
			                      })});
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_SKIRMISH")
		{
			fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<Skirmish>(state)});
		}
	}
	if (e->type() == EVENT_FORM_INTERACTION &&
	    e->forms().EventFlag == FormEventType::ScrollBarChange)
	{
		if (e->forms().RaisedBy->Name == "GLOBAL_GAIN_SLIDER")
		{
			auto slider = std::dynamic_pointer_cast<ScrollBar>(e->forms().RaisedBy);
			if (!slider)
			{
				LogError("Failed to cast \"GLOBAL_GAIN_SLIDER\" control to ScrollBar");
				return;
			}
			float gain =
			    static_cast<float>(slider->getValue()) / static_cast<float>(slider->getMaximum());
			fw().soundBackend->setGain(SoundBackend::Gain::Global, gain);
		}
		else if (e->forms().RaisedBy->Name == "MUSIC_GAIN_SLIDER")
		{
			auto slider = std::dynamic_pointer_cast<ScrollBar>(e->forms().RaisedBy);
			if (!slider)
			{
				LogError("Failed to cast \"MUSIC_GAIN_SLIDER\" control to ScrollBar");
				return;
			}
			float gain =
			    static_cast<float>(slider->getValue()) / static_cast<float>(slider->getMaximum());
			fw().soundBackend->setGain(SoundBackend::Gain::Music, gain);
		}
		else if (e->forms().RaisedBy->Name == "SAMPLE_GAIN_SLIDER")
		{
			auto slider = std::dynamic_pointer_cast<ScrollBar>(e->forms().RaisedBy);
			if (!slider)
			{
				LogError("Failed to cast \"SAMPLE_GAIN_SLIDER\" control to ScrollBar");
				return;
			}
			float gain =
			    static_cast<float>(slider->getValue()) / static_cast<float>(slider->getMaximum());
			fw().soundBackend->setGain(SoundBackend::Gain::Sample, gain);
		}
	}
	if (e->type() == EVENT_FORM_INTERACTION &&
	    e->forms().EventFlag == FormEventType::CheckBoxChange)
	{
		if (e->forms().RaisedBy->Name == "SOME_NON_CONFIG_FILE_OPTION_OR_HAS_TO_EFFECT_IMMEDIATELY")
		{
			auto box = std::dynamic_pointer_cast<CheckBox>(e->forms().RaisedBy);
			// Set non-config option here or do the effect
		}
	}
}

void InGameOptions::update() { menuform->update(); }

void InGameOptions::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->render();
}

bool InGameOptions::isTransition() { return false; }

}; // namespace OpenApoc
