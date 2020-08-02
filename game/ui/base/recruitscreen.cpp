#include "game/ui/base/recruitscreen.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/listbox.h"
#include "forms/radiobutton.h"
#include "forms/scrollbar.h"
#include "forms/ui.h"
#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/font.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "framework/renderer.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/city/vequipment.h"
#include "game/state/gamestate.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/shared/aequipment.h"
#include "game/state/shared/organisation.h"
#include "game/ui/components/controlgenerator.h"
#include "game/ui/general/agentsheet.h"
#include "game/ui/general/messagebox.h"
#include "library/strings_format.h"

namespace OpenApoc
{

namespace
{
static const std::map<AgentType::Role, int> hireCost = {
    {AgentType::Role::Soldier, HIRE_COST_SOLDIER},
    {AgentType::Role::BioChemist, HIRE_COST_BIO},
    {AgentType::Role::Physicist, HIRE_COST_PHYSIC},
    {AgentType::Role::Engineer, HIRE_COST_ENGI}};
static const std::map<AgentType::Role, int> fireCost = {
    {AgentType::Role::Soldier, FIRE_COST_SOLDIER},
    {AgentType::Role::BioChemist, FIRE_COST_BIO},
    {AgentType::Role::Physicist, FIRE_COST_PHYSIC},
    {AgentType::Role::Engineer, FIRE_COST_ENGI}};
} // namespace

RecruitScreen::RecruitScreen(sp<GameState> state)
    : BaseStage(state), bigUnitRanks(getBigUnitRanks())
{
	// Load resources
	form = ui().getForm("recruitscreen");
	formAgentStats = form->findControlTyped<Form>("AGENT_STATS_VIEW");
	formPersonnelStats = form->findControlTyped<Form>("PERSONNEL_STATS_VIEW");
	formAgentStats->setVisible(false);
	formPersonnelStats->setVisible(false);

	// Assign event handlers
	onHover = [this](FormsEvent *e) {
		auto list = std::static_pointer_cast<ListBox>(e->forms().RaisedBy);
		auto agent = list->getHoveredData<Agent>();
		if (agent)
			displayAgentStats(*agent);
	};

	form->findControlTyped<ListBox>("LIST1")->addCallback(FormEventType::ListBoxChangeHover,
	                                                      onHover);
	form->findControlTyped<ListBox>("LIST2")->addCallback(FormEventType::ListBoxChangeHover,
	                                                      onHover);

	arrow = form->findControlTyped<Graphic>("MAGIC_ARROW");
	textViewBaseStatic = form->findControlTyped<Label>("TEXT_BUTTON_BASE_STATIC");
	form->findControlTyped<RadioButton>("BUTTON_SOLDIERS")->setChecked(true);
	type = AgentType::Role::Soldier;
	viewHighlight = BaseGraphics::FacilityHighlight::Quarters;

	// Adding callbacks after checking the button because we don't need to
	// have the callback be called since changeBase() will update display anyways

	form->findControlTyped<RadioButton>("BUTTON_SOLDIERS")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->setDisplayType(AgentType::Role::Soldier); });
	form->findControlTyped<RadioButton>("BUTTON_BIOSCIS")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->setDisplayType(AgentType::Role::BioChemist); });
	form->findControlTyped<RadioButton>("BUTTON_PHYSCIS")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->setDisplayType(AgentType::Role::Physicist); });
	form->findControlTyped<RadioButton>("BUTTON_ENGINRS")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->setDisplayType(AgentType::Role::Engineer); });

	populateAgentList();

	for (auto &list : agentLists)
	{
		for (auto &control : list)
		{
			// MouseClick - move an agent to opposite list
			control->addCallback(FormEventType::MouseClick, [this](FormsEvent *e) {
				int leftIndex = getLeftIndex();
				int rightIndex = 8;

				auto listLeft = form->findControlTyped<ListBox>("LIST1");
				auto listRight = form->findControlTyped<ListBox>("LIST2");

				auto agentControl = e->forms().RaisedBy;
				if (std::find(agentLists[leftIndex].begin(), agentLists[leftIndex].end(),
				              agentControl) != agentLists[leftIndex].end())
				{
					listLeft->removeItem(agentControl);
					listRight->addItem(agentControl);
					agentLists[leftIndex].erase(std::find(
					    agentLists[leftIndex].begin(), agentLists[leftIndex].end(), agentControl));
					agentLists[rightIndex].push_back(agentControl);
				}
				else if (this->state->current_base->getUsage(
				             *(this->state), FacilityType::Capacity::Quarters, lqDelta + 1) > 100)
				{
					fw().stageQueueCommand(
					    {StageCmd::Command::PUSH,
					     mksp<MessageBox>(tr("Accomodation exceeded"),
					                      tr("Transfer limited by available accommodation."),
					                      MessageBox::ButtonOptions::Ok)});
				}
				else
				{
					listRight->removeItem(agentControl);
					listLeft->addItem(agentControl);
					agentLists[rightIndex].erase(std::find(agentLists[rightIndex].begin(),
					                                       agentLists[rightIndex].end(),
					                                       agentControl));
					agentLists[leftIndex].push_back(agentControl);
				}

				updateFormValues();
			});
		}
	}
}

