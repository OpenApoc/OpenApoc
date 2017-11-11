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
#include "game/ui/components/controlgenerator.h"
#include <cmath>

namespace OpenApoc
{
const UString AgentAssignment::LEFT_LIST_NAME("AGENT_SELECT_BOX");
const UString AgentAssignment::RIGHT_LIST_NAME("VEHICLE_SELECT_BOX");
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
				newIcon->Location = {4, 3};
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
				newIcon->Location = {4, 3};
				c->replaceChildByName(newIcon);
			}
		}
	};

	// select/deselect individual agent
	funcHandleAgentSelection = [this](sp<Control> c, bool select) {
		if (select)
		{
			if (auto agent = c->getData<Agent>())
			{
				currentAgent = agent;
			}
		}
	};

	// select/deselect agents inside vehicle
	funcHandleVehicleSelection = [this](sp<Control> c, bool select) {
		auto agentList = c->findControlTyped<MultilistBox>(AGENT_LIST_NAME);
		if (select)
		{
			agentList->selectAll();
			if (auto vehicle = c->getData<Vehicle>())
			{
				currentVehicle = vehicle;
			}
		}
		else
		{
			agentList->clearSelection();
		}
	};

	// select/deselect agents inside building
	funcHandleBuildingSelection = [](sp<Control> c, bool select) {
		auto agentList = c->findControlTyped<MultilistBox>(AGENT_LIST_NAME);
		if (select)
		{
			agentList->selectAll();
		}
		else
		{
			agentList->clearSelection();
		}
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
			for (auto a : agents)
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

			for (auto v : vehicles)
			{
				if (v->currentBuilding == b.second)
				{
					buildings.emplace_back(b.second);
					break;
				}
			}
		}
	}

	// Create tree.
	// create left-side list
	auto baseLeftList = findControlTyped<MultilistBox>(LEFT_LIST_NAME);
	baseLeftList->setFuncHandleSelection(funcHandleBuildingSelection);
	baseLeftList->clear();

	const int offset = 20;
	for (auto b : buildings)
	{
		auto buildingControl = ControlGenerator::createBuildingAssignmentControl(*state, b);
		// MouseUp - drop dragged list
		buildingControl->addCallback(
		    FormEventType::MouseUp, [buildingControl, this](FormsEvent *e) {
			    if (!isDragged || e->forms().RaisedBy == sourceRaisedBy)
				    return;

			    isDragged = false;
			    draggedList->setVisible(false);

			    for (auto agent : draggedList->Controls)
			    {
				    agent->getData<Agent>()->enterBuilding(
				        *state,
				        {state.get(),
				         agent->getData<Agent>()->currentVehicle
				             ? agent->getData<Agent>()->currentVehicle->currentBuilding
				             : agent->getData<Agent>()->currentBuilding});
			    }

			    sourceRaisedBy->clearSelection();
			});
		baseLeftList->addItem(buildingControl);

		auto agentLeftList = buildingControl->createChild<MultilistBox>();
		agentLeftList->Name = AGENT_LIST_NAME;
		agentLeftList->Location.x = offset;
		agentLeftList->Location.y = buildingControl->SelectionSize.y;
		agentLeftList->HoverColour = baseLeftList->HoverColour;
		agentLeftList->SelectedColour = baseLeftList->SelectedColour;
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

			for (auto agent : draggedList->Controls)
			{
				agent->getData<Agent>()->enterBuilding(
				    *state,
				    {state.get(), agent->getData<Agent>()->currentVehicle->currentBuilding});
			}

			agentLeftList->setDirty();
			sourceRaisedBy->clearSelection();
		});

		addAgentsToList(agentLeftList, offset);
	}

	// create right-side list
	auto baseRightList = findControlTyped<MultilistBox>(RIGHT_LIST_NAME);
	baseRightList->clear();

	if (!this->building && this->vehicle)
	{
		addVehiclesToList(baseRightList, 0);
	}
	else if (!this->building && !this->vehicle && this->agent)
	{
		addAgentsToList(baseRightList, 0);
	}
	else // building || (!this->building && !this->vehicle && !this->agent)
	{
		addBuildingsToList(baseRightList, 0);
	}
}

void AgentAssignment::addAgentsToList(sp<MultilistBox> list, const int listOffset)
{
	for (auto a : agents)
	{
		auto agentControl = ControlGenerator::createAgentAssignmentControl(*state, a);
		agentControl->setFuncUpdate(funcAgentUpdate);
		agentControl->Size.x -= listOffset;
		agentControl->SelectionSize.x -= listOffset;
		list->addItem(agentControl);
	}
}

