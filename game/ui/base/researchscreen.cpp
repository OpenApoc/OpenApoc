#include "game/ui/base/researchscreen.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/base/facility.h"
#include "game/state/gamestate.h"
#include "game/ui/base/researchselect.h"

namespace OpenApoc
{

ResearchScreen::ResearchScreen(sp<GameState> state, sp<Facility> selected_lab)
    : BaseStage(state), selected_lab(selected_lab)
{
	form = ui().getForm("FORM_RESEARCHSCREEN");
	viewHighlight = BaseGraphics::FacilityHighlight::Labs;
	viewFacility = selected_lab;
	arrow = form->findControlTyped<Graphic>("MAGIC_ARROW");
}

ResearchScreen::~ResearchScreen() = default;

void ResearchScreen::changeBase(sp<Base> newBase)
{
	BaseStage::changeBase(newBase);

	// FIXME: Should only reset if selected_lab doesn't belong to current base
	this->selected_lab = nullptr;
	this->labs.clear();
	for (auto &facility : this->state->current_base->facilities)
	{
		if (facility->buildTime == 0 &&
		    (facility->type->capacityType == FacilityType::Capacity::Chemistry ||
		     facility->type->capacityType == FacilityType::Capacity::Physics ||
		     facility->type->capacityType == FacilityType::Capacity::Workshop))
		{
			this->labs.push_back(facility);
			if (!this->selected_lab)
			{
				this->selected_lab = facility;
				this->viewFacility = this->selected_lab;
			}
		}
	}

	auto labList = form->findControlTyped<ListBox>("LIST_LABS");
	labList->clear();
	for (auto &facility : this->labs)
	{
		auto graphic = mksp<Graphic>(facility->type->sprite);
		graphic->AutoSize = true;
		graphic->setData(facility);
		labList->addItem(graphic);
	}

	setCurrentLabInfo();
}

void ResearchScreen::begin()
{
	auto img = mksp<RGBImage>(Vec2<int>{1, 2});
	{
		RGBImageLock l(img);
		l.set({0, 0}, Colour{255, 255, 219});
		l.set({0, 1}, Colour{215, 0, 0});
	}
	this->healthImage = img;

	BaseStage::begin();

	auto unassignedAgentList = form->findControlTyped<ListBox>("LIST_UNASSIGNED");
	unassignedAgentList->addCallback(FormEventType::ListBoxChangeSelected, [this](Event *e) {
		LogWarning("unassigned agent selected");
		if (this->assigned_agent_count >= this->selected_lab->type->capacityAmount)
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
			LogError("Agent \"%s\" already assigned to a lab?", agent->name.cStr());
			return;
		}
		agent->assigned_to_lab = true;
		this->selected_lab->lab->assigned_agents.push_back({state.get(), agent});
		this->setCurrentLabInfo();
	});
	auto removeFn = [this](Event *e) {
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
			LogError("Agent \"%s\" not assigned to a lab?", agent->name.cStr());
			return;
		}
		agent->assigned_to_lab = false;
		this->selected_lab->lab->assigned_agents.remove({state.get(), agent});
		this->setCurrentLabInfo();
	};
	auto assignedAgentList = form->findControlTyped<ListBox>("LIST_ASSIGNED");
	assignedAgentList->addCallback(FormEventType::ListBoxChangeSelected, removeFn);

	// Set the listboxes to always emit events, otherwise the first entry is considered 'selected'
	// to clicking on it won't get a callback
	assignedAgentList->AlwaysEmitSelectionEvents = true;
	unassignedAgentList->AlwaysEmitSelectionEvents = true;
}

void ResearchScreen::pause() {}

void ResearchScreen::resume()
{
	form->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
}

void ResearchScreen::finish() {}

void ResearchScreen::eventOccurred(Event *e)
{
	form->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE)
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
			return;
		}
	}

	if (e->type() == EVENT_MOUSE_MOVE)
	{
		arrow->Visible = (e->mouse().X > form->Location.x + arrow->Location.x);
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
				if (!this->selected_lab)
				{
					// No lab selected, ignore this click
					return;
				}
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH,
				     mksp<ResearchSelect>(this->state, this->selected_lab->lab)});
				return;
			}
			else if (e->forms().RaisedBy->Name == "BUTTON_RESEARCH_CANCELPROJECT")
			{
				if (!this->selected_lab)
				{
					return;
				}
				Lab::setResearch(this->selected_lab->lab, {state.get(), ""}, state);
				form->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
			}
		}
		if (e->forms().EventFlag == FormEventType::ScrollBarChange)
		{
			if (e->forms().RaisedBy->Name == "MANUFACTURE_QUANTITY_SLIDER")
			{
				if (this->selected_lab &&
				    this->selected_lab->lab->type == ResearchTopic::Type::Engineering &&
				    this->selected_lab->lab->current_project)
				{
					auto manufacturing_scrollbar =
					    form->findControlTyped<ScrollBar>("MANUFACTURE_QUANTITY_SLIDER");
					auto manufacturing_quantity = form->findControlTyped<Label>("TEXT_QUANTITY");
					auto quantity = manufacturing_scrollbar->getValue();

					Lab::setQuantity(this->selected_lab->lab, quantity);
					this->updateProgressInfo();
				}
			}
		}
	}
}

