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
    {"Notifications.Battle.HostileSpotted", "Hostile unit spotted"},
    {"Notifications.Battle.HostileDied", "Hostile unit has died"},
    {"Notifications.Battle.UnknownDied", "Unknown Unit has died"},
    {"Notifications.Battle.AgentDiedBattle", "Unit has died"},
    {"Notifications.Battle.AgentBrainsucked", "Unit Brainsucked"},
    {"Notifications.Battle.AgentCriticallyWounded", "Unit critically wounded"},
    {"Notifications.Battle.AgentBadlyInjured", "Unit badly injured"},
    {"Notifications.Battle.AgentInjured", "Unit injured"},
    {"Notifications.Battle.AgentUnderFire", "Unit under fire"},
    {"Notifications.Battle.AgentUnconscious", "Unit has lost consciousness"},
    {"Notifications.Battle.AgentLeftCombat", "Unit has left combat zone"},
    {"Notifications.Battle.AgentFrozen", "Unit has frozen"},
    {"Notifications.Battle.AgentBerserk", "Unit has gone beserk"},
    {"Notifications.Battle.AgentPanicked", "Unit has panicked"},
    {"Notifications.Battle.AgentPanicOver", "Unit has stopped panicking"},
    {"Notifications.Battle.AgentPsiAttacked", "Psionic attack on unit"},
    {"Notifications.Battle.AgentPsiControlled", "Unit under Psionic control"},
    {"Notifications.Battle.AgentPsiOver", "Unit freed from Psionic control"},
};

std::list<std::pair<UString, UString>> cityNotificationList = {
    {"Notifications.City.UfoSpotted", "UFO spotted"},
    {"Notifications.City.VehicleLightDamage", "Vehicle lightly damaged"},
    {"Notifications.City.VehicleModerateDamage", "Vehicle moderately damaged"},
    {"Notifications.City.VehicleHeavyDamage", "Vehicle heavily damaged"},
    {"Notifications.City.VehicleDestroyed", "Vehicle destroyed"},
    {"Notifications.City.VehicleEscaping", "Vehicle damaged and returning to base"},
    {"Notifications.City.VehicleNoAmmo", "Weapon out of ammo"},
    {"Notifications.City.VehicleLowFuel", "Vehicle low on fuel"},
    {"Notifications.City.AgentDiedCity", "Agent has died"},
    {"Notifications.City.AgentArrived", "Agent arrived at base"},
    {"Notifications.City.CargoArrived", "Cargo has arrived at base"},
    {"Notifications.City.TransferArrived", "Transfer arrived at base"},
    {"Notifications.City.RecoveryArrived", "Crash recovery arrived at base"},
    {"Notifications.City.VehicleRepaired", "Vehicle repaired"},
    {"Notifications.City.VehicleRearmed", "Vehicle rearmed"},
    {"Notifications.City.NotEnoughAmmo", "Not enough ammo to rearm vehicle"},
    {"Notifications.City.VehicleRefuelled", "Vehicle refuelled"},
    {"Notifications.City.NotEnoughFuel", "Not enough fuel to refuel vehicle"},
    {"Notifications.City.UnauthorizedVehicle", "Unauthorized vehicle detected"},
};