void AgentAssignment::addVehiclesToList(sp<MultilistBox> list, const int listOffset)
{
	// set selection behaviour
	list->setFuncHandleSelection(funcHandleVehicleSelection);

	const int offset = 20;
	for (auto v : vehicles)
	{
		auto vehicleControl = ControlGenerator::createVehicleAssignmentControl(*state, v);
		vehicleControl->setFuncUpdate(funcVehicleUpdate);
		vehicleControl->Size.x -= listOffset;
		vehicleControl->SelectionSize.x -= listOffset;
		// MouseUp - drop dragged list
		vehicleControl->addCallback(FormEventType::MouseUp, [vehicleControl, this](FormsEvent *e) {
			if (!isDragged || e->forms().RaisedBy == sourceRaisedBy)
				return;

			isDragged = false;
			draggedList->setVisible(false);

			for (auto agent : draggedList->Controls)
			{
				agent->getData<Agent>()->enterVehicle(
				    *state, {state.get(), vehicleControl->getData<Vehicle>()});
			}

			vehicleControl->findControl(AGENT_LIST_NAME)->setDirty();
			sourceRaisedBy->clearSelection();
		});
		list->addItem(vehicleControl);

		auto agentRightList = vehicleControl->createChild<MultilistBox>();
		agentRightList->Name = AGENT_LIST_NAME;
		agentRightList->Location.x = offset;
		agentRightList->Location.y = vehicleControl->SelectionSize.y;
		agentRightList->HoverColour = list->HoverColour;
		agentRightList->SelectedColour = list->SelectedColour;
		// set visibility filter
		agentRightList->setFuncIsVisibleItem([](sp<Control> c) {
			auto agent = c->getData<Agent>();
			bool visible = agent->currentVehicle == c->getParent()->getParent()->getData<Vehicle>();
			c->setVisible(visible);
			return visible;
		});
		// set selection behaviour
		agentRightList->setFuncHandleSelection(funcHandleAgentSelection);
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

void AgentAssignment::addBuildingsToList(sp<MultilistBox> list, const int listOffset)
{
	const int offset = 20;
	for (auto b : buildings)
	{
		auto buildingControl = ControlGenerator::createBuildingAssignmentControl(*state, b);
		buildingControl->Size.x -= listOffset;
		buildingControl->SelectionSize.x -= listOffset;
		list->addItem(buildingControl);

		auto vehicleRightList = buildingControl->createChild<MultilistBox>();
		vehicleRightList->Name = VEHICLE_LIST_NAME;
		vehicleRightList->Location.x = offset;
		vehicleRightList->Location.y = buildingControl->SelectionSize.y;
		vehicleRightList->HoverColour = list->HoverColour;
		vehicleRightList->SelectedColour = list->SelectedColour;
		// set visibility filter
		vehicleRightList->setFuncIsVisibleItem([](sp<Control> c) {
			auto vehicle = c->getData<Vehicle>();
			bool visible =
			    vehicle->currentBuilding == c->getParent()->getParent()->getData<Building>();
			c->setVisible(visible);
			return visible;
		});

		addVehiclesToList(vehicleRightList, offset + listOffset);
	}

	list->HoverColour = list->SelectedColour = {0, 0, 0, 0};
}

/**
 * Get selected agents with preservation of order.
 */
std::list<StateRef<Agent>> AgentAssignment::getSelectedAgents() const
{
	std::set<sp<Agent>> agentControlSet;

	auto baseLeftList = findControlTyped<MultilistBox>(LEFT_LIST_NAME);
	for (auto &baseControl : baseLeftList->Controls)
	{
		if (auto leftList = baseControl->findControlTyped<MultilistBox>(AGENT_LIST_NAME))
		{
			for (auto &sel : leftList->getSelectedSet())
			{
				if (auto a = sel->getData<Agent>())
				{
					agentControlSet.insert(a);
				}
			}
		}
	}

	auto baseRightList = findControlTyped<MultilistBox>(RIGHT_LIST_NAME);
	for (auto &baseControl : baseRightList->Controls)
	{
		if (auto rightList = baseControl->findControlTyped<MultilistBox>(VEHICLE_LIST_NAME))
		{
			for (auto &vehicleControl : rightList->Controls)
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
	}
}

void AgentAssignment::update() { Form::update(); }
}