void ResearchScreen::update() { form->update(); }

void ResearchScreen::render()
{
	auto labList = form->findControlTyped<ListBox>("LIST_LABS");
	if (this->selected_lab != labList->getSelectedData<Facility>() ||
	    (this->selected_lab && this->selected_lab->lab->current_project != this->current_topic))
	{
		this->selected_lab = labList->getSelectedData<Facility>();
		this->viewFacility = this->selected_lab;
		this->current_topic = this->selected_lab->lab->current_project;
		this->setCurrentLabInfo();
		this->refreshView();
	}
	fw().stageGetPrevious(this->shared_from_this())->render();
	// fw().renderer->drawFilledRect({0, 0}, fw().displayGetSize(), Colour{0, 0, 0, 128});
	form->render();
	BaseStage::render();
}

bool ResearchScreen::isTransition() { return false; }

void ResearchScreen::setCurrentLabInfo()
{
	if (!this->selected_lab)
	{
		auto unassignedAgentList = form->findControlTyped<ListBox>("LIST_UNASSIGNED");
		unassignedAgentList->clear();
		auto assignedAgentList = form->findControlTyped<ListBox>("LIST_ASSIGNED");
		assignedAgentList->clear();
		form->findControlTyped<Label>("TEXT_LAB_TYPE")->setText("");
		auto totalSkillLabel = form->findControlTyped<Label>("TEXT_TOTAL_SKILL");
		totalSkillLabel->setText(UString::format(tr("Total Skill: %d"), 0));
		return;
	}
	this->assigned_agent_count = 0;
	auto labType = this->selected_lab->type->capacityType;
	UString labTypeName = "UNKNOWN";
	Agent::Type listedAgentType = Agent::Type::BioChemist;

	if (labType == FacilityType::Capacity::Chemistry)
	{
		labTypeName = tr("Biochemistry");
		listedAgentType = Agent::Type::BioChemist;
	}
	else if (labType == FacilityType::Capacity::Physics)
	{
		labTypeName = tr("Quantum Physics");
		listedAgentType = Agent::Type::Physicist;
	}
	else if (labType == FacilityType::Capacity::Workshop)
	{
		labTypeName = tr("Engineering");
		listedAgentType = Agent::Type::Engineer;
	}
	else
	{
		LogError("Unexpected CapacityType in lab");
	}

	form->findControlTyped<Label>("TEXT_LAB_TYPE")->setText(labTypeName);

	auto font = ui().getFont("SMALFONT");
	auto agentEntryHeight = font->getFontHeight() * 3;

	auto unassignedAgentList = form->findControlTyped<ListBox>("LIST_UNASSIGNED");
	unassignedAgentList->clear();
	auto assignedAgentList = form->findControlTyped<ListBox>("LIST_ASSIGNED");
	assignedAgentList->clear();
	for (auto &agent : state->agents)
	{
		bool assigned_to_current_lab = false;
		if (agent.second->home_base != this->state->current_base)
			continue;

		if (agent.second->type != listedAgentType)
			continue;

		if (agent.second->assigned_to_lab)
		{
			for (auto &assigned_agent : this->selected_lab->lab->assigned_agents)
			{
				if (assigned_agent.getSp() == agent.second)
				{
					this->assigned_agent_count++;
					if (this->assigned_agent_count > this->selected_lab->type->capacityAmount)
					{
						LogError("Selected lab has %d assigned agents, but has a capacity of %d",
						         this->assigned_agent_count,
						         this->selected_lab->type->capacityAmount);
					}
					assigned_to_current_lab = true;
					break;
				}
			}
			if (!assigned_to_current_lab)
				continue;
		}
		auto agentControl =
		    this->createAgentControl({130, agentEntryHeight}, {state.get(), agent.second});

		if (assigned_to_current_lab)
		{
			assignedAgentList->addItem(agentControl);
		}
		else
		{
			unassignedAgentList->addItem(agentControl);
		}
	}
	assignedAgentList->ItemSize = agentEntryHeight;
	unassignedAgentList->ItemSize = agentEntryHeight;

	auto totalSkillLabel = form->findControlTyped<Label>("TEXT_TOTAL_SKILL");
	totalSkillLabel->setText(
	    UString::format(tr("Total Skill: %d"), this->selected_lab->lab->getTotalSkill()));

	updateProgressInfo();
}

