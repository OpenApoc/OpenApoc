#include "game/ui/base/researchscreen.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/listbox.h"
#include "forms/scrollbar.h"
#include "forms/ui.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/font.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/facility.h"
#include "game/state/gamestate.h"
#include "game/ui/base/researchselect.h"
#include "game/ui/components/controlgenerator.h"
#include "library/strings_format.h"

namespace OpenApoc
{

ResearchScreen::ResearchScreen(sp<GameState> state, sp<Facility> selected_lab) : BaseStage(state)
{
	form = ui().getForm("researchscreen");
	arrow = form->findControlTyped<Graphic>("MAGIC_ARROW");
	viewHighlight = BaseGraphics::FacilityHighlight::Labs;
	if (selected_lab)
	{
		state->current_base->selectedLab = viewFacility = selected_lab;
	}
	else
	{
		viewFacility = state->current_base->selectedLab.lock();
	}

	auto uiListSmallLabs = form->findControlTyped<ListBox>("LIST_SMALL_LABS");
	auto uiListLargeLabs = form->findControlTyped<ListBox>("LIST_LARGE_LABS");
	uiListSmallLabs->scroller->setVisible(false);
	uiListLargeLabs->scroller->setVisible(false);

	uiListSmallLabs->addCallback(FormEventType::ListBoxChangeSelected, [this](FormsEvent *e) {
		form->findControlTyped<ListBox>("LIST_LARGE_LABS")->setSelected(nullptr);
		viewFacility =
		    std::static_pointer_cast<ListBox>(e->forms().RaisedBy)->getSelectedData<Facility>();
		setCurrentLabInfo();
	});
	uiListLargeLabs->addCallback(FormEventType::ListBoxChangeSelected, [this](FormsEvent *e) {
		form->findControlTyped<ListBox>("LIST_SMALL_LABS")->setSelected(nullptr);
		viewFacility =
		    std::static_pointer_cast<ListBox>(e->forms().RaisedBy)->getSelectedData<Facility>();
		setCurrentLabInfo();
	});
}

ResearchScreen::~ResearchScreen() = default;

void ResearchScreen::changeBase(sp<Base> newBase)
{
	BaseStage::changeBase(newBase);

	// update lab's lists
	smallLabs.clear();
	largeLabs.clear();
	for (auto &facility : this->state->current_base->facilities)
	{
		if (facility->buildTime == 0 &&
		    (facility->type->capacityType == FacilityType::Capacity::Chemistry ||
		     facility->type->capacityType == FacilityType::Capacity::Physics ||
		     facility->type->capacityType == FacilityType::Capacity::Workshop))
		{
			if (facility->type->size == 1)
			{
				smallLabs.push_back(facility);
			}
			else
			{
				largeLabs.push_back(facility);
			}
		}
	}

	// find selected lab
	viewFacility = state->current_base->selectedLab.lock();
	if (!viewFacility)
	{
		if (!smallLabs.empty())
		{
			viewFacility = smallLabs.front();
		}
		else if (!largeLabs.empty())
		{
			viewFacility = largeLabs.front();
		}
	}

	// populate ui lists
	populateUILabList("LIST_SMALL_LABS", smallLabs);
	populateUILabList("LIST_LARGE_LABS", largeLabs);

	setCurrentLabInfo();
}

void ResearchScreen::begin()
{
	BaseStage::begin();

	if (viewFacility)
	{
		state->current_base->selectedLab = viewFacility;
	}

	auto unassignedAgentList = form->findControlTyped<ListBox>("LIST_UNASSIGNED");
	unassignedAgentList->addCallback(FormEventType::ListBoxChangeSelected, [this](FormsEvent *e) {
		LogWarning("unassigned agent selected");
		if (this->assigned_agent_count >= this->viewFacility->type->capacityAmount)
		{
			LogWarning("no free space in lab");
			return;
		}
		auto list = std::static_pointer_cast<ListBox>(e->forms().RaisedBy);
		auto agent = list->getSelectedData<Agent>();
		if (!agent)
		{
			LogError("No agent in selected data");
			return;
		}
		if (agent->assigned_to_lab)
		{
			LogError("Agent \"%s\" already assigned to a lab?", agent->name);
			return;
		}
		agent->assigned_to_lab = true;
		this->viewFacility->lab->assigned_agents.push_back({state.get(), agent});
		this->setCurrentLabInfo();
	});
	auto removeFn = [this](FormsEvent *e) {
		LogWarning("assigned agent selected");
		auto list = std::static_pointer_cast<ListBox>(e->forms().RaisedBy);
		auto agent = list->getSelectedData<Agent>();
		if (!agent)
		{
			LogError("No agent in selected data");
			return;
		}
		if (!agent->assigned_to_lab)
		{
			LogError("Agent \"%s\" not assigned to a lab?", agent->name);
			return;
		}
		agent->assigned_to_lab = false;
		this->viewFacility->lab->assigned_agents.remove({state.get(), agent});
		this->setCurrentLabInfo();
	};
	auto assignedAgentList = form->findControlTyped<ListBox>("LIST_ASSIGNED");
	assignedAgentList->addCallback(FormEventType::ListBoxChangeSelected, removeFn);
}

void ResearchScreen::pause() {}

void ResearchScreen::resume()
{
	form->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
	updateProgressInfo();
}

void ResearchScreen::finish() {}

void ResearchScreen::eventOccurred(Event *e)
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
	// DEBUG
	if (e->type() == EVENT_KEY_DOWN)
	{
		switch (e->keyboard().KeyCode)
		{
			case SDLK_F10:
			{
				if (current_topic)
				{
					switch (this->viewFacility->lab->current_project->type)
					{
						case ResearchTopic::Type::BioChem:
						case ResearchTopic::Type::Physics:
							current_topic->man_hours_progress = current_topic->man_hours - 1;
							break;
						case ResearchTopic::Type::Engineering:
							viewFacility->lab->manufacture_man_hours_invested =
							    current_topic->man_hours *
							    this->viewFacility->lab->manufacture_goal;
							break;
						default:
							LogError("Unknown lab type");
							break;
					}
					updateProgressInfo();
				}
				return;
			}
		}
	}

