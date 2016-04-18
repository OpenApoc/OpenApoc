#include "game/ui/base/researchselect.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/base/base.h"
#include "game/state/gamestate.h"
#include "game/state/research.h"

namespace OpenApoc
{

ResearchSelect::ResearchSelect(sp<GameState> state, StateRef<Base> base, sp<Lab> lab)
    : Stage(), form(ui().GetForm("FORM_RESEARCHSELECT")), state(state), base(base), lab(lab)
{
}

ResearchSelect::~ResearchSelect() {}

void ResearchSelect::Begin()
{
	form->FindControlTyped<Label>("TEXT_FUNDS")->SetText(state->getPlayerBalance());
	auto title = form->FindControlTyped<Label>("TEXT_TITLE");
	switch (this->lab->type)
	{
		case ResearchTopic::Type::BioChem:
			title->SetText(tr("Select Biochemistry Project"));
			break;
		case ResearchTopic::Type::Physics:
			title->SetText(tr("Select Biochemistry Project"));
			break;
		default:
			title->SetText(tr("Select Unknown Project"));
			break;
	}
	this->redrawResearchList();

	auto research_list = form->FindControlTyped<ListBox>("LIST");
	research_list->AlwaysEmitSelectionEvents = true;

	research_list->addCallback(FormEventType::ListBoxChangeSelected, [this](Event *e) {
		LogInfo("Research selection change");
		auto list = std::static_pointer_cast<ListBox>(e->Forms().RaisedBy);
		auto topic = list->GetSelectedData<ResearchTopic>();
		if (topic->current_lab)
		{
			LogInfo("Topic already in progress");
			return;
		}
		Lab::setResearch({state.get(), this->lab}, {state.get(), topic});
		this->redrawResearchList();
	});

	research_list->addCallback(FormEventType::ListBoxChangeHover, [this](Event *e) {
		LogInfo("Research selection change");
		auto list = std::static_pointer_cast<ListBox>(e->Forms().RaisedBy);
		auto topic = list->GetHoveredData<ResearchTopic>();
		auto title = this->form->FindControlTyped<Label>("TEXT_SELECTED_TITLE");
		auto description = this->form->FindControlTyped<Label>("TEXT_SELECTED_DESCRIPTION");
		if (topic)
		{
			title->SetText(tr(topic->name));
			description->SetText(tr(topic->description));
		}
		else
		{
			title->SetText("");
			description->SetText("");
		}
	});
}

void ResearchSelect::redrawResearchList()
{
	auto research_list = form->FindControlTyped<ListBox>("LIST");
	research_list->Clear();
	research_list->ItemSize = 20;
	research_list->ItemSpacing = 1;

	for (auto &r : state->research.topics)
	{
		if (r.second->type != this->lab->type)
		{
			continue;
		}
		if (r.second->isComplete())
		{
			continue;
		}
		if (!r.second->dependencies.satisfied(base) && r.second->started == false)
		{
			continue;
		}

		auto control = mksp<Control>();
		control->Size = {544, 20};
		if (this->lab->current_project == r.second)
		{
			control->BackgroundColour = {127, 0, 0, 255};
		}
		else
		{
			control->BackgroundColour = {0, 0, 0, 0};
		}

		auto topic_name = control->createChild<Label>((r.second->name), ui().GetFont("SMALFONT"));
		topic_name->Size = {200, 20};
		topic_name->Location = {6, 0};

		int skill_total = 0;
		if (r.second->current_lab)
		{
			skill_total = r.second->current_lab->getTotalSkill();
		}
		auto skill_total_label = control->createChild<Label>(UString::format("%d", skill_total),
		                                                     ui().GetFont("SMALFONT"));
		skill_total_label->Size = {50, 20};
		skill_total_label->Location = {328, 0};
		skill_total_label->TextHAlign = HorizontalAlignment::Right;

		UString labSize;
		switch (r.second->required_lab_size)
		{
			case ResearchTopic::LabSize::Small:
				labSize = tr("Small");
				break;
			case ResearchTopic::LabSize::Large:
				labSize = tr("Large");
				break;
			default:
				labSize = tr("UNKNOWN");
				break;
		}

		auto lab_size_label = control->createChild<Label>(labSize, ui().GetFont("SMALFONT"));
		lab_size_label->Size = {100, 20};
		lab_size_label->Location = {439, 0};

		control->SetData(r.second);

		research_list->AddItem(control);
	}
}

void ResearchSelect::Pause() {}

void ResearchSelect::Resume() {}

void ResearchSelect::Finish() {}

void ResearchSelect::EventOccurred(Event *e)
{
	form->EventOccured(e);

	if (e->Type() == EVENT_KEY_DOWN)
	{
		if (e->Keyboard().KeyCode == SDLK_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}

	if (e->Type() == EVENT_FORM_INTERACTION)
	{
		if (e->Forms().EventFlag == FormEventType::ButtonClick)
		{
			if (e->Forms().RaisedBy->Name == "BUTTON_OK")
			{
				stageCmd.cmd = StageCmd::Command::POP;
				return;
			}
		}
	}
}

void ResearchSelect::Update(StageCmd *const cmd)
{
	form->Update();
	*cmd = stageCmd;
	stageCmd = StageCmd();
}

void ResearchSelect::Render()
{
	fw().Stage_GetPrevious(this->shared_from_this())->Render();
	// fw().renderer->drawFilledRect({0, 0}, fw().Display_GetSize(), Colour{0, 0, 0, 128});
	form->Render();
}

bool ResearchSelect::IsTransition() { return false; }

}; // namespace OpenApoc