RecruitScreen::~RecruitScreen() = default;

/**
 * Populate the agentList.
 */
void RecruitScreen::populateAgentList()
{
	agentLists.resize(9);

	std::map<UString, int> bases;
	int index = 0;
	for (auto &b : state->player_bases)
	{
		bases[b.first] = index;
		index++;
	}

	auto player = state->getPlayer();
	auto list = form->findControlTyped<ListBox>("LIST2");

	// Populate list of agents
	for (auto &a : state->agents)
	{
		UnitSkillState skill = UnitSkillState::Vertical;
		if (a.second->type->role == AgentType::Role::Soldier)
			skill = UnitSkillState::Hidden;
		if (a.second->owner == player)
		{
			// Need to be able to strip agent
			if (a.second->currentBuilding == a.second->homeBuilding)
			{
				agentLists[bases[a.second->homeBuilding->base.id]].push_back(
				    ControlGenerator::createLargeAgentControl(*state, a.second, list->Size.x,
				                                              skill));
			}
		}
		else if (a.second->owner->hirableAgentTypes.find(a.second->type) !=
		         a.second->owner->hirableAgentTypes.end())
		{
			agentLists[8].push_back(
			    ControlGenerator::createLargeAgentControl(*state, a.second, list->Size.x, skill));
		}
	}
}

void RecruitScreen::changeBase(sp<Base> newBase)
{
	BaseStage::changeBase(newBase);
	textViewBaseStatic->setText(state->current_base->name);

	formAgentStats->setVisible(false);
	formPersonnelStats->setVisible(false);

	// Apply display type and base highlight
	setDisplayType(type);
}

void RecruitScreen::setDisplayType(const AgentType::Role role)
{
	if (this->type != role)
	{
		formAgentStats->setVisible(false);
		formPersonnelStats->setVisible(false);
		this->type = role;
	}

	form->findControlTyped<ScrollBar>("LIST1_SCROLL")->setValue(0);
	form->findControlTyped<ScrollBar>("LIST2_SCROLL")->setValue(0);
	auto listLeft = form->findControlTyped<ListBox>("LIST1");
	auto listRight = form->findControlTyped<ListBox>("LIST2");

	int leftIndex = getLeftIndex();
	int rightIndex = 8;

	listLeft->clear();
	listRight->clear();

	for (auto &a : agentLists[leftIndex])
	{
		if (a->getData<Agent>()->type->role == role)
		{
			listLeft->addItem(a);
		}
	}
	for (auto &a : agentLists[rightIndex])
	{
		if (a->getData<Agent>()->type->role == role)
		{
			listRight->addItem(a);
		}
	}

	auto label = form->findControlTyped<Label>("AGENTS_CAPTION_LEFT");
	switch (type)
	{
		case AgentType::Role::Soldier:
			label->setText(tr("X-COM Agents"));
			break;
		case AgentType::Role::BioChemist:
			label->setText(tr("Biochemists"));
			break;
		case AgentType::Role::Physicist:
			label->setText(tr("Quantum Physicists"));
			break;
		case AgentType::Role::Engineer:
			label->setText(tr("Engineers"));
			break;
	}

	// Update display for bases
	updateFormValues();
}

