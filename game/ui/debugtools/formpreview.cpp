#include "game/ui/debugtools/formpreview.h"
#include "forms/checkbox.h"
#include "forms/form.h"
#include "forms/label.h"
#include "forms/listbox.h"
#include "forms/textbutton.h"
#include "forms/ui.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/font.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "framework/renderer.h"

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

	auto l = c->createChild<Label>("Pick Form:", ui().getFont("smalfont"));
	l->Location = {0, 0};
	l->Size.x = c->Size.x;
	l->Size.y = ui().getFont("smalfont")->getFontHeight();
	l->BackgroundColour = {80, 80, 80, 255};

	interactWithDisplay = c->createChild<CheckBox>(
	    fw().data->loadImage(
	        "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:65:ui/menuopt.pal"),
	    fw().data->loadImage(
	        "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:64:ui/menuopt.pal"));
	interactWithDisplay->Size = {19, 16};
	interactWithDisplay->Location.x = 0;
	interactWithDisplay->Location.y = c->Size.y - interactWithDisplay->Size.y * 2;
	interactWithDisplay->BackgroundColour = {80, 80, 80, 255};
	interactWithDisplay->setChecked(true);

	l = c->createChild<Label>("Interact?", ui().getFont("smalfont"));
	l->Location.x = interactWithDisplay->Size.x + 2;
	l->Location.y = interactWithDisplay->Location.y;
	l->Size.x = c->Size.x - l->Location.x;
	l->Size.y = interactWithDisplay->Size.y;
	l->BackgroundColour = {80, 80, 80, 255};

	reloadButton = c->createChild<TextButton>("Reload xml forms", ui().getFont("smalfont"));
	reloadButton->Location = {0, interactWithDisplay->Location.y + interactWithDisplay->Size.y};
	reloadButton->Name = "FORM_RELOAD";
	reloadButton->Size = {c->Size.x, interactWithDisplay->Size.y};
	reloadButton->BackgroundColour = {80, 80, 80, 255};

	auto lb = c->createChild<ListBox>();
	lb->Location.x = 0;
	lb->Location.y = ui().getFont("smalfont")->getFontHeight();
	lb->Size.x = c->Size.x;
	lb->Size.y = interactWithDisplay->Location.y - lb->Location.y;
	lb->Name = "FORM_LIST";
	lb->ItemSize = lb->Location.y;
	lb->BackgroundColour = {24, 24, 24, 255};

	std::vector<UString> idlist = ui().getFormIDs();
	for (auto idx = idlist.begin(); idx != idlist.end(); idx++)
	{
		l = lb->createChild<Label>(*idx, ui().getFont("smalfont"));
		l->Name = l->getText();
		l->BackgroundColour = {192, 80, 80, 0};
		// lb->addItem( l );
	}

	propertyeditor = mksp<Form>();
	propertyeditor->Location = {2, 304};
	propertyeditor->Size.x = 200;
	propertyeditor->Size.y = fw().displayGetHeight() - propertyeditor->Location.y - 2;
	propertyeditor->BackgroundColour = {192, 192, 192, 255};
}

FormPreview::~FormPreview() = default;

void FormPreview::begin() {}

void FormPreview::pause() {}

void FormPreview::resume() {}

void FormPreview::finish() {}

void FormPreview::eventOccurred(Event *e)
{
	previewselector->eventOccured(e);
	if (propertyeditor != nullptr)
	{
		propertyeditor->eventOccured(e);
	}
	if (displayform != nullptr && interactWithDisplay->isChecked())
	{
		displayform->eventOccured(e);
	}

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE)
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
			return;
		}
	}

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().EventFlag == FormEventType::MouseClick)
	{

		if (displayform != nullptr && e->forms().RaisedBy->getForm() == displayform)
		{
			currentSelectedControl = e->forms().RaisedBy;
			configureSelectedControlForm();
		}
		else
		{
			currentSelectedControl = nullptr;
			configureSelectedControlForm();
		}

		if (e->forms().RaisedBy->getForm() == previewselector &&
		    e->forms().RaisedBy->getParent() != nullptr &&
		    e->forms().RaisedBy->getParent()->Name == "FORM_LIST")
		{

			if (currentSelected != nullptr)
			{
				currentSelected->BackgroundColour.a = 0;
				currentSelected->setDirty();
			}

			currentSelected = std::dynamic_pointer_cast<Label>(e->forms().RaisedBy);
			if (currentSelected != nullptr)
			{
				currentSelected->BackgroundColour.a = 255;
				displayform = ui().getForm(currentSelected->Name);
			}

			return;
		}
		else if (e->forms().RaisedBy->Name == "FORM_RELOAD")
		{
			if (currentSelected != nullptr)
			{
				ui().reloadFormsXml();
				displayform = ui().getForm(currentSelected->Name);
			}
		}
	}
}

void FormPreview::update()
{
	previewselector->update();
	if (propertyeditor != nullptr)
	{
		propertyeditor->update();
	}
	if (displayform != nullptr)
	{
		displayform->update();
	}

	glowindex = (glowindex + 4) % 511;
}

void FormPreview::render()
{
	if (displayform != nullptr)
	{
		displayform->render();
	}
	previewselector->render();
	if (propertyeditor != nullptr)
	{
		propertyeditor->render();
	}

	if (currentSelectedControl != nullptr)
	{
		Vec2<int> border = currentSelectedControl->getLocationOnScreen();
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

bool FormPreview::isTransition() { return false; }

void FormPreview::configureSelectedControlForm()
{

	if (currentSelectedControl == nullptr)
	{
	}
	else
	{
	}
}

}; // namespace OpenApoc
