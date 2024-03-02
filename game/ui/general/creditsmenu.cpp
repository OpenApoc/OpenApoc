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
// Contributor lists, if adding to these keep the blank entry at the end!
std::list<UString> developerList = {
    u8"PmProg - Marq Watkin",
    u8"redv",
    u8"SupSuper - Daniel Albano",
    u8"JonnyH - Jonathan Hamilton",
    u8"Istrebitel",
    u8"Skin36",
    u8"FilmBoy84 - Jacob Deuchar",
    u8"Makus / Makus82 Shellstorm - Ivan Shibanov",
    u8"FranciscoDA / Flacko empty`void / Empty Void Jarskih - Jari Hanski",
    u8"Kurtsley - Brian Beard",
    u8"Atrosha - Panasenko Vasiliy Sergeevich",
    u8"",
};
std::list<UString> otherGitList = {
    u8"Luis Camara",
    u8"SuperUserCode",
    u8"KGD192",
    u8"sfalexrog - Alexey Rogachevskiy",
    u8"zigmar / zigmar - ems - Pavel Antokolsky",
    u8"ShadowDancer",
    u8"TreacherousOne - Kirill Mishustin",
    u8"steveschnepp - Steve Schnepp",
    u8"StewartM",
    u8"RedRobin84 - Martin Cervenka",
    u8"SiemensSchuckert",
    u8"sparkstar",
    u8"dl471",
    u8"5thAvenue",
    u8"gnegno84 - Marcello Santambrogio",
    u8"killermosi - Silviu Ghita",
    u8"BabyWolf - Volkov Semjon",
    u8"AMDmi3 - Dmitry Marakasov",
    u8"AndreyCreator",
    u8"Blackwolf - Kuzoku",
    u8"andersand",
    u8"Andy51 - Andrey Isakov",
    u8"Hambones82",
    u8"h3xx - Dan Church",
    u8"ashenomo",
    u8"kaja47",
    u8"solbu - Johnny Solbu",
    u8"pkubaj",
    u8"DoxaLogosGit - Jay Atkinson",
    u8"Sonicelo - Gregor Sušanj",
    u8"kkmic",
    u8"Przemyslaw",
    u8"Onak",
    u8"Roger",
    u8"",
};
std::list<UString> testingList = {
    u8"FilmBoy84 - Jacob Deuchar",
    u8"Quickmind / Quickmind01",
    u8"HeadGrowsBack",
    u8"RoadHogsButt",
    u8"Yataka Shimaoka",
    u8"EmperorLol - Laurie Blake",
    u8"Jigoku - Panzer - Dean Martin",
    u8"",
};
std::list<UString> moddingList = {
    u8"JonnyH - Jonathan Hamilton",
    u8"FilmBoy84 - Jacob Deuchar",
    u8"Istrebitel",
    u8"Voiddweller",
    u8"Skin36",
    u8"",
};
std::list<UString> reversingList = {
    u8"Skin36",
    u8"",
};
std::list<UString> translationList = {
    u8"5thAvenue", u8"Blackwolf - Kuzoku", u8"Skin36", u8"SolariusScorch", u8"Xracer", "",
};
} // namespace

CreditsMenu::CreditsMenu() : Stage(), menuform(ui().getForm("creditsmenu")) {}

CreditsMenu::~CreditsMenu() = default;

void CreditsMenu::loadlist()
{
	auto contributorListControl = menuform->findControlTyped<ListBox>("LISTBOX_CREDITS");
	contributorListControl->clear();
	auto font = ui().getFont("smalfont");

	// Need this because new line doesn't work at the beginning of a label
	auto spacer = mksp<Label>("", font);
	spacer->Size = {100, contributorListControl->ItemSize};
	contributorListControl->addItem(spacer);

	// Lead Team, Developers and Programming
	auto devLabel = mksp<Label>("- Lead Team, Developers and Programming -\n", font);
	devLabel->Size = {100, contributorListControl->ItemSize * 2};
	devLabel->TextHAlign = HorizontalAlignment::Centre;
	contributorListControl->addItem(devLabel);

	for (auto &l : developerList)
	{
		auto label = mksp<Label>(l, font);

		label->Size = {216, contributorListControl->ItemSize};
		label->TextHAlign = HorizontalAlignment::Centre;
		contributorListControl->addItem(label);
	}

	// Other GitHub contributors
	auto trainLabel = mksp<Label>("- Other GitHub Contributors -\n", font);
	trainLabel->Size = {140, contributorListControl->ItemSize * 2};
	trainLabel->TextHAlign = HorizontalAlignment::Centre;
	contributorListControl->addItem(trainLabel);

	for (auto &l : otherGitList)
	{
		auto label = mksp<Label>(l, font);

		label->Size = {216, contributorListControl->ItemSize};
		label->TextHAlign = HorizontalAlignment::Centre;
		contributorListControl->addItem(label);
	}

	// Testers
	auto progLabel = mksp<Label>("- Testing -\n", font);
	progLabel->Size = {140, contributorListControl->ItemSize * 2};
	progLabel->TextHAlign = HorizontalAlignment::Centre;
	contributorListControl->addItem(progLabel);

	for (auto &l : testingList)
	{
		auto label = mksp<Label>(l, font);

		label->Size = {116, contributorListControl->ItemSize};
		label->TextHAlign = HorizontalAlignment::Centre;
		contributorListControl->addItem(label);
	}

	// Modding Structure
	auto testLabel = mksp<Label>("- Modding Structure -\n", font);
	testLabel->Size = {140, contributorListControl->ItemSize * 2};
	testLabel->TextHAlign = HorizontalAlignment::Centre;
	contributorListControl->addItem(testLabel);

	for (auto &l : moddingList)
	{
		auto label = mksp<Label>(l, font);

		label->Size = {216, contributorListControl->ItemSize};
		label->TextHAlign = HorizontalAlignment::Centre;
		contributorListControl->addItem(label);
	}

	// Reversing and Research
	auto translateLabel = mksp<Label>("- Reversing and Research -\n", font);
	translateLabel->Size = {140, contributorListControl->ItemSize * 2};
	translateLabel->TextHAlign = HorizontalAlignment::Centre;
	contributorListControl->addItem(translateLabel);

	for (auto &l : reversingList)
	{
		auto label = mksp<Label>(l, font);

		label->Size = {216, contributorListControl->ItemSize};
		label->TextHAlign = HorizontalAlignment::Centre;
		contributorListControl->addItem(label);
	}

	// Translation
	auto githubLabel = mksp<Label>("- Translation -\n", font);
	githubLabel->Size = {140, contributorListControl->ItemSize * 2};
	githubLabel->TextHAlign = HorizontalAlignment::Centre;
	contributorListControl->addItem(githubLabel);

	for (auto &l : translationList)
	{
		auto label = mksp<Label>(l, font);

		label->Size = {216, contributorListControl->ItemSize};
		label->TextHAlign = HorizontalAlignment::Centre;
		contributorListControl->addItem(label);
	}

	// Memorial
	auto memLabel = mksp<Label>(
	    "\nIn Memory of Panasenko Vasiliy Sergeevich / \"Atrosha\" (1980-2021)\n", font);
	memLabel->Size = {140, contributorListControl->ItemSize * 3};
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