int RecruitScreen::getLeftIndex()
{
	int index = 0;
	for (auto &b : state->player_bases)
	{
		if (b.first == state->current_base.id)
		{
			return index;
		}
		index++;
	}
	return 8;
}

void RecruitScreen::updateFormValues()
{
	int leftIndex = getLeftIndex();
	int moneyDelta = 0;
	lqDelta = 0;

	auto player = state->getPlayer();
	std::map<UString, int> bases;
	int index = 0;
	for (auto &b : state->player_bases)
	{
		bases[b.first] = index;
		index++;
	}

	// This base
	for (auto &a : agentLists[leftIndex])
	{
		auto agent = a->getData<Agent>();
		// Hired to this
		if (agent->owner != player)
		{
			lqDelta++;
			moneyDelta -= hireCost.at(agent->type->role);
		}
		// Transferred to this
		else if (bases[agent->homeBuilding->base.id] != leftIndex)
		{
			lqDelta++;
		}
	}

	// Other bases
	for (int i = 0; i < 8; i++)
	{
		if (i == leftIndex)
		{
			continue;
		}
		for (auto &a : agentLists[i])
		{
			auto agent = a->getData<Agent>();
			// Hired to other
			if (agent->owner != player)
			{
				moneyDelta -= hireCost.at(agent->type->role);
			}
			// Transferred to other
			else if (bases[agent->homeBuilding->base.id] == leftIndex)
			{
				lqDelta--;
			}
		}
	}

	// Fired
	for (auto &a : agentLists[8])
	{
		auto agent = a->getData<Agent>();
		if (agent->owner == player)
		{
			// Fired from any base
			moneyDelta -= fireCost.at(agent->type->role);
			// Fired from this base in particular
			if (bases[agent->homeBuilding->base.id] == leftIndex)
			{
				lqDelta--;
			}
		}
	}

	// Update money
	int balance = state->getPlayer()->balance + moneyDelta;
	form->findControlTyped<Label>("TEXT_FUNDS")->setText(Strings::fromInteger(balance));
	form->findControlTyped<Label>("TEXT_FUNDS_DELTA")
	    ->setText(format("%s%s", moneyDelta > 0 ? "+" : "", Strings::fromInteger(moneyDelta)));

	updateBaseHighlight();
}

void RecruitScreen::updateBaseHighlight()
{
	int usage = state->current_base->getUsage(*state, FacilityType::Capacity::Quarters, lqDelta);
	fillBaseBar(usage);
	auto facilityLabel = form->findControlTyped<Label>("FACILITY_FIRST_TEXT");
	facilityLabel->setText(format("%i%%", usage));
}

void RecruitScreen::fillBaseBar(int percent)
{
	auto facilityBar = form->findControlTyped<Graphic>("FACILITY_FIRST_FILL");
	facilityBar->setVisible(true);

	auto progressImage = mksp<RGBImage>(facilityBar->Size);
	int redHeight = progressImage->size.y * std::min(100, percent) / 100;
	{
		// FIXME: For some reason, there's no border here like in the research screen, so we
		// have to make one manually, probably there's a better way
		RGBImageLock l(progressImage);
		for (int x = 0; x < 2; x++)
		{
			for (int y = 1; y <= progressImage->size.y; y++)
			{
				if (y < redHeight)
				{
					l.set({x, progressImage->size.y - y}, {255, 0, 0, 255});
				}
			}
		}
	}
	facilityBar->setImage(progressImage);
}

/**
 * Display stats of an agent
 * @agent - the agent
 */
void RecruitScreen::displayAgentStats(const Agent &agent)
{

	switch (agent.type->role)
	{
		case AgentType::Role::Soldier:
			AgentSheet(formAgentStats).display(agent, bigUnitRanks, false);
			formAgentStats->setVisible(true);
			formPersonnelStats->setVisible(false);
			break;
		default:
			personnelSheet(agent, formPersonnelStats);
			formAgentStats->setVisible(false);
			formPersonnelStats->setVisible(true);
	}
}