std::list<std::pair<UString, UString>> openApocList = {
    {"OpenApoc.NewFeature.UFODamageModel", "X-Com: EU Damage Model (0-200%)"},
    {"OpenApoc.NewFeature.InstantExplosionDamage", "Explosions deal damage instantly"},
    {"OpenApoc.NewFeature.GravliftSounds", "Gravlift sounds"},
    {"OpenApoc.NewFeature.NoInstantThrows", "Throwing requires proper facing and pose"},
    {"OpenApoc.NewFeature.PayloadExplosion", "Ammunition explodes when blown up"},
    {"OpenApoc.NewFeature.DisplayUnitPaths", "Display unit paths in battle"},
    {"OpenApoc.NewFeature.AdditionalUnitIcons", "Display additional unit icons (fatal, psi)"},
    {"OpenApoc.NewFeature.AllowForceFiringParallel", "Allow force-firing parallel to the ground"},
    {"OpenApoc.NewFeature.RequireLOSToMaintainPsi", "(N) Require LOS to maintain psi attack"},
    {"OpenApoc.NewFeature.AdvancedInventoryControls", "Allow unloading clips and quick equip"},
    {"OpenApoc.NewFeature.EnableAgentTemplates", "Enable agent equipment templates"},
    {"OpenApoc.NewFeature.FerryChecksRelationshipWhenBuying",
     "Relationship check for purchase delivery"},
    {"OpenApoc.NewFeature.AllowManualCityTeleporters", "Allow manual use of teleporters in city"},
    {"OpenApoc.NewFeature.AllowManualCargoFerry", "Allow manual ferrying using owned vehicles"},
    {"OpenApoc.NewFeature.AllowSoldierTaxiUse", "Allow soldiers to call taxi"},
    {"OpenApoc.NewFeature.AllowAttackingOwnedVehicles", "Allow attacking owned vehicles"},
    {"OpenApoc.NewFeature.CallExistingFerry", "Reallistic transportation system"},
    {"OpenApoc.NewFeature.AlternateVehicleShieldSound", "Alternate vehicle shield hit SFX"},
    {"OpenApoc.NewFeature.StoreDroppedEquipment",
     "Attempt to recover agent equipment dropped in city"},
    {"OpenApoc.NewFeature.EnforceCargoLimits", "(N) Enforce vehicle cargo limits"},
    {"OpenApoc.NewFeature.AllowNearbyVehicleLootPickup", "Allow nearby vehicles to pick up loot"},
    {"OpenApoc.NewFeature.AllowBuildingLootDeposit", "Allow loot to be stashed in the building"},
    {"OpenApoc.NewFeature.ArmoredRoads", "Armored roads (20 armor value)"},
    {"OpenApoc.NewFeature.CrashingGroundVehicles", "Unsupported ground vehicles crash"},
    {"OpenApoc.NewFeature.OpenApocCityControls", "Improved city control scheme"},
    {"OpenApoc.NewFeature.CollapseRaidedBuilding", "Successful raid collapses building"},
    {"OpenApoc.NewFeature.ScrambleOnUnintentionalHit",
     "Any hit on hostile building provokes retaliation"},
    {"OpenApoc.NewFeature.MarketOnRight", "Put market stock on the right side"},
    {"OpenApoc.NewFeature.CrashingDimensionGate", "Uncapable vehicles crash when entering gates"},
    {"OpenApoc.NewFeature.SkipTurboMovement", "Skip turbo movement calculations"},
    {"OpenApoc.NewFeature.CrashingOutOfFuel", "Vehicles crash when out of fuel"},

    {"OpenApoc.Mod.StunHostileAction", "(M) Stunning hurts relationships"},
    {"OpenApoc.Mod.RaidHostileAction", "(M) Initiating raid hurts relationships"},
    {"OpenApoc.Mod.CrashingVehicles", "(M) Vehicles crash on low HP"},
    {"OpenApoc.Mod.InvulnerableRoads", "(M) Invulnerable roads"},
    {"OpenApoc.Mod.ATVTank", "(M) Griffon becomes All-Terrain"},
    {"OpenApoc.Mod.BSKLauncherSound", "(M) Original Brainsucker Launcher SFX"},

};

std::vector<UString> listNames = {"Message Toggles", "OpenApoc Features"};
}

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
		checkBox->Size = {240, 16};
		checkBox->setData(mksp<UString>(p.first));
		checkBox->setChecked(config().getBool(p.first));
		auto label = checkBox->createChild<Label>(tr(p.second), font);
		label->Size = {216, 16};
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
	/* Initialse all initial values */

	menuform->findControlTyped<ScrollBar>("GLOBAL_GAIN_SLIDER")
	    ->setValue(config().getInt("Framework.Audio.GlobalGain"));

	menuform->findControlTyped<ScrollBar>("MUSIC_GAIN_SLIDER")
	    ->setValue(config().getInt("Framework.Audio.MusicGain"));

	menuform->findControlTyped<ScrollBar>("SAMPLE_GAIN_SLIDER")
	    ->setValue(config().getInt("Framework.Audio.SampleGain"));

	menuform->findControlTyped<CheckBox>("AUTO_SCROLL")
	    ->setChecked(config().getBool("Options.Misc.AutoScroll"));
	menuform->findControlTyped<CheckBox>("TOOL_TIPS")
	    ->setChecked(config().getBool("Options.Misc.ToolTips"));
	menuform->findControlTyped<CheckBox>("ACTION_MUSIC")
	    ->setChecked(config().getBool("Options.Misc.ActionMusic"));
	menuform->findControlTyped<CheckBox>("AUTO_EXECUTE_ORDERS")
	    ->setChecked(config().getBool("Options.Misc.AutoExecute"));

	menuform->findControlTyped<TextButton>("BUTTON_BATTLE")
	    ->setText(state->current_battle ? "Exit Battle" : "Skirmish Mode");

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
	config().set("Options.Misc.ToolTips",
	             menuform->findControlTyped<CheckBox>("TOOL_TIPS")->isChecked());
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
		if (e->keyboard().KeyCode == SDLK_ESCAPE)
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
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
		if (e->forms().RaisedBy->Name == "BUTTON_GIVE_ALL_RESEARCH")
		{
			for (auto &r : this->state->research.topics)
			{
				LogWarning("Topic \"%s\"", r.first);
				auto &topic = r.second;
				if (topic->isComplete())
				{
					LogWarning("Topic \"%s\" already complete", r.first);
				}

				{
					topic->forceComplete();
					LogWarning("Topic \"%s\" marked as complete", r.first);
				}
			}
			this->state->research.resortTopicList();
			return;
		}
		if (e->forms().RaisedBy->Name == "BUTTON_BATTLE")
		{
			if (state->current_battle)
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
			else
			{
				fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<Skirmish>(state)});
			}
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
