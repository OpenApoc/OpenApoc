#include "game/ui/debugtools/formpreview.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"

namespace OpenApoc
{

FormPreview::FormPreview() : Stage()
{
	displayform = nullptr;
	currentSelected = nullptr;
	currentSelectedControl = nullptr;
	glowindex = 0;

	previewselector = mksp<Form>();
	previewselector->Size = {200, 300};
	previewselector->Location = {2, 2};
	previewselector->BackgroundColour = {192, 192, 192, 255};

	auto ch = previewselector->createChild<Control>();
	ch->Location = {2, 2};
	ch->Size = previewselector->Size - 4;
	ch->BackgroundColour = {80, 80, 80, 255};

	auto c = ch->createChild<Control>();
	c->Location = {2, 2};
	c->Size = previewselector->Size - 4;
	c->BackgroundColour = {80, 80, 80, 255};

	auto l = c->createChild<Label>("Pick Form:", ui().GetFont("SMALFONT"));
	l->Location = {0, 0};
	l->Size.x = c->Size.x;
	l->Size.y = ui().GetFont("SMALFONT")->GetFontHeight();
	l->BackgroundColour = {80, 80, 80, 255};

	interactWithDisplay = c->createChild<CheckBox>(
	    fw().data->load_image(
	        "PCK:XCOM3/UFODATA/NEWBUT.PCK:XCOM3/UFODATA/NEWBUT.TAB:65:UI/menuopt.pal"),
	    fw().data->load_image(
	        "PCK:XCOM3/UFODATA/NEWBUT.PCK:XCOM3/UFODATA/NEWBUT.TAB:64:UI/menuopt.pal"));
	interactWithDisplay->Size = {19, 16};
	interactWithDisplay->Location.x = 0;
	interactWithDisplay->Location.y = c->Size.y - interactWithDisplay->Size.y * 2;
	interactWithDisplay->BackgroundColour = {80, 80, 80, 255};
	interactWithDisplay->SetChecked(true);

	l = c->createChild<Label>("Interact?", ui().GetFont("SMALFONT"));
	l->Location.x = interactWithDisplay->Size.x + 2;
	l->Location.y = interactWithDisplay->Location.y;
	l->Size.x = c->Size.x - l->Location.x;
	l->Size.y = interactWithDisplay->Size.y;
	l->BackgroundColour = {80, 80, 80, 255};

	reloadButton = c->createChild<TextButton>("Reload xml forms", ui().GetFont("SMALFONT"));
	reloadButton->Location = {0, interactWithDisplay->Location.y + interactWithDisplay->Size.y};
	reloadButton->Name = "FORM_RELOAD";
	reloadButton->Size = {c->Size.x, interactWithDisplay->Size.y};
	reloadButton->BackgroundColour = {80, 80, 80, 255};

	auto lb = c->createChild<ListBox>();
	lb->Location.x = 0;
	lb->Location.y = ui().GetFont("SMALFONT")->GetFontHeight();
	lb->Size.x = c->Size.x;
	lb->Size.y = interactWithDisplay->Location.y - lb->Location.y;
	lb->Name = "FORM_LIST";
	lb->ItemSize = lb->Location.y;
	lb->BackgroundColour = {24, 24, 24, 255};

	std::vector<UString> idlist = ui().GetFormIDs();
	for (auto idx = idlist.begin(); idx != idlist.end(); idx++)
	{
		l = lb->createChild<Label>(*idx, ui().GetFont("SMALFONT"));
		l->Name = l->GetText();
		l->BackgroundColour = {192, 80, 80, 0};
		// lb->AddItem( l );
	}

	propertyeditor = mksp<Form>();
	propertyeditor->Location = {2, 304};
	propertyeditor->Size.x = 200;
	propertyeditor->Size.y = fw().Display_GetHeight() - propertyeditor->Location.y - 2;
	propertyeditor->BackgroundColour = {192, 192, 192, 255};
}

FormPreview::~FormPreview() {}

void FormPreview::Begin() {}

void FormPreview::Pause() {}

void FormPreview::Resume() {}

void FormPreview::Finish() {}

void FormPreview::EventOccurred(Event *e)
{
	previewselector->EventOccured(e);
	if (propertyeditor != nullptr)
	{
		propertyeditor->EventOccured(e);
	}
	if (displayform != nullptr && interactWithDisplay->IsChecked())
	{
		displayform->EventOccured(e);
	}

	if (e->Type() == EVENT_KEY_DOWN)
	{
		if (e->Keyboard().KeyCode == SDLK_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}

	if (e->Type() == EVENT_FORM_INTERACTION && e->Forms().EventFlag == FormEventType::MouseClick)
	{

		if (displayform != nullptr && e->Forms().RaisedBy->GetForm() == displayform)
		{
			currentSelectedControl = e->Forms().RaisedBy;
			ConfigureSelectedControlForm();
		}
		else
		{
			currentSelectedControl = nullptr;
			ConfigureSelectedControlForm();
		}

		if (e->Forms().RaisedBy->GetForm() == previewselector &&
		    e->Forms().RaisedBy->GetParent() != nullptr &&
		    e->Forms().RaisedBy->GetParent()->Name == "FORM_LIST")
		{

			if (currentSelected != nullptr)
			{
				currentSelected->BackgroundColour.a = 0;
			}

			currentSelected = std::dynamic_pointer_cast<Label>(e->Forms().RaisedBy);
			currentSelected->BackgroundColour.a = 255;
			displayform = ui().GetForm(currentSelected->Name);

			return;
		}
		else if (e->Forms().RaisedBy->Name == "FORM_RELOAD")
		{
			if (currentSelected != nullptr)
			{
				ui().reloadFormsXml();
				displayform = ui().GetForm(currentSelected->Name);
			}
		}
	}
}

void FormPreview::Update(StageCmd *const cmd)
{
	previewselector->Update();
	if (propertyeditor != nullptr)
	{
		propertyeditor->Update();
	}
	if (displayform != nullptr)
	{
		displayform->Update();
	}
	*cmd = this->stageCmd;
	// Reset the command to default
	this->stageCmd = StageCmd();

	glowindex = (glowindex + 4) % 511;
}

void FormPreview::Render()
{
	if (displayform != nullptr)
	{
		displayform->Render();
	}
	previewselector->Render();
	if (propertyeditor != nullptr)
	{
		propertyeditor->Render();
	}

	if (currentSelectedControl != nullptr)
	{
		Vec2<int> border = currentSelectedControl->GetLocationOnScreen();
		if (glowindex < 256)
		{
			fw().renderer->drawRect(border, currentSelectedControl->Size,
			                        Colour(glowindex, glowindex, glowindex), 3.0f);
		}
		else
		{
			int revglow = 255 - (glowindex - 256);
			fw().renderer->drawRect(border, currentSelectedControl->Size,
			                        Colour(revglow, revglow, revglow), 3.0f);
		}
	}
}

bool FormPreview::IsTransition() { return false; }

void FormPreview::ConfigureSelectedControlForm()
{

	if (currentSelectedControl == nullptr)
	{
	}
	else
	{
	}
}

}; // namespace OpenApoc