/**
 * Fills the form of personnel's statistics. Such as skill and that's all.
 * @agent - an agent whose stats will be displayed
 * @formPersonnelStats - a form of stats
 */
void RecruitScreen::personnelSheet(const Agent &agent, sp<Form> formPersonnelStats)
{
	formPersonnelStats->findControlTyped<Label>("AGENT_NAME")->setText(agent.name);
	formPersonnelStats->findControlTyped<Graphic>("SELECTED_PORTRAIT")
	    ->setImage(agent.getPortrait().photo);
	formPersonnelStats->findControlTyped<Label>("VALUE_SKILL")
	    ->setText(format("%d", agent.getSkill()));
}

/**
 * Loads and returns big pictures of ranks.
 */
std::vector<sp<Image>> RecruitScreen::getBigUnitRanks()
{
	std::vector<sp<Image>> bigUnitRanks;

	for (int i = 12; i <= 18; i++)
	{
		bigUnitRanks.push_back(
		    fw().data->loadImage(format("PCK:xcom3/tacdata/tacbut.pck:xcom3/tacdata/"
		                                "tacbut.tab:%d:xcom3/tacdata/tactical.pal",
		                                i)));
	}

	return bigUnitRanks;
}

void RecruitScreen::attemptCloseScreen()
{
	auto player = state->getPlayer();
	std::map<UString, int> bases;
	int index = 0;
	for (auto &b : state->player_bases)
	{
		bases[b.first] = index;
		index++;
	}

	int hired = 0;
	int transferred = 0;
	for (int i = 0; i < 8; i++)
	{
		for (auto &a : agentLists[i])
		{
			auto agent = a->getData<Agent>();
			if (agent->owner != player)
			{
				hired++;
			}
			else if (bases[agent->homeBuilding->base.id] != i)
			{
				transferred++;
			}
		}
	}

	int fired = 0;
	for (auto &a : agentLists[8])
	{
		if (a->getData<Agent>()->owner == player)
		{
			auto agent = a->getData<Agent>();
			if (agent->owner == player)
			{
				fired++;
			}
		}
	}

	if (hired != 0 || fired != 0 || transferred != 0)
	{
		UString message =
		    format("%d %s\n%d %s", hired, tr("unit(s) hired"), fired, tr("unit(s) fired."));
		if (transferred > 0)
		{
			message = format("%s\n(%d %s)", message, transferred, tr("units(s) transferred"));
		}
		fw().stageQueueCommand(
		    {StageCmd::Command::PUSH,
		     mksp<MessageBox>(
		         tr("Confirm Orders"), message, MessageBox::ButtonOptions::YesNoCancel,
		         [this] { this->closeScreen(true); }, [this] { this->closeScreen(); })});
	}
	else
	{
		closeScreen();
	}
}

void RecruitScreen::executeOrders()
{
	std::vector<StateRef<Base>> bases;
	for (auto &b : state->player_bases)
	{
		bases.push_back(b.second->building->base);
	}
	bases.resize(8);

	auto player = state->getPlayer();

	for (int i = 0; i < 8; i++)
	{
		for (auto &a : agentLists[i])
		{
			auto agent = a->getData<Agent>();
			if (bases[i] != agent->homeBuilding->base)
			{
				if (agent->owner != player)
				{
					agent->hire(*state, bases[i]->building);
					player->balance -= hireCost.at(agent->type->role);
				}
				else
				{
					agent->transfer(*state, bases[i]->building);
				}
			}
		}
	}

	for (auto &a : agentLists[8])
	{
		auto agent = a->getData<Agent>();
		if (agent->owner == state->getPlayer())
		{
			player->balance -= fireCost.at(agent->type->role);
			std::list<sp<AEquipment>> equipmentToStrip;
			for (auto &e : agent->equipment)
			{
				if (e->payloadType)
				{
					equipmentToStrip.push_back(e->unloadAmmo());
				}
				equipmentToStrip.push_back(e);
			}
			// don't care to actually strip them, just add to inventory
			for (auto &e : equipmentToStrip)
			{
				agent->homeBuilding->base->inventoryAgentEquipment[e->type.id] +=
				    e->type->type == AEquipmentType::Type::Ammo ? e->ammo : 1;
			}
			agent->die(*state, true);
		}
	}
}