	if (e->type() == EVENT_MOUSE_MOVE)
	{
		arrow->setVisible(!(e->mouse().X > arrow->getLocationOnScreen().x));
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
			else if (e->forms().RaisedBy->Name == "BUTTON_RESEARCH_NEWPROJECT")
			{
				if (!this->viewFacility)
				{
					// No lab selected, ignore this click
					return;
				}
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH,
				     mksp<ResearchSelect>(this->state, this->viewFacility->lab)});
				return;
			}
			else if (e->forms().RaisedBy->Name == "BUTTON_RESEARCH_CANCELPROJECT")
			{
				if (!this->viewFacility)
				{
					return;
				}
				Lab::setResearch(this->viewFacility->lab, {state.get(), ""}, state);
				form->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
				this->updateProgressInfo();
				return;
			}
		}
		if (e->forms().EventFlag == FormEventType::ScrollBarChange)
		{
			if (e->forms().RaisedBy->Name == "MANUFACTURE_QUANTITY_SLIDER")
			{
				if (this->viewFacility &&
				    this->viewFacility->lab->type == ResearchTopic::Type::Engineering &&
				    this->viewFacility->lab->current_project)
				{
					auto manufacturing_scrollbar =
					    form->findControlTyped<ScrollBar>("MANUFACTURE_QUANTITY_SLIDER");
					auto manufacturing_quantity = form->findControlTyped<Label>("TEXT_QUANTITY");
					auto quantity = manufacturing_scrollbar->getValue();

					Lab::setQuantity(this->viewFacility->lab, quantity);
					this->updateProgressInfo();
				}
			}
		}
	}
}

void ResearchScreen::update() { form->update(); }

void ResearchScreen::render()
{
	current_topic = viewFacility ? viewFacility->lab->current_project : nullptr;

	refreshView();
	fw().stageGetPrevious(this->shared_from_this())->render();
	form->render();
	BaseStage::render();
}

bool ResearchScreen::isTransition() { return false; }

/**
 * Populating the UI lab list.
 * @listName - listbox's name in the form
 * @listLabs - list of existing labs
 */
void ResearchScreen::populateUILabList(const UString &listName, std::list<sp<Facility>> &listLabs)
{
	auto uiListLabs = form->findControlTyped<ListBox>(listName);
	uiListLabs->clear();

	sp<Control> selectedItem = nullptr;
	for (auto &facility : listLabs)
	{
		auto item = ControlGenerator::createLabControl(state, facility);
		item->setData(facility);

		auto label = std::static_pointer_cast<Label>(item->Controls[0]);
		label->setText(format("%d", facility->lab->assigned_agents.size()));

		uiListLabs->addItem(item);
		if (facility == viewFacility)
		{
			selectedItem = item;
		}
	}
	uiListLabs->setSelected(selectedItem);
}

