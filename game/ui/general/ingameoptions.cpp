#include "game/ui/general/ingameoptions.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/gamestate.h"
#include "game/ui/general/mainmenu.h"

namespace OpenApoc
{

InGameOptions::InGameOptions(sp<GameState> state)
    : Stage(), menuform(ui().GetForm("FORM_INGAMEOPTIONS")), state(state)
{

	/* Initialse all initial values */

	menuform->FindControlTyped<ScrollBar>("GLOBAL_GAIN_SLIDER")
	    ->SetValue(fw().Settings->getInt("Audio.GlobalGain"));

	menuform->FindControlTyped<ScrollBar>("MUSIC_GAIN_SLIDER")
	    ->SetValue(fw().Settings->getInt("Audio.MusicGain"));

	menuform->FindControlTyped<ScrollBar>("SAMPLE_GAIN_SLIDER")
	    ->SetValue(fw().Settings->getInt("Audio.SampleGain"));

	menuform->FindControlTyped<CheckBox>("SHOW_VEHICLE_PATH")->SetChecked(state->showVehiclePath);
	menuform->FindControlTyped<CheckBox>("SHOW_TILE_ORIGIN")->SetChecked(state->showTileOrigin);
	menuform->FindControlTyped<CheckBox>("SHOW_SELECTABLE_BOUNDS")
	    ->SetChecked(state->showSelectableBounds);
}

InGameOptions::~InGameOptions()
{
	/* Store persistent options */

	fw().Settings->set("Audio.GlobalGain",
	                   menuform->FindControlTyped<ScrollBar>("GLOBAL_GAIN_SLIDER")->GetValue());
	fw().Settings->set("Audio.MusicGain",
	                   menuform->FindControlTyped<ScrollBar>("MUSIC_GAIN_SLIDER")->GetValue());
	fw().Settings->set("Audio.SampleGain",
	                   menuform->FindControlTyped<ScrollBar>("SAMPLE_GAIN_SLIDER")->GetValue());
}

void InGameOptions::Begin() {}

void InGameOptions::Pause() {}

void InGameOptions::Resume() {}

void InGameOptions::Finish() {}

void InGameOptions::EventOccurred(Event *e)
{
	menuform->EventOccured(e);

	if (e->Type() == EVENT_KEY_DOWN)
	{
		if (e->Keyboard().KeyCode == SDLK_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}

	if (e->Type() == EVENT_FORM_INTERACTION && e->Forms().EventFlag == FormEventType::ButtonClick)
	{
		if (e->Forms().RaisedBy->Name == "BUTTON_OK")
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
		else if (e->Forms().RaisedBy->Name == "BUTTON_ABANDONGAME")
		{
			stageCmd.cmd = StageCmd::Command::REPLACE;
			stageCmd.nextStage = mksp<MainMenu>();
			return;
		}
		else if (e->Forms().RaisedBy->Name == "BUTTON_QUIT")
		{
			stageCmd.cmd = StageCmd::Command::QUIT;
			return;
		}
		else if (e->Forms().RaisedBy->Name == "BUTTON_SAVEGAME")
		{
			// FIXME: Save game selector
			this->state->saveGame("save");
			return;
		}
	}
	if (e->Type() == EVENT_FORM_INTERACTION &&
	    e->Forms().EventFlag == FormEventType::ScrollBarChange)
	{
		if (e->Forms().RaisedBy->Name == "GLOBAL_GAIN_SLIDER")
		{
			auto slider = std::dynamic_pointer_cast<ScrollBar>(e->Forms().RaisedBy);
			if (!slider)
			{
				LogError("Failed to cast \"GLOBAL_GAIN_SLIDER\" control to ScrollBar");
				return;
			}
			float gain =
			    static_cast<float>(slider->GetValue()) / static_cast<float>(slider->Maximum);
			fw().soundBackend->setGain(SoundBackend::Gain::Global, gain);
		}
		else if (e->Forms().RaisedBy->Name == "MUSIC_GAIN_SLIDER")
		{
			auto slider = std::dynamic_pointer_cast<ScrollBar>(e->Forms().RaisedBy);
			if (!slider)
			{
				LogError("Failed to cast \"MUSIC_GAIN_SLIDER\" control to ScrollBar");
				return;
			}
			float gain =
			    static_cast<float>(slider->GetValue()) / static_cast<float>(slider->Maximum);
			fw().soundBackend->setGain(SoundBackend::Gain::Music, gain);
		}
		else if (e->Forms().RaisedBy->Name == "SAMPLE_GAIN_SLIDER")
		{
			auto slider = std::dynamic_pointer_cast<ScrollBar>(e->Forms().RaisedBy);
			if (!slider)
			{
				LogError("Failed to cast \"SAMPLE_GAIN_SLIDER\" control to ScrollBar");
				return;
			}
			float gain =
			    static_cast<float>(slider->GetValue()) / static_cast<float>(slider->Maximum);
			fw().soundBackend->setGain(SoundBackend::Gain::Sample, gain);
		}
	}
	if (e->Type() == EVENT_FORM_INTERACTION &&
	    e->Forms().EventFlag == FormEventType::CheckBoxChange)
	{
		if (e->Forms().RaisedBy->Name == "SHOW_VEHICLE_PATH")
		{
			auto box = std::dynamic_pointer_cast<CheckBox>(e->Forms().RaisedBy);
			state->showVehiclePath = box->IsChecked();
			LogWarning("Set SHOW_VEHICLE_PATH to %d", box->IsChecked());
		}
		if (e->Forms().RaisedBy->Name == "SHOW_TILE_ORIGIN")
		{
			auto box = std::dynamic_pointer_cast<CheckBox>(e->Forms().RaisedBy);
			state->showTileOrigin = box->IsChecked();
			LogWarning("Set SHOW_TILE_ORIGIN to %d", box->IsChecked());
		}
		if (e->Forms().RaisedBy->Name == "SHOW_SELECTABLE_BOUNDS")
		{
			auto box = std::dynamic_pointer_cast<CheckBox>(e->Forms().RaisedBy);
			state->showSelectableBounds = box->IsChecked();
			LogWarning("Set SHOW_SELECTABLE_BOUNDS to %d", box->IsChecked());
		}
	}
}

void InGameOptions::Update(StageCmd *const cmd)
{
	menuform->Update();
	*cmd = this->stageCmd;
	// Reset the command to default
	this->stageCmd = StageCmd();
}

void InGameOptions::Render()
{
	fw().Stage_GetPrevious(this->shared_from_this())->Render();
	fw().renderer->drawFilledRect({0, 0}, fw().Display_GetSize(), Colour{0, 0, 0, 128});
	menuform->Render();
}

bool InGameOptions::IsTransition() { return false; }

}; // namespace OpenApoc
