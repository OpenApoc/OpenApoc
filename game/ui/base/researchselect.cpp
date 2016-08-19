#include "game/ui/base/researchselect.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/base/base.h"
#include "game/state/gamestate.h"
#include "game/state/research.h"
#include "game/ui/general/messagebox.h"


namespace OpenApoc
{

ResearchSelect::ResearchSelect(sp<GameState> state, sp<Lab> lab)
    : Stage(), form(ui().GetForm("FORM_RESEARCHSELECT")), lab(lab), state(state)
{
}

ResearchSelect::~ResearchSelect() {}

void ResearchSelect::Begin()
{
	current_topic = this->lab->current_project;

	form->FindControlTyped<Label>("TEXT_FUNDS")->SetText(state->getPlayerBalance());
	auto title = form->FindControlTyped<Label>("TEXT_TITLE");
	auto progress = form->FindControlTyped<Label>("TEXT_PROGRESS");
	auto skill = form->FindControlTyped<Label>("TEXT_SKILL");
	switch (this->lab->type)
	{
		case ResearchTopic::Type::BioChem:
			title->SetText(tr("Select Biochemistry Project"));
			progress->SetText(tr("Progress"));
			skill->SetText(tr("Skill"));
			break;
		case ResearchTopic::Type::Physics:
			title->SetText(tr("Select Physics Project"));
			progress->SetText(tr("Progress"));
			skill->SetText(tr("Skill"));
			break;
		case ResearchTopic::Type::Engineering:
			title->SetText(tr("Select Manufacturing Project"));
			progress->SetText(tr("Unit Cost"));
			skill->SetText(tr("Skill Hours"));
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
		if (topic->isComplete())
		{
			LogInfo("Topic already complete");
			auto message_box = mksp<MessageBox>(
				tr("PROJECT COMPLETE"),
				tr("This project is already complete."),
				MessageBox::ButtonOptions::Ok
				);
			stageCmd.cmd = StageCmd::Command::PUSH;
			stageCmd.nextStage = message_box;
			return;
		}
		if (this->lab->type == ResearchTopic::Type::Engineering && topic->cost > state->player->balance)
		{
			LogInfo("Cannot afford to manufacture");
			auto message_box = mksp<MessageBox>(
				tr("FUNDS EXCEEDED"),
				tr("Production costs exceed your available funds."),
				MessageBox::ButtonOptions::Ok
				);
			stageCmd.cmd = StageCmd::Command::PUSH;
			stageCmd.nextStage = message_box;
			return;
		}
		current_topic = topic;
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

	auto ok_button= form->FindControlTyped<GraphicButton>("BUTTON_OK");
	ok_button->addCallback(FormEventType::ButtonClick, [this](Event *e) {
		LogInfo("Research selection OK pressed, applying selection");
		Lab::setResearch({ state.get(), this->lab }, { state.get(), current_topic }, state);
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
		if (!r.second->dependencies.satisfied(state->current_base) && r.second->started == false)
		{
			continue;
		}

		auto control = mksp<Control>();
		control->Size = { 544, 20 };
		if (current_topic == r.second)
		{
			control->BackgroundColour = { 127, 0, 0, 255 };
		}
		else
		{
			control->BackgroundColour = { 0, 0, 0, 0 };
		}

		auto topic_name = control->createChild<Label>((r.second->name), ui().GetFont("SMALFONT"));
		topic_name->Size = { 200, 20 };
		topic_name->Location = { 6, 0 };

		if (this->lab->type == ResearchTopic::Type::Engineering || ((this->lab->type == ResearchTopic::Type::BioChem || this->lab->type == ResearchTopic::Type::Physics) && r.second->isComplete()))
		{
			UString progress_text;
			if (this->lab->type == ResearchTopic::Type::Engineering)
				progress_text = UString::format("$%d", r.second->cost);
			else
				progress_text = tr("Complete");
			auto progress_label = control->createChild<Label>(progress_text, ui().GetFont("SMALFONT"));
			progress_label->Size = { 100, 20 };
			progress_label->Location = { 234, 0 };
		}
		else
		{
			float projectProgress = clamp((float)r.second->man_hours_progress / (float)r.second->man_hours, 0.0f, 1.0f);

			auto progressBar = control->createChild<Graphic>();
			progressBar->Size = { 101, 6 };
			progressBar->Location = { 234, 3 };
			
			auto progressImage = mksp<RGBImage>(progressBar->Size);
			int redWidth = progressBar->Size.x * projectProgress;
			{
				// FIXME: For some reason, there's no border here like in the research sceen, so we have to make one manually, probably there's a better way
				RGBImageLock l(progressImage);
				for (int y = 0; y < 2; y++)
				{
					for (int x = 0; x < progressBar->Size.x; x++)
					{
						if (x < redWidth)
							l.set({ x, y }, { 255, 0, 0, 255 });
						else
							l.set({ x, y }, { 77, 77, 77, 255 });
					}
					
				}
				l.set({ 0, 2 }, { 77, 77, 77, 255 });
				l.set({ progressBar->Size.x - 1, 2 }, { 77, 77, 77, 255 });
				l.set({ 0, 3 }, { 130, 130, 130, 255 });
				l.set({ progressBar->Size.x - 1, 3 }, { 130, 130, 130, 255 });
				l.set({ 0, 4 }, { 140, 140, 140, 255 });
				l.set({ progressBar->Size.x - 1, 4 }, { 140, 140, 140, 255 });
				for (int x = 0; x < progressBar->Size.x; x++)
				{
					l.set({ x, 5 }, { 205, 205, 205, 255 });
				}
			}
			progressBar->SetImage(progressImage);
		}

		
		int skill_total = 0;
		switch (this->lab->type)
		{
		case ResearchTopic::Type::BioChem:
		case ResearchTopic::Type::Physics:
			if (r.second->current_lab)
				skill_total = r.second->current_lab->getTotalSkill();
			break;
		case ResearchTopic::Type::Engineering:
			skill_total = r.second->man_hours;
			break;
		default:
			break;
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