void ResearchScreen::setCurrentLabInfo()
{
	if (!this->viewFacility)
	{
		auto unassignedAgentList = form->findControlTyped<ListBox>("LIST_UNASSIGNED");
		unassignedAgentList->clear();
		auto assignedAgentList = form->findControlTyped<ListBox>("LIST_ASSIGNED");
		assignedAgentList->clear();
		form->findControlTyped<Label>("TEXT_LAB_TYPE")->setText("");
		auto totalSkillLabel = form->findControlTyped<Label>("TEXT_TOTAL_SKILL");
		totalSkillLabel->setText(format(tr("Total Skill: %d"), 0));
		updateProgressInfo();
		return;
	}
	this->state->current_base->selectedLab = viewFacility;
	this->assigned_agent_count = 0;
	auto labType = this->viewFacility->type->capacityType;
	UString labTypeName = "UNKNOWN";
	AgentType::Role listedAgentType = AgentType::Role::BioChemist;

	if (labType == FacilityType::Capacity::Chemistry)
	{
		labTypeName = tr("Biochemistry");
		listedAgentType = AgentType::Role::BioChemist;
	}
	else if (labType == FacilityType::Capacity::Physics)
	{
		labTypeName = tr("Quantum Physics");
		listedAgentType = AgentType::Role::Physicist;
	}
	else if (labType == FacilityType::Capacity::Workshop)
	{
		labTypeName = tr("Engineering");
		listedAgentType = AgentType::Role::Engineer;
	}
	else
	{
		LogError("Unexpected CapacityType in lab");
	}

	form->findControlTyped<Label>("TEXT_LAB_TYPE")->setText(labTypeName);

	auto agentEntryHeight = ControlGenerator::getFontHeight(*state) * 3;

	auto unassignedAgentList = form->findControlTyped<ListBox>("LIST_UNASSIGNED");
	unassignedAgentList->clear();
	auto assignedAgentList = form->findControlTyped<ListBox>("LIST_ASSIGNED");
	assignedAgentList->clear();
	for (auto &agent : state->agents)
	{
		bool assigned_to_current_lab = false;
		if (agent.second->homeBuilding->base != this->state->current_base)
			continue;

		if (agent.second->type->role != listedAgentType)
			continue;

		if (agent.second->assigned_to_lab)
		{
			for (auto &assigned_agent : this->viewFacility->lab->assigned_agents)
			{
				if (assigned_agent.getSp() == agent.second)
				{
					this->assigned_agent_count++;
					if (this->assigned_agent_count > this->viewFacility->type->capacityAmount)
					{
						LogError("Selected lab has %d assigned agents, but has a capacity of %d",
						         this->assigned_agent_count,
						         this->viewFacility->type->capacityAmount);
					}
					agent.second->lab_assigned = this->viewFacility->lab;
					assigned_to_current_lab = true;
					break;
				}
			}
			if (!assigned_to_current_lab)
				continue;
		}
		if (assigned_to_current_lab)
		{
			assignedAgentList->addItem(ControlGenerator::createLargeAgentControl(
			    *state, agent.second, 160, UnitSkillState::Horizontal));
		}
		else
		{
			unassignedAgentList->addItem(ControlGenerator::createLargeAgentControl(
			    *state, agent.second, unassignedAgentList->Size.x, UnitSkillState::Vertical));
		}
	}
	assignedAgentList->ItemSize = agentEntryHeight;
	unassignedAgentList->ItemSize = agentEntryHeight;

	auto totalSkillLabel = form->findControlTyped<Label>("TEXT_TOTAL_SKILL");
	totalSkillLabel->setText(
	    format(tr("Total Skill: %d"), this->viewFacility->lab->getTotalSkill()));

	// update scientists quantity for selected lab
	auto uiListLabs = form->findControlTyped<ListBox>("LIST_SMALL_LABS");
	auto selectedItem = uiListLabs->getSelectedItem();
	if (!selectedItem)
	{
		uiListLabs = form->findControlTyped<ListBox>("LIST_LARGE_LABS");
		selectedItem = uiListLabs->getSelectedItem();
	}
	if (selectedItem)
	{
		auto label = std::static_pointer_cast<Label>(selectedItem->Controls[0]);
		label->setText(format("%d", this->viewFacility->lab->assigned_agents.size()));
	}

	updateProgressInfo();
}

