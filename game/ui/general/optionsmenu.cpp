#include "game/ui/general/optionsmenu.h"
#include "forms/form.h"
#include "forms/label.h"
#include "forms/listbox.h"
#include "forms/textbutton.h"
#include "forms/textedit.h"
#include "forms/ui.h"
#include "framework/configfile.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"

namespace OpenApoc
{

OptionsMenu::OptionsMenu() : Stage(), menuform(ui().getForm("options"))
{
	auto options = config().getOptions();
	auto listbox = menuform->findControlTyped<ListBox>("LISTBOX_OPTIONS");
	for (auto &section : options)
	{
		if (!section.first.empty())
		{
			listbox->addItem(mksp<TextButton>(section.first, ui().getFont("smalfont")));
			for (auto &opt : section.second)
			{
				listbox->addItem(createOptionRow(opt));
			}
		}
	}
}

OptionsMenu::~OptionsMenu() = default;

sp<Control> OptionsMenu::createOptionRow(const ConfigOption &option)
{
	auto control = mksp<Control>();

	const int HEIGHT = 21;

	auto label = control->createChild<Label>(option.getName(), ui().getFont("smalfont"));
	label->Location = {0, 0};
	label->Size = {250, HEIGHT};
	label->TextVAlign = VerticalAlignment::Centre;

	/*
	auto value = control->createChild<TextEdit>(config().getString(option.getKey()),
	                                            ui().getFont("smalfont"));
	value->Location = {0, 0};
	value->Size = {250, HEIGHT};
	value->TextVAlign = VerticalAlignment::Centre;
	*/

	control->ToolTipText = option.getDescription();
	control->ToolTipFont = ui().getFont("smallset");

	return control;
}

void OptionsMenu::begin() {}

void OptionsMenu::pause() {}

void OptionsMenu::resume() {}

void OptionsMenu::finish() {}

void OptionsMenu::eventOccurred(Event *e)
{
	menuform->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE || e->keyboard().KeyCode == SDLK_RETURN ||
		    e->keyboard().KeyCode == SDLK_KP_ENTER)
		{
			menuform->findControl("BUTTON_OK")->click();
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
	}
}

void OptionsMenu::update() { menuform->update(); }

void OptionsMenu::render() { menuform->render(); }

bool OptionsMenu::isTransition() { return false; }

}; // namespace OpenApoc
