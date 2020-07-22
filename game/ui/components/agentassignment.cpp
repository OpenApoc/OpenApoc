#define _USE_MATH_DEFINES

#include "game/ui/components/agentassignment.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/label.h"
#include "forms/listbox.h"
#include "forms/multilistbox.h"
#include "forms/ui.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/font.h"
#include "framework/framework.h"
#include "framework/renderer.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/vehicle.h"
#include "game/state/gamestate.h"
#include "game/state/shared/agent.h"
#include "game/state/shared/organisation.h"
#include "game/state/stateobject.h"
#include "game/ui/base/vequipscreen.h"
#include "game/ui/components/controlgenerator.h"
#include "game/ui/general/aequipscreen.h"
#include <cmath>

namespace OpenApoc
{
const UString AgentAssignment::AGENT_SELECT_BOX("AGENT_SELECT_BOX");
const UString AgentAssignment::AGENT_LIST_NAME("AGENT_LIST");
const UString AgentAssignment::VEHICLE_LIST_NAME("VEHICLE_LIST");

AgentAssignment::AgentAssignment(sp<GameState> state) : Form(), state(state) {}

void AgentAssignment::init(sp<Form> form, Vec2<int> location, Vec2<int> size)
{
	form->copyControlData(shared_from_this());
	Location = location;
	Size = size;

	draggedList = findControlTyped<MultilistBox>("DRAGGED_AGENT_BOX");
	draggedList->setVisible(false);

	// update the vehicle's icon
	funcVehicleUpdate = [this](sp<Control> c) {
		if (auto vehicle = c->getData<Vehicle>())
		{
			auto icon = c->findControl(ControlGenerator::VEHICLE_ICON_NAME);
			if (icon && *icon->getData<int>() != std::min(13, vehicle->getPassengers()))
			{
				auto newIcon = ControlGenerator::createVehicleIcon(*state, vehicle);
				c->replaceChildByName(newIcon);
			}
		}
	};

	// update the agent's icon
	funcAgentUpdate = [this](sp<Control> c) {
		if (!c->isVisible())
		{
			return;
		}
		if (auto agent = c->getData<Agent>())
		{
			auto icon = c->findControl(ControlGenerator::AGENT_ICON_NAME);
			if (icon &&
			    *icon->getData<CityUnitState>() != ControlGenerator::getCityUnitState(agent))
			{
				auto newIcon = ControlGenerator::createAgentIcon(
				    *state, agent, UnitSelectionState::Unselected, false);
				c->replaceChildByName(newIcon);
			}
		}
	};

	// select/deselect individual agent
	funcHandleAgentSelection = [this](Event *e, sp<Control> c, bool select) {
		if (!e)
			return select;

		auto agent = c->getData<Agent>();
		if (agent)
		{
			this->currentAgent = agent;
		}
		auto icon = c->findControl(ControlGenerator::AGENT_ICON_NAME);
		if (agent && icon && c->isPointInsideControlBounds(e, icon) &&
		    Event::isPressed(e->forms().MouseInfo.Button, Event::MouseButton::Right))
		{
			this->isDragged = false;
			fw().stageQueueCommand(
			    {StageCmd::Command::PUSH, mksp<AEquipScreen>(this->state, agent)});

			return !select;
		}

		return select;
	};

	// select/deselect agents inside vehicle
	funcHandleVehicleSelection = [this](Event *e, sp<Control> c, bool select) {
		if (!e)
			return select;

		auto vehicle = c->getData<Vehicle>();
		auto icon = c->findControl(ControlGenerator::VEHICLE_ICON_NAME);
		if (vehicle && icon && c->isPointInsideControlBounds(e, icon) &&
		    Event::isPressed(e->forms().MouseInfo.Button, Event::MouseButton::Right))
		{
			this->isDragged = false;

			auto equipScreen = mksp<VEquipScreen>(this->state);
			equipScreen->setSelectedVehicle(vehicle);
			fw().stageQueueCommand({StageCmd::Command::PUSH, equipScreen});

			return !select;
		}

		auto agentList = c->findControlTyped<MultilistBox>(AGENT_LIST_NAME);
		if (select)
		{
			agentList->selectAll();
			this->currentVehicle = vehicle;
		}
		else
		{
			agentList->clearSelection();
		}

		return select;
	};

	// Selection render
	funcSelectionItemRender = [this](sp<Control> c) {
		fw().renderer->drawRect(c->Location + renderOffset, c->SelectionSize - renderOffset,
		                        SelectedColour);
	};

	// Hover render
	funcHoverItemRender = [this](sp<Control> c) {
		fw().renderer->drawRect(c->Location + renderOffset, c->SelectionSize - renderOffset,
		                        HoverColour);
	};
}

void AgentAssignment::setLocation(sp<Agent> agent)
{
	this->agent = agent;
	this->vehicle = agent->currentVehicle;
	this->building = agent->currentBuilding;

	if (agent->currentVehicle && agent->currentVehicle->currentBuilding)
	{
		this->building = agent->currentVehicle->currentBuilding;
	}

	updateLocation();
}

void AgentAssignment::setLocation(sp<Vehicle> vehicle)
{
	this->agent = nullptr;
	this->vehicle = vehicle;
	this->building = vehicle->currentBuilding;

	updateLocation();
}

void AgentAssignment::setLocation(sp<Building> building)
{
	this->agent = nullptr;
	this->vehicle = nullptr;
	this->building = building;

	updateLocation();
}

void AgentAssignment::setLocation()
{
	this->agent = nullptr;
	this->vehicle = nullptr;
	this->building = nullptr;

	updateLocation();
}

void AgentAssignment::updateLocation()
{
	agents.clear();
	vehicles.clear();
	buildings.clear();

	// update agents, vehicles and buildings lists
	if (building)
	{
		for (auto &a : state->agents)
		{
			if (a.second->owner == state->getPlayer() &&
			    a.second->type->role == AgentType::Role::Soldier &&
			    (a.second->currentBuilding == building ||
			     (a.second->currentVehicle &&
			      a.second->currentVehicle->currentBuilding == building)))
			{
				agents.emplace_back(a.second);
			}
		}
		for (auto &v : state->vehicles)
		{
			if (v.second->owner == state->getPlayer() && v.second->currentBuilding == building)
			{
				vehicles.emplace_back(v.second);
			}
		}
		buildings.emplace_back(building);
	}
	else if (vehicle)
	{
		for (auto &a : state->agents)
		{
			if (a.second->owner == state->getPlayer() &&
			    a.second->type->role == AgentType::Role::Soldier &&
			    a.second->currentVehicle == vehicle)
			{
				agents.emplace_back(a.second);
			}
		}
		vehicles.emplace_back(vehicle);
	}
	else if (agent)
	{
		agents.emplace_back(agent);
	}
	else
	{
		for (auto &a : state->agents)
		{
			if (a.second->owner == state->getPlayer() &&
			    a.second->type->role == AgentType::Role::Soldier)
			{
				agents.emplace_back(a.second);
			}
		}
		for (auto &v : state->vehicles)
		{
			if (v.second->owner == state->getPlayer())
			{
				vehicles.emplace_back(v.second);
			}
		}
		for (auto &b : state->current_city->buildings)
		{
			bool foundBuilding = false;
			for (auto &a : agents)
			{
				if (a->currentBuilding == b.second)
				{
					foundBuilding = true;
					buildings.emplace_back(b.second);
					break;
				}
			}
			if (foundBuilding)
				continue;

			for (auto &v : vehicles)
			{
				if (v->currentBuilding == b.second)
				{
					buildings.emplace_back(b.second);
					break;
				}
			}
		}
	}

	if (!vehicles.empty())
	{
		currentVehicle = vehicles.front();
	}
	if (!agents.empty())
	{
		currentAgent = agents.front();
	}

	/* Create tree.
	AGENT_SELECT_BOX
	DoubleListControl#1
	    |               |
	    LEFT_LIST       RIGHT_LIST
	    Building#1      Building#1
	        |               |
	        AGENT_LIST      VEHICLE_LIST
	        Agent#1         Vehicle#1
	        Agent#2             |
	        Agent#3             AGENT_LIST
	        Agent#4             Agent#5
	                            Agent#6
	                        Vehicle#2
	                            |
	                            AGENT_LIST
	DoubleListControl#2
	    |               |
	    LEFT_LIST       RIGHT_LIST
	    Building#2      Building#2
	        |               |
	        AGENT_LIST      VEHICLE_LIST
	        Agent#7         Vehicle#3
	        Agent#8             |
	                            AGENT_LIST
	*/

	// create general list
	auto agentSelectBox = findControlTyped<MultilistBox>(AGENT_SELECT_BOX);
	agentSelectBox->clear();

	SelectedColour = agentSelectBox->SelectedColour;
	HoverColour = agentSelectBox->HoverColour;
	agentSelectBox->HoverColour = agentSelectBox->SelectedColour = {0, 0, 0, 0};

	const int offset = 20;
	for (auto &b : buildings)
	{
		auto doubleListControl = ControlGenerator::createDoubleListControl(agentSelectBox->Size.x);
		agentSelectBox->addItem(doubleListControl);

		auto leftList =
		    doubleListControl->findControlTyped<MultilistBox>(ControlGenerator::LEFT_LIST_NAME);
		auto rightList =
		    doubleListControl->findControlTyped<MultilistBox>(ControlGenerator::RIGHT_LIST_NAME);
		leftList->ItemSpacing = rightList->ItemSpacing = agentSelectBox->ItemSpacing;

		// create left list
		auto buildingLeftControl = ControlGenerator::createBuildingAssignmentControl(*state, b);
		// MouseUp - drop dragged list
		buildingLeftControl->addCallback(
		    FormEventType::MouseUp, [buildingLeftControl, this](FormsEvent *e) {
			    if (!isDragged || e->forms().RaisedBy == sourceRaisedBy)
				    return;

			    isDragged = false;
			    draggedList->setVisible(false);

			    auto building = buildingLeftControl->getData<Building>();
			    bool success = false;
			    for (auto &agentControl : draggedList->Controls)
			    {
				    auto agent = agentControl->getData<Agent>();
				    auto currentBuilding = agent->currentVehicle
				                               ? agent->currentVehicle->currentBuilding
				                               : agent->currentBuilding;
				    success = building == (sp<Building>)currentBuilding;
				    if (!success)
					    break;
				    agent->enterBuilding(*state, currentBuilding);
			    }

			    if (success)
			    {
				    buildingLeftControl->findControl(AGENT_LIST_NAME)->setDirty();
				    sourceRaisedBy->clearSelection();
			    }
		    });
		leftList->addItem(buildingLeftControl);

		auto agentLeftList = buildingLeftControl->createChild<MultilistBox>();
		agentLeftList->Name = AGENT_LIST_NAME;
		agentLeftList->Location.x = offset;
		agentLeftList->Location.y =
		    buildingLeftControl->SelectionSize.y + agentSelectBox->ItemSpacing;
		agentLeftList->ItemSpacing = agentSelectBox->ItemSpacing;
		// set visibility filter
		agentLeftList->setFuncIsVisibleItem([](sp<Control> c) {
			auto agent = c->getData<Agent>();
			bool visible =
			    !agent->currentVehicle &&
			    agent->currentBuilding == c->getParent()->getParent()->getData<Building>();
			c->setVisible(visible);
			return visible;
		});
		// set selection behaviour
		agentLeftList->setFuncHandleSelection(funcHandleAgentSelection);
		// set selection render
		agentLeftList->setFuncSelectionItemRender(funcSelectionItemRender);
		// set hover render
		agentLeftList->setFuncHoverItemRender(funcHoverItemRender);
		// MouseDown - ready for drag
		agentLeftList->addCallback(FormEventType::MouseDown, [agentLeftList, this](FormsEvent *e) {
			if (e->forms().RaisedBy == agentLeftList)
			{
				this->sourceRaisedBy = agentLeftList;
				this->isDragged = true;
			}
		});
		// MouseUp - drop dragged list
		agentLeftList->addCallback(FormEventType::MouseUp, [agentLeftList, this](FormsEvent *e) {
			if (!isDragged || e->forms().RaisedBy == sourceRaisedBy)
				return;

			isDragged = false;
			draggedList->setVisible(false);

			auto building = agentLeftList->getParent()->getData<Building>();
			bool success = false;
			for (auto &agentControl : draggedList->Controls)
			{
				auto agent = agentControl->getData<Agent>();
				auto currentBuilding = agent->currentVehicle
				                           ? agent->currentVehicle->currentBuilding
				                           : agent->currentBuilding;
				success = building == (sp<Building>)currentBuilding;
				if (!success)
					break;
				agent->enterBuilding(*state, currentBuilding);
			}

			if (success)
			{
				agentLeftList->setDirty();
				sourceRaisedBy->clearSelection();
			}
		});

		addAgentsToList(agentLeftList, offset);

		// create right list
		addBuildingToRightList(b, rightList, 0);
	}

	if (!this->building && this->vehicle)
	{
		// single vehicle - add to the right list
		auto doubleListControl = ControlGenerator::createDoubleListControl(agentSelectBox->Size.x);
		agentSelectBox->addItem(doubleListControl);

		auto rightList =
		    doubleListControl->findControlTyped<MultilistBox>(ControlGenerator::RIGHT_LIST_NAME);
		rightList->ItemSpacing = agentSelectBox->ItemSpacing;
		addVehiclesToList(rightList, 0);
	}
	else if (!this->building && !this->vehicle && this->agent)
	{
		// single agent - add to the left list
		auto doubleListControl = ControlGenerator::createDoubleListControl(agentSelectBox->Size.x);
		agentSelectBox->addItem(doubleListControl);

		auto leftList =
		    doubleListControl->findControlTyped<MultilistBox>(ControlGenerator::LEFT_LIST_NAME);
		addAgentsToList(leftList, 0);
	}
}

void AgentAssignment::addAgentsToList(sp<MultilistBox> list, const int listOffset)
{
	for (auto &a : agents)
	{
		auto agentControl = ControlGenerator::createAgentAssignmentControl(*state, a);
		agentControl->setFuncPreRender(funcAgentUpdate);
		agentControl->Size.x -= listOffset;
		agentControl->SelectionSize.x -= listOffset;
		list->addItem(agentControl);
	}
}

void AgentAssignment::addVehiclesToList(sp<MultilistBox> list, const int listOffset)
{
	const int offset = 20;
	for (auto &v : vehicles)
	{
		auto vehicleControl = ControlGenerator::createVehicleAssignmentControl(*state, v);
		vehicleControl->setFuncPreRender(funcVehicleUpdate);
		vehicleControl->Size.x -= listOffset;
		vehicleControl->SelectionSize.x -= listOffset;
		// MouseUp - drop dragged list
		vehicleControl->addCallback(FormEventType::MouseUp, [vehicleControl, this](FormsEvent *e) {
			if (!isDragged || e->forms().RaisedBy == sourceRaisedBy)
				return;

			isDragged = false;
			draggedList->setVisible(false);

			auto vehicle = vehicleControl->getData<Vehicle>();
			bool success = false;
			for (auto &agentControl : draggedList->Controls)
			{
				auto agent = agentControl->getData<Agent>();
				auto currentBuilding = agent->currentVehicle
				                           ? agent->currentVehicle->currentBuilding
				                           : agent->currentBuilding;
				success =
				    vehicle->currentBuilding == currentBuilding && agent->currentVehicle != vehicle;
				if (!success)
					break;
				agent->enterVehicle(*state, {state.get(), vehicle});
			}

			if (success)
			{
				vehicleControl->findControl(AGENT_LIST_NAME)->setDirty();
				sourceRaisedBy->clearSelection();
			}
		});
		list->addItem(vehicleControl);

		auto agentRightList = vehicleControl->createChild<MultilistBox>();
		agentRightList->Name = AGENT_LIST_NAME;
		agentRightList->Location.x = offset;
		agentRightList->Location.y = vehicleControl->SelectionSize.y + list->ItemSpacing;
		agentRightList->ItemSpacing = list->ItemSpacing;
		// set visibility filter
		agentRightList->setFuncIsVisibleItem([](sp<Control> c) {
			auto agent = c->getData<Agent>();
			bool visible = agent->currentVehicle == c->getParent()->getParent()->getData<Vehicle>();
			c->setVisible(visible);
			return visible;
		});
		// set selection behaviour
		agentRightList->setFuncHandleSelection(funcHandleAgentSelection);
		// set selection render
		agentRightList->setFuncSelectionItemRender(funcSelectionItemRender);
		// set hover render
		agentRightList->setFuncHoverItemRender(funcHoverItemRender);
		// MouseDown - ready for drag
		agentRightList->addCallback(FormEventType::MouseDown,
		                            [agentRightList, this](FormsEvent *e) {
			                            if (e->forms().RaisedBy == agentRightList)
			                            {
				                            this->sourceRaisedBy = agentRightList;
				                            this->isDragged = true;
			                            }
		                            });

		addAgentsToList(agentRightList, offset + listOffset);
	}
}

void AgentAssignment::addBuildingToRightList(sp<Building> building, sp<MultilistBox> list,
                                             const int listOffset)
{
	const int offset = 20;

	auto buildingControl = ControlGenerator::createBuildingAssignmentControl(*state, building);
	buildingControl->Size.x -= listOffset;
	buildingControl->SelectionSize.x -= listOffset;
	list->addItem(buildingControl);

	auto vehicleRightList = buildingControl->createChild<MultilistBox>();
	vehicleRightList->Name = VEHICLE_LIST_NAME;
	vehicleRightList->Location.x = offset;
	vehicleRightList->Location.y = buildingControl->SelectionSize.y + list->ItemSpacing;
	vehicleRightList->ItemSpacing = list->ItemSpacing;
	// set visibility filter
	vehicleRightList->setFuncIsVisibleItem([](sp<Control> c) {
		auto vehicle = c->getData<Vehicle>();
		bool visible = vehicle->currentBuilding == c->getParent()->getParent()->getData<Building>();
		c->setVisible(visible);
		return visible;
	});
	// set selection behaviour
	vehicleRightList->setFuncHandleSelection(funcHandleVehicleSelection);
	// set selection render
	vehicleRightList->setFuncSelectionItemRender(funcSelectionItemRender);
	// set hover render
	vehicleRightList->setFuncHoverItemRender(funcHoverItemRender);

	addVehiclesToList(vehicleRightList, offset + listOffset);
}

/**
 * Get selected agents with preservation of order.
 */
std::list<StateRef<Agent>> AgentAssignment::getSelectedAgents() const
{
	std::set<sp<Agent>> agentControlSet;

	auto agentSelectBox = findControlTyped<MultilistBox>(AGENT_SELECT_BOX);

	for (auto &doubleListControl : agentSelectBox->Controls)
	{
		// left side
		if (auto leftList =
		        doubleListControl->findControlTyped<MultilistBox>(ControlGenerator::LEFT_LIST_NAME))
		{
			if (auto agentList = leftList->findControlTyped<MultilistBox>(AGENT_LIST_NAME))
			{
				for (auto &sel : agentList->getSelectedSet())
				{
					if (auto a = sel->getData<Agent>())
					{
						agentControlSet.insert(a);
					}
				}
			}
		}

		// right side
		if (auto rightList = doubleListControl->findControlTyped<MultilistBox>(
		        ControlGenerator::RIGHT_LIST_NAME))
		{
			if (auto vehicleList = rightList->findControlTyped<MultilistBox>(VEHICLE_LIST_NAME))
			{
				for (auto &vehicleControl : vehicleList->Controls)
				{
					if (auto agentList =
					        vehicleControl->findControlTyped<MultilistBox>(AGENT_LIST_NAME))
					{
						for (auto &sel : agentList->getSelectedSet())
						{
							if (auto a = sel->getData<Agent>())
							{
								agentControlSet.insert(a);
							}
						}
					}
				}
			}
		}
	}

	std::list<StateRef<Agent>> agents;
	for (auto &a : state->agents)
	{
		if (agentControlSet.find(a.second) != agentControlSet.end())
		{
			agents.emplace_back(state.get(), a.second);
		}
	}

	return agents;
}

/**
 * Get selected vehicles with preservation of order.
 */
std::list<StateRef<Vehicle>> AgentAssignment::getSelectedVehicles() const
{
	std::set<sp<Vehicle>> vehicleControlSet;

	auto agentSelectBox = findControlTyped<MultilistBox>(AGENT_SELECT_BOX);

	for (auto &doubleListControl : agentSelectBox->Controls)
	{
		// only right side used
		if (auto rightList = doubleListControl->findControlTyped<MultilistBox>(
		        ControlGenerator::RIGHT_LIST_NAME))
		{
			if (auto vehicleList = rightList->findControlTyped<MultilistBox>(VEHICLE_LIST_NAME))
			{
				for (auto &sel : vehicleList->getSelectedSet())
				{
					if (auto v = sel->getData<Vehicle>())
					{
						vehicleControlSet.insert(v);
					}
				}
			}
		}
	}

	std::list<StateRef<Vehicle>> vehicles;
	for (auto &v : state->vehicles)
	{
		if (vehicleControlSet.find(v.second) != vehicleControlSet.end())
		{
			vehicles.emplace_back(state.get(), v.second);
		}
	}

	return vehicles;
}

void AgentAssignment::eventOccured(Event *e)
{
	Form::eventOccured(e);

	switch (e->type())
	{
		case EVENT_FORM_INTERACTION:
			if (e->forms().EventFlag == FormEventType::MouseUp)
			{
				if (isDragged && (e->forms().RaisedBy == sourceRaisedBy ||
				                  e->forms().RaisedBy->Name == "FORM_AGENT_ASSIGNMENT"))
				{
					// miss click
					isDragged = false;
					draggedList->setVisible(false);
				}
			}
			break;

		case EVENT_MOUSE_DOWN:
			positionX = e->mouse().X;
			positionY = e->mouse().Y;
			break;

		case EVENT_MOUSE_MOVE:
			// if dragged - change location of dragged list
			if (isDragged)
			{
				int distance = (positionX - e->mouse().X) * (positionX - e->mouse().X) +
				               (positionY - e->mouse().Y) * (positionY - e->mouse().Y);
				if (distance > insensibility)
				{
					draggedList->Location.x = e->mouse().X - this->resolvedLocation.x;
					draggedList->Location.y = e->mouse().Y - this->resolvedLocation.y;
					if (!draggedList->isVisible())
					{
						// time to make the list
						draggedList->clear();
						draggedList->setVisible(true);
						for (auto &a : sourceRaisedBy->getSelectedItems())
						{
							a->copyTo(draggedList)->setData(a->getData<Agent>());
						}
					}
				}
			}
			break;
		default:
			// All other events ignored
			break;
	}
}

void AgentAssignment::update() { Form::update(); }
} // namespace OpenApoc