void ResearchScreen::updateProgressInfo()
{
	if (this->viewFacility && this->viewFacility->lab->current_project)
	{
		auto &topic = this->viewFacility->lab->current_project;
		auto progressBar = form->findControlTyped<Graphic>("GRAPHIC_PROGRESS_BAR");
		auto progressImage = mksp<RGBImage>(progressBar->Size);
		float projectProgress = 0.0f;
		switch (this->viewFacility->lab->current_project->type)
		{
			case ResearchTopic::Type::BioChem:
			case ResearchTopic::Type::Physics:
				projectProgress = clamp(static_cast<float>(topic->man_hours_progress) /
				                            static_cast<float>(topic->man_hours),
				                        0.0f, 1.0f);
				break;
			case ResearchTopic::Type::Engineering:
				projectProgress =
				    clamp(static_cast<float>(
				              this->viewFacility->lab->manufacture_man_hours_invested +
				              topic->man_hours * this->viewFacility->lab->manufacture_done) /
				              static_cast<float>(topic->man_hours *
				                                 this->viewFacility->lab->manufacture_goal),
				          0.0f, 1.0f);
				break;
			default:
				LogError("Unknown lab type");
				break;
		}
		// This creates an image with the size of the PROGRESS_BAR control, then fills
		// up a proportion of it with red pixels (starting from the left) corresponding
		// to the progress of the project.
		int redWidth = progressBar->Size.x * projectProgress;
		{
			RGBImageLock l(progressImage);
			for (int y = 0; y < progressBar->Size.y; y++)
			{
				for (int x = 0; x < redWidth; x++)
				{
					l.set({x, y}, {255, 0, 0, 255});
				}
			}
		}
		progressBar->setImage(progressImage);
		auto topicTitle = form->findControlTyped<Label>("TEXT_CURRENT_PROJECT");
		topicTitle->setText(tr(topic->name));
		auto completionPercent = form->findControlTyped<Label>("TEXT_PROJECT_COMPLETION");
		auto completionText = format(tr("%d%%"), static_cast<int>(projectProgress * 100.0f));
		completionPercent->setText(completionText);
	}
	else
	{
		auto progressBar = form->findControlTyped<Graphic>("GRAPHIC_PROGRESS_BAR");
		progressBar->setImage(nullptr);
		auto topicTitle = form->findControlTyped<Label>("TEXT_CURRENT_PROJECT");
		topicTitle->setText(tr("No Project"));
		auto completionPercent = form->findControlTyped<Label>("TEXT_PROJECT_COMPLETION");
		completionPercent->setText("");
	}
	auto manufacture_bg = form->findControlTyped<Graphic>("MANUFACTURE_BG");

	auto manufacturing_scrollbar = form->findControlTyped<ScrollBar>("MANUFACTURE_QUANTITY_SLIDER");
	auto manufacturing_scroll_left =
	    form->findControlTyped<GraphicButton>("MANUFACTURE_QUANTITY_DOWN");
	auto manufacturing_scroll_right =
	    form->findControlTyped<GraphicButton>("MANUFACTURE_QUANTITY_UP");
	auto manufacturing_quantity = form->findControlTyped<Label>("TEXT_QUANTITY");
	auto manufacturing_ntomake = form->findControlTyped<Label>("TEXT_NUMBER_TO_MAKE");
	if (this->viewFacility && this->viewFacility->lab->current_project &&
	    this->viewFacility->lab->current_project->type == ResearchTopic::Type::Engineering)
	{
		manufacture_bg->setVisible(true);
		manufacturing_ntomake->setVisible(true);
		manufacturing_quantity->setVisible(true);
		manufacturing_scrollbar->setVisible(true);
		manufacturing_scroll_left->setVisible(true);
		manufacturing_scroll_right->setVisible(true);
		manufacturing_scrollbar->setValue(this->viewFacility->lab->getQuantity());
		manufacturing_quantity->setText(format("%d", this->viewFacility->lab->getQuantity()));
	}
	else
	{
		manufacture_bg->setVisible(false);
		manufacturing_ntomake->setVisible(false);
		manufacturing_quantity->setVisible(false);
		manufacturing_scrollbar->setVisible(false);
		manufacturing_scroll_left->setVisible(false);
		manufacturing_scroll_right->setVisible(false);
	}
}

}; // namespace OpenApoc
