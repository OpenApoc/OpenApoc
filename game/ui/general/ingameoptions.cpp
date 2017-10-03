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
#include "game/ui/city/cityview.h"
#include "game/ui/general/mainmenu.h"
#include "game/ui/general/messagebox.h"
#include "game/ui/general/savemenu.h"
#include "game/ui/general/skirmish.h"
#include <list>

namespace OpenApoc
{
namespace
{
const std::list<std::pair<UString, UString>> battleNotificationList = {
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

const std::list<std::pair<UString, UString>> cityNotificationList = {
    {"Notifications.City.UfoSpotted", "UFO spotted"},
    {"Notifications.City.VehicleLightDamage", "Vehicle lightly damaged"},
    {"Notifications.City.VehicleModerateDamage", "Vehicle moderately damaged"},
    {"Notifications.City.VehicleHeavyDamage", "Vehicle heavily damaged"},
    {"Notifications.City.VehicleDestroyed", "Vehicle destroyed"},
    {"Notifications.City.VehicleEscaping", "Vehicle damaged and returning to base"},
    {"Notifications.City.VehicleNoAmmo", "Weapon out of ammo"},
    {"Notifications.City.VehicleLowFuel", "Vehicle low on fuel"},
    {"Notifications.City.AgentDiedCity", "Agent died"},
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
}

InGameOptions::InGameOptions(sp<GameState> state)
    : Stage(), menuform(ui().getForm("ingameoptions")), state(state)
{
}

InGameOptions::~InGameOptions() {}

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

	auto &notificationList = state->current_battle ? battleNotificationList : cityNotificationList;
	auto listControl = menuform->findControlTyped<ListBox>("NOTIFICATIONS_LIST");
	auto font = ui().getFont("smalfont");
	for (auto &p : notificationList)
	{
		auto checkBox = mksp<CheckBox>(fw().data->loadImage("BUTTON_CHECKBOX_TRUE"),
		                               fw().data->loadImage("BUTTON_CHECKBOX_FALSE"));
		checkBox->Size = {240, 16};
		checkBox->setData(mksp<UString>(p.first));
		checkBox->setChecked(config().getBool(p.first));
		auto label = checkBox->createChild<Label>(p.second, font);
		label->Size = {216, 16};
		label->Location = {24, 0};
		listControl->addItem(checkBox);
	}
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

	auto listControl = menuform->findControlTyped<ListBox>("NOTIFICATIONS_LIST");
	for (auto &c : listControl->Controls)
	{
		auto name = c->getData<UString>();
		config().set(*name, std::dynamic_pointer_cast<CheckBox>(c)->isChecked());
	}
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
		else if (e->forms().RaisedBy->Name == "BUTTON_ABANDONGAME")
		{
			fw().stageQueueCommand({StageCmd::Command::REPLACEALL, mksp<MainMenu>()});
			return;
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_QUIT")
		{
			fw().stageQueueCommand({StageCmd::Command::QUIT});
			return;
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_SAVEGAME")
		{
			fw().stageQueueCommand(
			    {StageCmd::Command::PUSH, mksp<SaveMenu>(SaveMenuAction::Save, state)});
			return;
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_DELETESAVEDGAME")
		{
			fw().stageQueueCommand(
			    {StageCmd::Command::PUSH, mksp<SaveMenu>(SaveMenuAction::Delete, state)});
			return;
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_LOADGAME")
		{
			fw().stageQueueCommand(
			    {StageCmd::Command::PUSH, mksp<SaveMenu>(SaveMenuAction::Load, state)});
			return;
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_GIVE_ALL_RESEARCH")
		{
			for (auto &r : this->state->research.topics)
			{
				LogWarning("Topic \"%s\"", r.first);
				auto &topic = r.second;
				if (topic->isComplete())
				{
					LogWarning("Topic \"%s\" already complete", r.first);
				}
				else
				{
					topic->man_hours_progress = topic->man_hours;
					LogWarning("Topic \"%s\" marked as complete", r.first);
				}
			}
			this->state->research.resortTopicList();
			return;
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_BATTLE")
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
			    static_cast<float>(slider->getValue()) / static_cast<float>(slider->Maximum);
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
			    static_cast<float>(slider->getValue()) / static_cast<float>(slider->Maximum);
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
			    static_cast<float>(slider->getValue()) / static_cast<float>(slider->Maximum);
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
