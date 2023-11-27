#include "game/ui/general/creditsmenu.h"
#include "forms/checkbox.h"
#include "forms/form.h"
#include "forms/label.h"
#include "forms/listbox.h"
#include "forms/scrollbar.h"
#include "forms/ui.h"
#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "framework/keycodes.h"
#include "game/ui/components/controlgenerator.h"
#include "stdio.h"

namespace OpenApoc
{
namespace
{
// If adding to these lists, don't remove the space at the end...
std::list<UString> developerList = {
    "__________", "pmprog", "redv",       "Andrey", "empty'void", "Flacko", "Flacko the 2nd",
    "istrebitel", "JonnyH", "shellstorm", "skin36", "SupSuper",   "Warboy", "",
};
std::list<UString> traineeList = {
    "__________",
    "FilmBoy84",
    "Jari",
    "",
};
std::list<UString> programmerList = {
    "__________", "Andy51",   "Atrosha",        "Bluddy", "clowds",  "Cristi Popa",
    "Drosan",     "gnegno",   "Kurtsley",       "Leon",   "letwolf", "luiscamara",
    "Stewart",    "TheSnide", "TreacherousOne", "Zigmar", "",
};
std::list<UString> testerList = {
    "__________", "Filmboy84",       "Atrosha",   "Beorn",         "CidDaBird",
    "empty'void", "Flacko the 2nd",  "Hyton",     "Jigoku-Panzer", "laurieblakesdr",
    "MadHaTr",    "makus",           "Quickmind", "RoadhogsButt",  "Sergi4UA",
    "TimboF5",    "Yataka Shimaoka", "",
};
std::list<UString> translatorList = {
    "__________", "Filmboy84", "Confederate Ghost", "IFoldlyGo",       "Kammerer", "makus",
    "PlayMann",   "PPeti66x",  "SAlex_UT",          "Sergi4UA",        "skin36",   "Slitchy",
    "TaoQiBao",   "TimboF5",   "Xcom commander",    "Yataka Shimaoka", "",
};
std::list<UString> githubContribList = {
    "__________",
    "etc...",
    "",
};
} // namespace

CreditsMenu::CreditsMenu() : Stage(), menuform(ui().getForm("creditsmenu")) {}

CreditsMenu::~CreditsMenu() = default;

void CreditsMenu::loadlist()
{
	auto contributorListControl = menuform->findControlTyped<ListBox>("LISTBOX_CREDITS");
	contributorListControl->clear();
	auto font = ui().getFont("smalfont");

	// Developers
	auto devLabel = mksp<Label>("- Developers -", font);
	devLabel->Size = {100, contributorListControl->ItemSize};
	devLabel->TextHAlign = HorizontalAlignment::Centre;
	contributorListControl->addItem(devLabel);

	for (auto &l : developerList)
	{
		auto label = mksp<Label>(l, font);

		label->Size = {216, contributorListControl->ItemSize};
		label->TextHAlign = HorizontalAlignment::Centre;
		contributorListControl->addItem(label);
	}

	// Trainee developers
	auto trainLabel = mksp<Label>("- Trainee Developers -", font);
	trainLabel->Size = {140, contributorListControl->ItemSize};
	trainLabel->TextHAlign = HorizontalAlignment::Centre;
	contributorListControl->addItem(trainLabel);

	for (auto &l : traineeList)
	{
		auto label = mksp<Label>(l, font);

		label->Size = {216, contributorListControl->ItemSize};
		label->TextHAlign = HorizontalAlignment::Centre;
		contributorListControl->addItem(label);
	}

	// Programmers
	auto progLabel = mksp<Label>("- Programmers -", font);
	progLabel->Size = {140, contributorListControl->ItemSize};
	progLabel->TextHAlign = HorizontalAlignment::Centre;
	contributorListControl->addItem(progLabel);

	for (auto &l : programmerList)
	{
		auto label = mksp<Label>(l, font);

		label->Size = {116, contributorListControl->ItemSize};
		label->TextHAlign = HorizontalAlignment::Centre;
		contributorListControl->addItem(label);
	}

	// Testers
	auto testLabel = mksp<Label>("- Testers -", font);
	testLabel->Size = {140, contributorListControl->ItemSize};
	testLabel->TextHAlign = HorizontalAlignment::Centre;
	contributorListControl->addItem(testLabel);

	for (auto &l : testerList)
	{
		auto label = mksp<Label>(l, font);

		label->Size = {216, contributorListControl->ItemSize};
		label->TextHAlign = HorizontalAlignment::Centre;
		contributorListControl->addItem(label);
	}

	// Translators
	auto translateLabel = mksp<Label>("- Translators -", font);
	translateLabel->Size = {140, contributorListControl->ItemSize};
	translateLabel->TextHAlign = HorizontalAlignment::Centre;
	contributorListControl->addItem(translateLabel);

	for (auto &l : translatorList)
	{
		auto label = mksp<Label>(l, font);

		label->Size = {216, contributorListControl->ItemSize};
		label->TextHAlign = HorizontalAlignment::Centre;
		contributorListControl->addItem(label);
	}

	// Github contributors if not already in list
	auto githubLabel = mksp<Label>("- GitHub Contributors -", font);
	githubLabel->Size = {140, contributorListControl->ItemSize};
	githubLabel->TextHAlign = HorizontalAlignment::Centre;
	contributorListControl->addItem(githubLabel);

	for (auto &l : githubContribList)
	{
		auto label = mksp<Label>(l, font);

		label->Size = {216, contributorListControl->ItemSize};
		label->TextHAlign = HorizontalAlignment::Centre;
		contributorListControl->addItem(label);
	}

	// Memorial
	auto memLabel = mksp<Label>("- In Memory of Atrosha -", font);
	memLabel->Size = {140, contributorListControl->ItemSize};
	memLabel->TextHAlign = HorizontalAlignment::Centre;
	contributorListControl->addItem(memLabel);
}

bool CreditsMenu::isTransition() { return false; }

void CreditsMenu::begin() { loadlist(); }

void CreditsMenu::pause() {}

void CreditsMenu::resume() {}

void CreditsMenu::finish() {}

void CreditsMenu::eventOccurred(Event *e)
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

void CreditsMenu::update() { menuform->update(); }

void CreditsMenu::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->render();
}
} // namespace OpenApoc