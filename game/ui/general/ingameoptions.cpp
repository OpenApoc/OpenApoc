#include "game/ui/general/ingameoptions.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/gamestate.h"
#include "game/ui/general/mainmenu.h"
#include "game/ui/general/savemenu.h"

namespace OpenApoc
{

InGameOptions::InGameOptions(sp<GameState> state)
    : Stage(), menuform(ui().getForm("FORM_INGAMEOPTIONS")), state(state)
{

	/* Initialse all initial values */

	menuform->findControlTyped<ScrollBar>("GLOBAL_GAIN_SLIDER")
	    ->setValue(fw().Settings->getInt("Audio.GlobalGain"));

	menuform->findControlTyped<ScrollBar>("MUSIC_GAIN_SLIDER")
	    ->setValue(fw().Settings->getInt("Audio.MusicGain"));

	menuform->findControlTyped<ScrollBar>("SAMPLE_GAIN_SLIDER")
	    ->setValue(fw().Settings->getInt("Audio.SampleGain"));

	menuform->findControlTyped<CheckBox>("SHOW_VEHICLE_PATH")->setChecked(state->showVehiclePath);
	menuform->findControlTyped<CheckBox>("SHOW_TILE_ORIGIN")->setChecked(state->showTileOrigin);
	menuform->findControlTyped<CheckBox>("SHOW_SELECTABLE_BOUNDS")
	    ->setChecked(state->showSelectableBounds);
}

InGameOptions::~InGameOptions()
{
	/* Store persistent options */

	fw().Settings->set("Audio.GlobalGain",
	                   menuform->findControlTyped<ScrollBar>("GLOBAL_GAIN_SLIDER")->getValue());
	fw().Settings->set("Audio.MusicGain",
	                   menuform->findControlTyped<ScrollBar>("MUSIC_GAIN_SLIDER")->getValue());
	fw().Settings->set("Audio.SampleGain",
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
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().EventFlag == FormEventType::ButtonClick)
	{
		if (e->forms().RaisedBy->Name == "BUTTON_OK")
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_ABANDONGAME")
		{
			stageCmd.cmd = StageCmd::Command::REPLACEALL;
			stageCmd.nextStage = mksp<MainMenu>();
			return;
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_QUIT")
		{
			stageCmd.cmd = StageCmd::Command::QUIT;
			return;
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_SAVEGAME")
		{
			stageCmd.cmd = StageCmd::Command::PUSH;
			stageCmd.nextStage = mksp<SaveMenu>(SaveMenuAction::Save, state);
			return;
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_DELETESAVEDGAME")
		{
			stageCmd.cmd = StageCmd::Command::PUSH;
			stageCmd.nextStage = mksp<SaveMenu>(SaveMenuAction::Delete, state);
			return;
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_LOADGAME")
		{
			stageCmd.cmd = StageCmd::Command::PUSH;
			stageCmd.nextStage = mksp<SaveMenu>(SaveMenuAction::Load, state);
			return;
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_GIVE_ALL_RESEARCH")
		{
			for (auto &r : this->state->research.topics)
			{
				LogWarning("Topic \"%s\"", r.first.cStr());
				auto &topic = r.second;
				if (topic->isComplete())
				{
					LogWarning("Topic \"%s\" already complete", r.first.cStr());
				}
				else
				{
					topic->man_hours_progress = topic->man_hours;
					LogWarning("Topic \"%s\" marked as complete", r.first.cStr());
				}
			}
			return;
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

void InGameOptions::update(StageCmd *const cmd)
{
	menuform->update();
	*cmd = this->stageCmd;
	// Reset the command to default
	this->stageCmd = StageCmd();
}

void InGameOptions::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	fw().renderer->drawFilledRect({0, 0}, fw().displayGetSize(), Colour{0, 0, 0, 128});
	menuform->render();
}

bool InGameOptions::isTransition() { return false; }

}; // namespace OpenApoc
