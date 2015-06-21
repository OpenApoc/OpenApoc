
#include "game/general/ingameoptions.h"
#include "framework/framework.h"

namespace OpenApoc {

InGameOptions::InGameOptions(Framework &fw)
	: Stage(fw)
{
	menuform = fw.gamecore->GetForm("FORM_INGAMEOPTIONS");

	/* Initialse all initial values */

	Control *c = menuform->FindControl("GLOBAL_GAIN_SLIDER");
	if (!c)
	{
		LogError("Failed to find \"GLOBAL_GAIN_SLIDER\" control");
		return;
	}
	HScrollBar *slider = dynamic_cast<HScrollBar*>(c);
	if (!slider)
	{
		LogError("Failed to cast \"GLOBAL_GAIN_SLIDER\" control to HScrollBar");
		return;
	}
	slider->SetValue(fw.Settings->getInt("Audio.GlobalGain"));

	c = menuform->FindControl("MUSIC_GAIN_SLIDER");
	if (!c)
	{
		LogError("Failed to find \"MUSIC_GAIN_SLIDER\" control");
		return;
	}
	slider = dynamic_cast<HScrollBar*>(c);
	if (!slider)
	{
		LogError("Failed to cast \"MUSIC_GAIN_SLIDER\" control to HScrollBar");
		return;
	}
	slider->SetValue(fw.Settings->getInt("Audio.MusicGain"));

	c = menuform->FindControl("SAMPLE_GAIN_SLIDER");
	if (!c)
	{
		LogError("Failed to find \"SAMPLE_GAIN_SLIDER\" control");
		return;
	}
	slider = dynamic_cast<HScrollBar*>(c);
	if (!slider)
	{
		LogError("Failed to cast \"SAMPLE_GAIN_SLIDER\" control to HScrollBar");
		return;
	}
	slider->SetValue(fw.Settings->getInt("Audio.SampleGain"));
}

InGameOptions::~InGameOptions()
{
	/* Store persistent options */

	Control *c = menuform->FindControl("GLOBAL_GAIN_SLIDER");
	if (!c)
	{
		LogError("Failed to find \"GLOBAL_GAIN_SLIDER\" control");
		return;
	}
	HScrollBar *slider = dynamic_cast<HScrollBar*>(c);
	if (!slider)
	{
		LogError("Failed to cast \"GLOBAL_GAIN_SLIDER\" control to HScrollBar");
		return;
	}
	fw.Settings->set("Audio.GlobalGain", slider->Value);

	c = menuform->FindControl("MUSIC_GAIN_SLIDER");
	if (!c)
	{
		LogError("Failed to find \"MUSIC_GAIN_SLIDER\" control");
		return;
	}
	slider = dynamic_cast<HScrollBar*>(c);
	if (!slider)
	{
		LogError("Failed to cast \"MUSIC_GAIN_SLIDER\" control to HScrollBar");
		return;
	}
	fw.Settings->set("Audio.MusicGain", slider->Value);

	c = menuform->FindControl("SAMPLE_GAIN_SLIDER");
	if (!c)
	{
		LogError("Failed to find \"SAMPLE_GAIN_SLIDER\" control");
		return;
	}
	slider = dynamic_cast<HScrollBar*>(c);
	if (!slider)
	{
		LogError("Failed to cast \"SAMPLE_GAIN_SLIDER\" control to HScrollBar");
		return;
	}
	fw.Settings->set("Audio.SampleGain", slider->Value);
}

void InGameOptions::Begin()
{
}

void InGameOptions::Pause()
{
}

void InGameOptions::Resume()
{
}

void InGameOptions::Finish()
{
}

void InGameOptions::EventOccurred(Event *e)
{
	menuform->EventOccured( e );
	fw.gamecore->MouseCursor->EventOccured( e );

	if( e->Type == EVENT_KEY_DOWN )
	{
		if( e->Data.Keyboard.KeyCode == ALLEGRO_KEY_ESCAPE )
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}

	if( e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.EventFlag == FormEventType::ButtonClick )
	{
		if (e->Data.Forms.RaisedBy->Name == "BUTTON_QUIT")
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
		else if (e->Data.Forms.RaisedBy->Name == "GLOBAL_GAIN_DOWN")
		{
			Control *c = menuform->FindControl("GLOBAL_GAIN_SLIDER");
			if (!c)
			{
				LogError("Failed to find \"GLOBAL_GAIN_SLIDER\" control");
				return;
			}
			HScrollBar *slider = dynamic_cast<HScrollBar*>(c);
			if (!slider)
			{
				LogError("Failed to cast \"GLOBAL_GAIN_SLIDER\" control to HScrollBar");
				return;
			}
			slider->SetValue(slider->Value - 1);
		}
		else if (e->Data.Forms.RaisedBy->Name == "GLOBAL_GAIN_UP")
		{
			Control *c = menuform->FindControl("GLOBAL_GAIN_SLIDER");
			if (!c)
			{
				LogError("Failed to find \"GLOBAL_GAIN_SLIDER\" control");
				return;
			}
			HScrollBar *slider = dynamic_cast<HScrollBar*>(c);
			if (!slider)
			{
				LogError("Failed to cast \"GLOBAL_GAIN_SLIDER\" control to HScrollBar");
				return;
			}
			slider->SetValue(slider->Value + 1);
		}
		else if (e->Data.Forms.RaisedBy->Name == "MUSIC_GAIN_DOWN")
		{
			Control *c = menuform->FindControl("MUSIC_GAIN_SLIDER");
			if (!c)
			{
				LogError("Failed to find \"MUSIC_GAIN_SLIDER\" control");
				return;
			}
			HScrollBar *slider = dynamic_cast<HScrollBar*>(c);
			if (!slider)
			{
				LogError("Failed to cast \"MUSIC_GAIN_SLIDER\" control to HScrollBar");
				return;
			}
			slider->SetValue(slider->Value - 1);
		}
		else if (e->Data.Forms.RaisedBy->Name == "MUSIC_GAIN_UP")
		{
			Control *c = menuform->FindControl("MUSIC_GAIN_SLIDER");
			if (!c)
			{
				LogError("Failed to find \"MUSIC_GAIN_SLIDER\" control");
				return;
			}
			HScrollBar *slider = dynamic_cast<HScrollBar*>(c);
			if (!slider)
			{
				LogError("Failed to cast \"MUSIC_GAIN_SLIDER\" control to HScrollBar");
				return;
			}
			slider->SetValue(slider->Value + 1);
		}
		else if (e->Data.Forms.RaisedBy->Name == "SAMPLE_GAIN_DOWN")
		{
			Control *c = menuform->FindControl("SAMPLE_GAIN_SLIDER");
			if (!c)
			{
				LogError("Failed to find \"SAMPLE_GAIN_SLIDER\" control");
				return;
			}
			HScrollBar *slider = dynamic_cast<HScrollBar*>(c);
			if (!slider)
			{
				LogError("Failed to cast \"SAMPLE_GAIN_SLIDER\" control to HScrollBar");
				return;
			}
			slider->SetValue(slider->Value - 1);
		}
		else if (e->Data.Forms.RaisedBy->Name == "SAMPLE_GAIN_UP")
		{
			Control *c = menuform->FindControl("SAMPLE_GAIN_SLIDER");
			if (!c)
			{
				LogError("Failed to find \"SAMPLE_GAIN_SLIDER\" control");
				return;
			}
			HScrollBar *slider = dynamic_cast<HScrollBar*>(c);
			if (!slider)
			{
				LogError("Failed to cast \"SAMPLE_GAIN_SLIDER\" control to HScrollBar");
				return;
			}
			slider->SetValue(slider->Value + 1);
		}
	}
	if( e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.EventFlag == FormEventType::ScrollBarChange )
	{
		if (e->Data.Forms.RaisedBy->Name == "GLOBAL_GAIN_SLIDER")
		{
			HScrollBar *slider = dynamic_cast<HScrollBar*>(e->Data.Forms.RaisedBy);
			if (!slider)
			{
				LogError("Failed to cast \"GLOBAL_GAIN_SLIDER\" control to HScrollBar");
				return;
			}
			float gain = (float)slider->Value / (float)slider->Maximum;
			fw.soundBackend->setGain(SoundBackend::Gain::Global, gain);
		}
		else if (e->Data.Forms.RaisedBy->Name == "MUSIC_GAIN_SLIDER")
		{
			HScrollBar *slider = dynamic_cast<HScrollBar*>(e->Data.Forms.RaisedBy);
			if (!slider)
			{
				LogError("Failed to cast \"MUSIC_GAIN_SLIDER\" control to HScrollBar");
				return;
			}
			float gain = (float)slider->Value / (float)slider->Maximum;
			fw.soundBackend->setGain(SoundBackend::Gain::Music, gain);
		}
		else if (e->Data.Forms.RaisedBy->Name == "SAMPLE_GAIN_SLIDER")
		{
			HScrollBar *slider = dynamic_cast<HScrollBar*>(e->Data.Forms.RaisedBy);
			if (!slider)
			{
				LogError("Failed to cast \"SAMPLE_GAIN_SLIDER\" control to HScrollBar");
				return;
			}
			float gain = (float)slider->Value / (float)slider->Maximum;
			fw.soundBackend->setGain(SoundBackend::Gain::Sample, gain);
		}

	}
}

void InGameOptions::Update(StageCmd * const cmd)
{
	menuform->Update();
	*cmd = this->stageCmd;
	//Reset the command to default
	this->stageCmd = StageCmd();
}

void InGameOptions::Render()
{
	fw.Stage_GetPrevious(this->shared_from_this())->Render();
	fw.renderer->drawFilledRect({0,0}, fw.Display_GetSize(), Colour{0,0,0,128});
	menuform->Render();
	fw.gamecore->MouseCursor->Render();
}

bool InGameOptions::IsTransition()
{
	return false;
}

}; //namespace OpenApoc
