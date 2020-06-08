#include "game/ui/base/researchselect.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/listbox.h"
#include "forms/ui.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/city/base.h"
#include "game/state/city/research.h"
#include "game/state/gamestate.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/city/vammotype.h"
#include "game/state/rules/city/vehicletype.h"
#include "game/state/rules/city/vequipmenttype.h"
#include "game/state/shared/organisation.h"
#include "game/ui/general/messagebox.h"
#include "library/strings_format.h"

namespace OpenApoc
{

ResearchSelect::ResearchSelect(sp<GameState> state, sp<Lab> lab)
    : Stage(), form(ui().getForm("researchselect")), lab(lab), state(state)
{
	progressImage = fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx", 63));
}

ResearchSelect::~ResearchSelect() = default;

void ResearchSelect::begin()
{
	current_topic = this->lab->current_project;

	form->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
	auto title = form->findControlTyped<Label>("TEXT_TITLE");
	auto progress = form->findControlTyped<Label>("TEXT_PROGRESS");
	auto skill = form->findControlTyped<Label>("TEXT_SKILL");
	switch (this->lab->type)
	{
		case ResearchTopic::Type::BioChem:
			title->setText(tr("Select Biochemistry Project"));
			progress->setText(tr("Progress"));
			skill->setText(tr("Skill"));
			break;
		case ResearchTopic::Type::Physics:
			title->setText(tr("Select Physics Project"));
			progress->setText(tr("Progress"));
			skill->setText(tr("Skill"));
			break;
		case ResearchTopic::Type::Engineering:
			title->setText(tr("Select Manufacturing Project"));
			progress->setText(tr("Unit Cost"));
			skill->setText(tr("Skill Hours"));
			break;
		default:
			title->setText(tr("Select Unknown Project"));
			break;
	}
	this->populateResearchList();
	this->redrawResearchList();

	auto research_list = form->findControlTyped<ListBox>("LIST");

	research_list->addCallback(FormEventType::ListBoxChangeSelected, [this](FormsEvent *e) {
		LogInfo("Research selection change");
		auto list = std::static_pointer_cast<ListBox>(e->forms().RaisedBy);
		auto topic = list->getSelectedData<ResearchTopic>();
		if (topic->current_lab)
		{
			LogInfo("Topic already in progress");
		}
		else
		{
			if (topic->isComplete())
			{
				LogInfo("Topic already complete");
				auto message_box = mksp<MessageBox>(tr("PROJECT COMPLETE"),
				                                    tr("This project is already complete."),
				                                    MessageBox::ButtonOptions::Ok);
				fw().stageQueueCommand({StageCmd::Command::PUSH, message_box});
				// Restore previous selection
				list->setSelected(current_topic ? control_map[current_topic] : nullptr);
				return;
			}
			if (topic->required_lab_size == ResearchTopic::LabSize::Large &&
			    this->lab->size == ResearchTopic::LabSize::Small)
			{
				LogInfo("Topic is large and lab is small");
				auto message_box =
				    mksp<MessageBox>(tr("PROJECT TOO LARGE"),
				                     tr("This project requires an advanced lab or workshop."),
				                     MessageBox::ButtonOptions::Ok);
				fw().stageQueueCommand({StageCmd::Command::PUSH, message_box});
				// Restore previous selection
				list->setSelected(current_topic ? control_map[current_topic] : nullptr);
				return;
			}
			if (this->lab->type == ResearchTopic::Type::Engineering &&
			    topic->cost > state->player->balance)
			{
				LogInfo("Cannot afford to manufacture");
				auto message_box = mksp<MessageBox>(
				    tr("FUNDS EXCEEDED"), tr("Production costs exceed your available funds."),
				    MessageBox::ButtonOptions::Ok);
				fw().stageQueueCommand({StageCmd::Command::PUSH, message_box});
				// Restore previous selection
				list->setSelected(current_topic ? control_map[current_topic] : nullptr);
				return;
			}
		}
		if (current_topic && topic != current_topic)
		{
			control_map[current_topic]->setDirty();
		}
		current_topic = topic;
		this->redrawResearchList();
	});

	research_list->addCallback(FormEventType::ListBoxChangeHover, [this](FormsEvent *e) {
		LogInfo("Research display on hover change");
		auto list = std::static_pointer_cast<ListBox>(e->forms().RaisedBy);
		auto topic = list->getHoveredData<ResearchTopic>();
		auto title = this->form->findControlTyped<Label>("TEXT_SELECTED_TITLE");
		auto description = this->form->findControlTyped<Label>("TEXT_SELECTED_DESCRIPTION");
		auto pic = this->form->findControlTyped<Graphic>("GRAPHIC_SELECTED");
		if (topic)
		{
			title->setText(tr(topic->name));
			description->setText(tr(topic->description));
			if (topic->picture)
			{
				pic->setImage(topic->picture);
			}
			else
			{
				if (topic->type == ResearchTopic::Type::Engineering)
				{
					switch (topic->item_type)
					{
						case ResearchTopic::ItemType::VehicleEquipment:
							pic->setImage(
							    this->state->vehicle_equipment[topic->itemId]->equipscreen_sprite);
							break;
						case ResearchTopic::ItemType::VehicleEquipmentAmmo:
							pic->setImage(nullptr);
							break;
						case ResearchTopic::ItemType::AgentEquipment:
							pic->setImage(
							    this->state->agent_equipment[topic->itemId]->equipscreen_sprite);
							break;
						case ResearchTopic::ItemType::Craft:
							pic->setImage(
							    this->state->vehicle_types[topic->itemId]->equip_icon_small);
							break;
					}
				}
				else
				{
					if (!topic->dependencies.items.agentItemsRequired.empty())
					{
						pic->setImage(topic->dependencies.items.agentItemsRequired.begin()
						                  ->first->equipscreen_sprite);
					}
					else if (!topic->dependencies.items.vehicleItemsRequired.empty())
					{
						pic->setImage(topic->dependencies.items.vehicleItemsRequired.begin()
						                  ->first->equipscreen_sprite);
					}
					else
					{
						pic->setImage(nullptr);
					}
				}
			}
		}
		else
		{
			title->setText("");
			description->setText("");
			pic->setImage(nullptr);
		}
		this->redrawResearchList();
	});

	auto ok_button = form->findControlTyped<GraphicButton>("BUTTON_OK");
	ok_button->addCallback(FormEventType::ButtonClick, [this](FormsEvent *) {
		LogInfo("Research selection OK pressed, applying selection");
		Lab::setResearch({state.get(), this->lab}, {state.get(), current_topic}, state);
	});
}

void ResearchSelect::redrawResearchList()
{
	for (auto &pair : control_map)
	{
		if (current_topic == pair.first)
		{
			pair.second->BackgroundColour = {127, 0, 0, 255};
		}
		else
		{
			pair.second->BackgroundColour = {0, 0, 0, 0};
		}
	}
}

void ResearchSelect::populateResearchList()
{
	auto research_list = form->findControlTyped<ListBox>("LIST");
	research_list->clear();
	research_list->ItemSize = 20;
	research_list->ItemSpacing = 1;

	for (auto &t : state->research.topic_list)
	{
		if (t->type != this->lab->type)
		{
			continue;
		}
		if ((!t->dependencies.satisfied(state->current_base) && t->started == false) || t->hidden)
		{
			continue;
		}
		// FIXME: When we get font coloring, set light blue color for topics too large a size
		bool too_large = (t->required_lab_size == ResearchTopic::LabSize::Large &&
		                  this->lab->size == ResearchTopic::LabSize::Small);
		std::ignore = too_large;

		auto control = mksp<Control>();
		control->Size = {544, 20};

		auto topic_name = control->createChild<Label>((t->name), ui().getFont("smalfont"));
		topic_name->Size = {200, 18};
		topic_name->Location = {6, 2};

		if (this->lab->type == ResearchTopic::Type::Engineering ||
		    ((this->lab->type == ResearchTopic::Type::BioChem ||
		      this->lab->type == ResearchTopic::Type::Physics) &&
		     t->isComplete()))
		{
			UString progress_text;
			if (this->lab->type == ResearchTopic::Type::Engineering)
				progress_text = format("$%d", t->cost);
			else
				progress_text = tr("Complete");
			auto progress_label =
			    control->createChild<Label>(progress_text, ui().getFont("smalfont"));
			progress_label->Size = {100, 18};
			progress_label->Location = {234, 2};
		}
		else
		{
			float projectProgress =
			    clamp((float)t->man_hours_progress / (float)t->man_hours, 0.0f, 1.0f);

			auto progressBg = control->createChild<Graphic>(progressImage);
			progressBg->Size = {102, 6};
			progressBg->Location = {234, 6};
			auto progressBar = control->createChild<Graphic>();
			progressBar->Size = {101, 6};
			progressBar->Location = {234, 6};

			auto progressImage = mksp<RGBImage>(progressBar->Size);
			int redWidth = progressBar->Size.x * projectProgress;
			{
				RGBImageLock l(progressImage);
				for (int y = 0; y < 2; y++)
				{
					for (int x = 0; x < progressBar->Size.x; x++)
					{
						if (x < redWidth)
							l.set({x, y}, {255, 0, 0, 255});
					}
				}
			}
			progressBar->setImage(progressImage);
		}

		int skill_total = 0;
		switch (this->lab->type)
		{
			case ResearchTopic::Type::BioChem:
			case ResearchTopic::Type::Physics:
				if (t->current_lab)
					skill_total = t->current_lab->getTotalSkill();
				break;
			case ResearchTopic::Type::Engineering:
				skill_total = t->man_hours;
				break;
			default:
				break;
		}

		auto skill_total_label =
		    control->createChild<Label>(format("%d", skill_total), ui().getFont("smalfont"));
		skill_total_label->Size = {50, 18};
		skill_total_label->Location = {328, 2};
		skill_total_label->TextHAlign = HorizontalAlignment::Right;

		UString labSize;
		switch (t->required_lab_size)
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

		auto lab_size_label = control->createChild<Label>(labSize, ui().getFont("smalfont"));
		lab_size_label->Size = {100, 18};
		lab_size_label->Location = {439, 2};

		control->setData(t);

		research_list->addItem(control);
		control_map[t] = control;
	}

	if (current_topic)
	{
		research_list->setSelected(control_map[current_topic]);
	}
}

void ResearchSelect::pause() {}

void ResearchSelect::resume() {}

void ResearchSelect::finish() {}

void ResearchSelect::eventOccurred(Event *e)
{
	form->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE || e->keyboard().KeyCode == SDLK_RETURN ||
		    e->keyboard().KeyCode == SDLK_KP_ENTER)
		{
			form->findControl("BUTTON_OK")->click();
			return;
		}
	}

	if (e->type() == EVENT_FORM_INTERACTION)
	{
		if (e->forms().EventFlag == FormEventType::ButtonClick)
		{
			if (e->forms().RaisedBy->Name == "BUTTON_OK")
			{
				fw().stageQueueCommand({StageCmd::Command::POP});
				return;
			}
		}
	}
}

void ResearchSelect::update() { form->update(); }

void ResearchSelect::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	form->render();
}

bool ResearchSelect::isTransition() { return false; }

}; // namespace OpenApoc