void ResearchScreen::updateProgressInfo()
{
	if (this->selected_lab->lab->current_project)
	{
		auto &topic = this->selected_lab->lab->current_project;
		auto progressBar = form->findControlTyped<Graphic>("GRAPHIC_PROGRESS_BAR");
		auto progressImage = mksp<RGBImage>(progressBar->Size);
		float projectProgress = 0.0f;
		switch (this->selected_lab->lab->current_project->type)
		{
			case ResearchTopic::Type::BioChem:
			case ResearchTopic::Type::Physics:
				projectProgress =
				    clamp((float)topic->man_hours_progress / (float)topic->man_hours, 0.0f, 1.0f);
				break;
			case ResearchTopic::Type::Engineering:
				projectProgress =
				    clamp((float)(this->selected_lab->lab->manufacture_man_hours_invested +
				                  topic->man_hours * this->selected_lab->lab->manufacture_done) /
				              (float)(topic->man_hours * this->selected_lab->lab->manufacture_goal),
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
		auto completionText = UString::format(tr("%d%%"), (int)(projectProgress * 100.0f));
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
	auto manufacturing_scrollbar = form->findControlTyped<ScrollBar>("MANUFACTURE_QUANTITY_SLIDER");
	auto manufacturing_scroll_left =
	    form->findControlTyped<GraphicButton>("MANUFACTURE_QUANTITY_DOWN");
	auto manufacturing_scroll_right =
	    form->findControlTyped<GraphicButton>("MANUFACTURE_QUANTITY_UP");
	auto manufacturing_quantity = form->findControlTyped<Label>("TEXT_QUANTITY");
	auto manufacturing_ntomake = form->findControlTyped<Label>("TEXT_NUMBER_TO_MAKE");
	if (this->selected_lab->lab->current_project &&
	    this->selected_lab->lab->current_project->type == ResearchTopic::Type::Engineering)
	{
		manufacturing_ntomake->Visible = true;
		manufacturing_quantity->Visible = true;
		manufacturing_scrollbar->Visible = true;
		manufacturing_scroll_left->Visible = true;
		manufacturing_scroll_right->Visible = true;
		manufacturing_scrollbar->setValue(this->selected_lab->lab->getQuantity());
		manufacturing_quantity->setText(
		    UString::format(tr("%d"), this->selected_lab->lab->getQuantity()));
	}
	else
	{
		manufacturing_ntomake->Visible = false;
		manufacturing_quantity->Visible = false;
		manufacturing_scrollbar->Visible = false;
		manufacturing_scroll_left->Visible = false;
		manufacturing_scroll_right->Visible = false;
	}
}

// FIXME: Put this in the rules somewhere?
// FIXME: This could be shared with the citview ICON_RESOURCES?
static const UString agentFramePath =
    "PCK:xcom3/UFODATA/VS_ICON.PCK:xcom3/UFODATA/VS_ICON.TAB:37:xcom3/UFODATA/PAL_01.DAT";

sp<Control> ResearchScreen::createAgentControl(Vec2<int> size, StateRef<Agent> agent)
{
	auto baseControl = mksp<Control>();
	baseControl->setData(agent.getSp());
	baseControl->Name = "AGENT_PORTRAIT";
	baseControl->Size = size;

	auto frameGraphic = baseControl->createChild<Graphic>(fw().data->loadImage(agentFramePath));
	frameGraphic->AutoSize = true;
	frameGraphic->Location = {5, 5};
	auto photoGraphic = frameGraphic->createChild<Graphic>(agent->portrait.icon);
	photoGraphic->AutoSize = true;
	photoGraphic->Location = {1, 1};

	auto healthGraphic = frameGraphic->createChild<Graphic>(this->healthImage);
	// FIXME: Put these somewhere slightly less magic?
	// FIXME: Also shared between CityView?
	Vec2<int> healthBarOffset = {27, 2};
	Vec2<int> healthBarSize = {3, 20};
	// This is a bit annoying as the health bar starts at the bottom, but the coord origin is
	// top-left, so fix that up a bit
	float healthProportion =
	    (float)agent->current_stats.health / (float)agent->initial_stats.health;
	int healthBarHeight = (float)healthBarSize.y * healthProportion;
	healthBarOffset.y = healthBarOffset.y + (healthBarSize.y - healthBarHeight);
	healthBarSize.y = healthBarHeight;
	healthGraphic->Location = healthBarOffset;
	healthGraphic->Size = healthBarSize;
	healthGraphic->ImagePosition = FillMethod::Stretch;

	auto font = ui().getFont("SMALFONT");

	auto nameLabel = baseControl->createChild<Label>(agent->name, font);
	nameLabel->Location = {40, 0};
	nameLabel->Size = {100, font->getFontHeight() * 2};

	int skill = 0;
	if (agent->type == Agent::Type::Physicist)
	{
		skill = agent->current_stats.physics_skill;
	}
	else if (agent->type == Agent::Type::BioChemist)
	{
		skill = agent->current_stats.biochem_skill;
	}
	else if (agent->type == Agent::Type::Engineer)
	{
		skill = agent->current_stats.engineering_skill;
	}
	else
	{
		LogError("Trying to show non-scientist agent %s (%s)", agent.id.cStr(), agent->name.cStr());
	}

	auto skillLabel = baseControl->createChild<Label>(UString::format(tr("Skill %s"), skill), font);
	skillLabel->Size = {100, font->getFontHeight()};
	skillLabel->Location = {40, font->getFontHeight() * 2};

	return baseControl;
}

}; // namespace OpenApoc
