
#include "game/debugtools/formpreview.h"
#include "framework/framework.h"

namespace OpenApoc
{

FormPreview::FormPreview(Framework &fw) : Stage(fw)
{
	previewselector = new Form( fw, nullptr );
	previewselector->Size.x = 200;
	previewselector->Size.y = 300;
	previewselector->Location.x = 2;
	previewselector->Location.y = 2;
	previewselector->BackgroundColour.r = 192;
	previewselector->BackgroundColour.g = 192;
	previewselector->BackgroundColour.b = 192;

	Control* ch = new Control( fw, previewselector );
	ch->Location.x = 2;
	ch->Location.y = 2;
	ch->Size.x = previewselector->Size.x - 4;
	ch->Size.y = previewselector->Size.y - 4;
	ch->BackgroundColour.r = 80;
	ch->BackgroundColour.g = 80;
	ch->BackgroundColour.b = 80;

	Control* c = new Control( fw, ch );
	c->Location.x = 2;
	c->Location.y = 2;
	c->Size.x = previewselector->Size.x - 4;
	c->Size.y = previewselector->Size.y - 4;
	c->BackgroundColour.r = 80;
	c->BackgroundColour.g = 80;
	c->BackgroundColour.b = 80;




	Label* l = new Label( fw, c, "Pick Form:", fw.gamecore->GetFont( "SMALFONT" ) );
	l->Location.x = 0;
	l->Location.y = 0;
	l->Size.x = c->Size.x;
	l->Size.y = fw.gamecore->GetFont( "SMALFONT" )->GetFontHeight();
	l->BackgroundColour.b = l->BackgroundColour.r;
	l->BackgroundColour.g = l->BackgroundColour.r;

	interactWithDisplay = new CheckBox( fw, c );
	interactWithDisplay->Size.y = fw.gamecore->GetFont( "SMALFONT" )->GetFontHeight();
	interactWithDisplay->Size.x = interactWithDisplay->Size.y;
	interactWithDisplay->Location.x = 0;
	interactWithDisplay->Location.y = c->Size.y - interactWithDisplay->Size.y;
	interactWithDisplay->BackgroundColour.r = 80;
	interactWithDisplay->BackgroundColour.g = 80;
	interactWithDisplay->BackgroundColour.b = 80;

	l = new Label( fw, c, "Interact?", fw.gamecore->GetFont( "SMALFONT" ) );
	l->Location.x = interactWithDisplay->Size.x + 2;
	l->Location.y = interactWithDisplay->Location.y;
	l->Size.x = c->Size.x - l->Location.x;
	l->Size.y = interactWithDisplay->Size.y;
	l->BackgroundColour.r = 80;
	l->BackgroundColour.g = 80;
	l->BackgroundColour.b = 80;

	ListBox* lb = new ListBox( fw, c );
	lb->Location.x = 0;
	lb->Location.y = fw.gamecore->GetFont( "SMALFONT" )->GetFontHeight();
	lb->Size.x = c->Size.x;
	lb->Size.y = interactWithDisplay->Location.y - lb->Location.y;
	lb->Name = "FORM_LIST";
	lb->ItemHeight = lb->Location.y;
	lb->BackgroundColour.r = 24;
	lb->BackgroundColour.g = 24;
	lb->BackgroundColour.b = 24;

	std::vector<UString> idlist = fw.gamecore->GetFormIDs();
	for( auto idx = idlist.begin(); idx != idlist.end(); idx++ )
	{
		l = new Label( fw, lb, (UString)*idx, fw.gamecore->GetFont( "SMALFONT" ) );
		l->Name = l->GetText();
		l->BackgroundColour.r = 192;
		l->BackgroundColour.g = 80;
		l->BackgroundColour.b = 80;
		l->BackgroundColour.a = 0;
		// lb->AddItem( l );
	}

	displayform = nullptr;
	currentSelected = nullptr;
}

FormPreview::~FormPreview()
{
	delete previewselector;
}

void FormPreview::Begin() {}

void FormPreview::Pause() {}

void FormPreview::Resume() {}

void FormPreview::Finish() {}

void FormPreview::EventOccurred(Event *e)
{
	previewselector->EventOccured(e);
	if( displayform != nullptr && interactWithDisplay->Checked )
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

	if (e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.EventFlag == FormEventType::MouseClick)
	{
		if ( e->Data.Forms.RaisedBy->GetForm() == previewselector && e->Data.Forms.RaisedBy->GetParent()->Name == "FORM_LIST")
		{
			if( currentSelected != nullptr )
			{
				currentSelected->BackgroundColour.a = 0;
			}
			currentSelected = (Label*)e->Data.Forms.RaisedBy;
			currentSelected->BackgroundColour.a = 255;
			displayform = fw.gamecore->GetForm( e->Data.Forms.RaisedBy->Name );
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