void RecruitScreen::closeScreen(bool confirmed)
{
	if (!confirmed)
	{
		fw().stageQueueCommand({StageCmd::Command::POP});
		return;
	}

	// Step 01 : Calculate money and lq deltas
	int moneyDelta = 0;
	std::vector<int> vecLqDelta;
	vecLqDelta.resize(8);

	auto player = state->getPlayer();
	std::map<UString, int> bases;
	int index = 0;
	for (auto &b : state->player_bases)
	{
		bases[b.first] = index;
		index++;
	}

	// Hirees and transfers
	for (int i = 0; i < 8; i++)
	{
		for (auto &a : agentLists[i])
		{
			auto agent = a->getData<Agent>();
			// Hired
			if (agent->owner != player)
			{
				vecLqDelta[i]++;
				moneyDelta -= hireCost.at(agent->type->role);
			}
			// Moved away from his base to this base
			else if (bases[agent->homeBuilding->base.id] != i)
			{
				vecLqDelta[bases[agent->homeBuilding->base.id]]--;
				vecLqDelta[i]++;
			}
		}
	}
	// Fired
	for (auto &a : agentLists[8])
	{
		auto agent = a->getData<Agent>();
		if (agent->owner == player)
		{
			vecLqDelta[bases[agent->homeBuilding->base.id]]--;
			moneyDelta -= fireCost.at(agent->type->role);
		}
	}

	// Step 02: Insufficient funds?
	if (player->balance + moneyDelta < 0)
	{
		fw().stageQueueCommand(
		    {StageCmd::Command::PUSH,
		     mksp<MessageBox>(tr("Funds exceeded"), tr("Order limited by your available funds."),
		                      MessageBox::ButtonOptions::Ok)});
		return;
	}

	// Step 03: Insufficient space?
	// Check every base, find first bad one
	int bindex = 0;
	StateRef<Base> bad_base;
	for (auto &b : state->player_bases)
	{
		if (b.second->getUsage(*state, FacilityType::Capacity::Quarters, vecLqDelta[bindex]) > 100)
		{
			bad_base = b.second->building->base;
			break;
		}
		bindex++;
	}
	// Found bad base
	if (bad_base)
	{
		fw().stageQueueCommand({StageCmd::Command::PUSH,
		                        mksp<MessageBox>(tr("Accomodation exceeded"),
		                                         tr("Transfer limited by available accommodation."),
		                                         MessageBox::ButtonOptions::Ok)});
		if (bad_base != state->current_base)
		{
			for (auto &view : miniViews)
			{
				if (bad_base == view->getData<Base>())
				{
					currentView = view;
					changeBase(bad_base);
					break;
				}
			}
		}
		return;
	}

	// Sufficient funds and space, execute
	executeOrders();
	fw().stageQueueCommand({StageCmd::Command::POP});
}

void RecruitScreen::begin() { BaseStage::begin(); }

void RecruitScreen::pause() {}

void RecruitScreen::resume()
{
	form->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
}

void RecruitScreen::finish() {}

void RecruitScreen::eventOccurred(Event *e)
{
	form->eventOccured(e);

	if (e->type() == EVENT_MOUSE_MOVE)
	{
		arrow->setVisible(!(e->mouse().X > arrow->getLocationOnScreen().x));
	}

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE || e->keyboard().KeyCode == SDLK_RETURN ||
		    e->keyboard().KeyCode == SDLK_SPACE || e->keyboard().KeyCode == SDLK_KP_ENTER)
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
				attemptCloseScreen();
				return;
			}
		}
	}
}

void RecruitScreen::update() { form->update(); }

void RecruitScreen::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();

	textViewBaseStatic->setVisible(!textViewBase || !textViewBase->isVisible());

	form->render();
	BaseStage::render();
}

bool RecruitScreen::isTransition() { return false; }

}; // namespace OpenApoc
