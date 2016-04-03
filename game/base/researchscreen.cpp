#include "game/base/researchscreen.h"
#include "framework/framework.h"
#include "game/base/facility.h"
#include "game/resources/gamecore.h"

namespace OpenApoc
{

ResearchScreen::ResearchScreen(sp<GameState> state, StateRef<Base> base, sp<Facility> selected_lab)
    : Stage(), form(fw().gamecore->GetForm("FORM_RESEARCHSCREEN")), base(base),
      selected_lab(selected_lab), state(state)
{
}

ResearchScreen::~ResearchScreen() {}

void ResearchScreen::Begin()
{
	form->FindControlTyped<Label>("TEXT_FUNDS")->SetText(state->getPlayerBalance());

	for (auto &facility : this->base->facilities)
	{
		if (facility->type->capacityType == FacilityType::Capacity::Chemistry ||
		    facility->type->capacityType == FacilityType::Capacity::Physics ||
		    facility->type->capacityType == FacilityType::Capacity::Workshop)
		{
			this->labs.push_back(facility);
			if (!this->selected_lab)
				this->selected_lab = facility;
		}
	}

	auto labList = form->FindControlTyped<ListBox>("LIST_LABS");

	for (auto &facility : this->labs)
	{
		auto graphic = mksp<Graphic>(facility->type->sprite);
		graphic->AutoSize = true;
		graphic->SetData(facility);
		labList->AddItem(graphic);
	}
	auto img = mksp<RGBImage>(Vec2<int>{1, 2});
	{
		RGBImageLock l(img);
		l.set({0, 0}, Colour{255, 255, 219});
		l.set({0, 1}, Colour{215, 0, 0});
	}
	this->healthImage = img;

	setCurrentLabInfo();
}

void ResearchScreen::Pause() {}

void ResearchScreen::Resume() {}

void ResearchScreen::Finish() {}

void ResearchScreen::EventOccurred(Event *e)
{
	form->EventOccured(e);
	fw().gamecore->MouseCursor->EventOccured(e);

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

void ResearchScreen::Update(StageCmd *const cmd)
{
	form->Update();
	*cmd = stageCmd;
	stageCmd = StageCmd();
}

void ResearchScreen::Render()
{
	auto labList = form->FindControlTyped<ListBox>("LIST_LABS");
	if (this->selected_lab != labList->GetSelectedData<Facility>())
	{
		this->selected_lab = labList->GetSelectedData<Facility>();
		this->setCurrentLabInfo();
	}
	fw().Stage_GetPrevious(this->shared_from_this())->Render();
	// fw().renderer->drawFilledRect({0, 0}, fw().Display_GetSize(), Colour{0, 0, 0, 128});
	form->Render();
	fw().gamecore->MouseCursor->Render();
}

bool ResearchScreen::IsTransition() { return false; }

void ResearchScreen::setCurrentLabInfo()
{
	int totalLabSkill = 0;
	auto labType = this->selected_lab->type->capacityType;
	UString labTypeName = "UNKNOWN";
	Agent::Type listedAgentType;

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

	form->FindControlTyped<Label>("TEXT_LAB_TYPE")->SetText(labTypeName);

	auto unassignedAgentList = form->FindControlTyped<ListBox>("LIST_UNASSIGNED");
	unassignedAgentList->Clear();
	auto assignedAgentListCol1 = form->FindControlTyped<ListBox>("LIST_ASSIGNED_COL1");
	assignedAgentListCol1->Clear();
	auto assignedAgentListCol2 = form->FindControlTyped<ListBox>("LIST_ASSIGNED_COL2");
	assignedAgentListCol2->Clear();
	for (auto &agent : state->agents)
	{
		bool assigned_to_current_lab = false;
		if (agent.second->home_base != this->base)
			continue;

		if (agent.second->type != listedAgentType)
			continue;

		if (agent.second->assigned_to_lab)
		{
			for (auto &assigned_agent : this->selected_lab->assigned_agents)
			{
				if (assigned_agent.getSp() == agent.second)
				{
					assigned_to_current_lab = true;
					break;
				}
			}
			if (!assigned_to_current_lab)
				continue;
		}
		auto agentControl = this->createAgentControl({130, 40}, {state.get(), agent.second});

		if (assigned_to_current_lab)
		{
			if (assignedAgentListCol1->Controls.size() < 5)
				assignedAgentListCol1->AddItem(agentControl);
			else
				assignedAgentListCol2->AddItem(agentControl);
		}
		else
		{
			unassignedAgentList->AddItem(agentControl);
		}
	}
}
// FIXME: Put this in the rules somewhere?
// FIXME: This could be shared with the citview ICON_RESOURCES?
static const UString agentFramePath =
    "PCK:xcom3/UFODATA/VS_ICON.PCK:xcom3/UFODATA/VS_ICON.TAB:37:xcom3/UFODATA/PAL_01.DAT";

sp<Control> ResearchScreen::createAgentControl(Vec2<int> size, StateRef<Agent> agent)
{
	auto baseControl = mksp<Control>();
	baseControl->SetData(agent.getSp());
	baseControl->Name = "AGENT_PORTRAIT";
	baseControl->Size = size;

	auto frameGraphic = baseControl->createChild<Graphic>(fw().data->load_image(agentFramePath));
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

	auto font = fw().gamecore->GetFont("SMALFONT");

	auto nameLabel = baseControl->createChild<Label>(agent->name, font);
	nameLabel->Location = {40, 0};
	nameLabel->Size = {100, font->GetFontHeight() * 2};

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
		LogError("Trying to show non-scientist agent %s (%s)", agent.id.c_str(),
		         agent->name.c_str());
	}

	auto skillLabel = baseControl->createChild<Label>(UString::format(tr("Skill %s"), skill), font);
	skillLabel->Size = {100, font->GetFontHeight()};
	skillLabel->Location = {40, font->GetFontHeight() * 2};

	return baseControl;
}

}; // namespace OpenApoc
