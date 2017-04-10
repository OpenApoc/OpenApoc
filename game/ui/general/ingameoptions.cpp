#include "game/ui/general/ingameoptions.h"
#include "forms/checkbox.h"
#include "forms/form.h"
#include "forms/label.h"
#include "forms/scrollbar.h"
#include "forms/textbutton.h"
#include "forms/ui.h"
#include "framework/configfile.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "framework/sound.h"
#include "game/state/battle/battle.h"
#include "game/state/gamestate.h"
#include "game/ui/battle/battledebriefing.h"
#include "game/ui/city/cityview.h"
#include "game/ui/general/mainmenu.h"
#include "game/ui/general/mapselector.h"
#include "game/ui/general/messagebox.h"
#include "game/ui/general/savemenu.h"

namespace OpenApoc
{

InGameOptions::InGameOptions(sp<GameState> state)
    : Stage(), menuform(ui().getForm("ingameoptions")), state(state)
{

	/* Initialse all initial values */

	menuform->findControlTyped<ScrollBar>("GLOBAL_GAIN_SLIDER")
	    ->setValue(config().getInt("Framework.Audio.GlobalGain"));

	menuform->findControlTyped<ScrollBar>("MUSIC_GAIN_SLIDER")
	    ->setValue(config().getInt("Framework.Audio.MusicGain"));

	menuform->findControlTyped<ScrollBar>("SAMPLE_GAIN_SLIDER")
	    ->setValue(config().getInt("Framework.Audio.SampleGain"));

	menuform->findControlTyped<CheckBox>("SHOW_VEHICLE_PATH")->setChecked(state->showVehiclePath);
	menuform->findControlTyped<CheckBox>("SHOW_TILE_ORIGIN")->setChecked(state->showTileOrigin);
	menuform->findControlTyped<CheckBox>("SHOW_SELECTABLE_BOUNDS")
	    ->setChecked(state->showSelectableBounds);

	menuform->findControlTyped<TextButton>("BUTTON_BATTLE")
	    ->setText(state->current_battle ? "Exit Battle" : "Enter Battle");
}

InGameOptions::~InGameOptions()
{
	/* Store persistent options */

	config().set("Framework.Audio.GlobalGain",
	             menuform->findControlTyped<ScrollBar>("GLOBAL_GAIN_SLIDER")->getValue());
	config().set("Framework.Audio.MusicGain",
	             menuform->findControlTyped<ScrollBar>("MUSIC_GAIN_SLIDER")->getValue());
	config().set("Framework.Audio.SampleGain",
	             menuform->findControlTyped<ScrollBar>("SAMPLE_GAIN_SLIDER")->getValue());
}

void InGameOptions::begin()
{
	menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
}

void InGameOptions::pause() {}

void InGameOptions::resume() {}

void InGameOptions::finish() {}

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
				int unitsLost = state->current_battle->killStrandedUnits(*state, true);
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH,
				     mksp<MessageBox>(tr("Abort Mission"),
				                      format("%s %d", tr("Units Lost :"), unitsLost),
				                      MessageBox::ButtonOptions::YesNo, [this] {
					                      state->current_battle->abortMission(*state);
					                      fw().stageQueueCommand({StageCmd::Command::REPLACEALL,
					                                              mksp<BattleDebriefing>(state)});
					                  })});
			}
			else
			{
				fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<MapSelector>(state)});
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
		if (e->forms().RaisedBy->Name == "SHOW_VEHICLE_PATH")
		{
			auto box = std::dynamic_pointer_cast<CheckBox>(e->forms().RaisedBy);
			state->showVehiclePath = box->isChecked();
			LogWarning("Set SHOW_VEHICLE_PATH to %d", box->isChecked());
		}
		if (e->forms().RaisedBy->Name == "SHOW_TILE_ORIGIN")
		{
			auto box = std::dynamic_pointer_cast<CheckBox>(e->forms().RaisedBy);
			state->showTileOrigin = box->isChecked();
			LogWarning("Set SHOW_TILE_ORIGIN to %d", box->isChecked());
		}
		if (e->forms().RaisedBy->Name == "SHOW_SELECTABLE_BOUNDS")
		{
			auto box = std::dynamic_pointer_cast<CheckBox>(e->forms().RaisedBy);
			state->showSelectableBounds = box->isChecked();
			LogWarning("Set SHOW_SELECTABLE_BOUNDS to %d", box->isChecked());
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
