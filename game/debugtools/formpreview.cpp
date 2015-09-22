
#include "game/debugtools/formpreview.h"
#include "framework/framework.h"

namespace OpenApoc
{

FormPreview::FormPreview(Framework &fw) : Stage(fw)
{
	previewselector = new Form( fw, nullptr );
	previewselector->Size.x = 120;
	previewselector->Size.y = 60;
	previewselector->Location.x = 0;
	previewselector->Location.y = 0;
	//previewselector->BackgroundColour = new Colour( 40, 40, 40, 255 );
	displayform = nullptr;
}

FormPreview::~FormPreview() {}

void FormPreview::Begin() {}

void FormPreview::Pause() {}

void FormPreview::Resume() {}

void FormPreview::Finish() {}

void FormPreview::EventOccurred(Event *e)
{
	previewselector->EventOccured(e);
	if( displayform != nullptr )
	{
		displayform->EventOccured(e);
	}
	fw.gamecore->MouseCursor->EventOccured(e);

	if (e->Type == EVENT_KEY_DOWN)
	{
		if (e->Data.Keyboard.KeyCode == ALLEGRO_KEY_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}

	if (e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.EventFlag == FormEventType::ButtonClick)
	{
		if (e->Data.Forms.RaisedBy->Name == "PREVIEW_FORM")
		{
			// menuform = fw.gamecore->GetForm("FORM_DEBUG_MENU");
			return;
		}
	}
}

void FormPreview::Update(StageCmd *const cmd)
{
	previewselector->Update();
	if( displayform != nullptr )
	{
		displayform->Update();
	}
	*cmd = this->stageCmd;
	// Reset the command to default
	this->stageCmd = StageCmd();
}

void FormPreview::Render()
{
	previewselector->Render();
	if( displayform != nullptr )
	{
		displayform->Render();
	}
	fw.gamecore->MouseCursor->Render();
}

bool FormPreview::IsTransition() { return false; }

}; // namespace OpenApoc
