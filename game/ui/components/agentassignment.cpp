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
#include "game/state/city/vehicle.h"
#include "game/state/gamestate.h"
#include "game/state/shared/agent.h"
#include "game/state/shared/organisation.h"
#include "game/ui/components/controlgenerator.h"
#include <cmath>

namespace OpenApoc
{

AgentAssignment::AgentAssignment(sp<GameState> state) : Form(), state(state) {}

void AgentAssignment::init(sp<Form> form, Vec2<int> location, Vec2<int> size)
{
	form->copyControlData(shared_from_this());
	Location = location;
	Size = size;

	draggedList = findControlTyped<MultilistBox>("DRAGGED_AGENT_BOX");
	draggedList->setVisible(false);
}

void AgentAssignment::setLocation(sp<Agent> agent)
{
	if (agent->currentBuilding)
	{
		setLocation(agent->currentBuilding);
		return;
	}
	else if (agent->currentVehicle)
	{
		setLocation(agent->currentVehicle);
		return;
	}

	this->agent = agent;
	this->vehicle = nullptr;
	this->building = nullptr;

	updateLocation();
}

void AgentAssignment::setLocation(sp<Vehicle> vehicle)
{
	if (vehicle->currentBuilding)
	{
		setLocation(vehicle->currentBuilding);
		return;
	}

	this->agent = nullptr;
	this->vehicle = vehicle;
	this->building = nullptr;

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
	if (building)
	{
		for (auto &a : state->agents)
		{
			if (a.second->owner != state->getPlayer() ||
			    a.second->type->role != AgentType::Role::Soldier)
			{
				continue;
			}
			if (a.second->currentBuilding == building ||
			    (a.second->currentVehicle && a.second->currentVehicle->currentBuilding == building))
			{
				agents.emplace_back(a.second);
			}
		}
		for (auto &v : state->vehicles)
		{
			if (v.second->owner != state->getPlayer() || v.second->currentBuilding != building)
			{
				continue;
			}
			vehicles.emplace_back(v.second);
		}
	}
	else if (vehicle)
	{
		for (auto &a : state->agents)
		{
			if (a.second->owner != state->getPlayer() ||
			    a.second->type->role != AgentType::Role::Soldier ||
			    a.second->currentVehicle != vehicle)
			{
				continue;
			}
			agents.emplace_back(a.second);
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
			if (a.second->owner != state->getPlayer() ||
			    a.second->type->role != AgentType::Role::Soldier)
			{
				continue;
			}
			agents.emplace_back(a.second);
		}
		for (auto &v : state->vehicles)
		{
			if (v.second->owner != state->getPlayer())
			{
				continue;
			}
			vehicles.emplace_back(v.second);
		}
	}

	// TODO: change icon; check the SelectionSize; refactor createLargeAgentControl; prepare
	// structure in the init()
	this->vehicleList.clear();
	this->agentGroupList.clear();

	// Left list
	auto baseLeftList = findControlTyped<MultilistBox>("AGENT_SELECT_BOX");
	baseLeftList->clear();

	auto owner = state->getPlayer();
	auto ownerControl1 = ControlGenerator::createOrganisationControl(*state, owner);
	ownerControl1->SelectionSize = {140, ownerControl1->Size.y};
	baseLeftList->addItem(ownerControl1);

	auto agentLeftList = ownerControl1->createChild<MultilistBox>();
	agentLeftList->Name = "AGENT_LEFT_LIST_0";
	agentLeftList->Location.x = 20;
	agentLeftList->Location.y = ownerControl1->Size.y;
	agentLeftList->HoverColour = baseLeftList->HoverColour;
	agentLeftList->SelectedColour = baseLeftList->SelectedColour;

	this->agentGroupList.push_back(agentLeftList);

	for (auto &agent : agents)
	{
		if (agent->currentVehicle)
		{
			continue;
		}
		auto agentControl = ControlGenerator::createLargeAgentControl(*state, agent);
		agentControl->Size = {140, 30};
		agentControl->SelectionSize = {140, 30};
		agentLeftList->addItem(agentControl);
	}

	// Right list
	auto baseRightList = findControlTyped<MultilistBox>("VEHICLE_SELECT_BOX");
	baseRightList->clear();

	// TODO: change icon; check the SelectionSize
	// auto owner = state->getPlayer();
	auto ownerControl2 = ControlGenerator::createOrganisationControl(*state, owner);
	ownerControl2->SelectionSize = {140, ownerControl2->Size.y};
	baseRightList->addItem(ownerControl2);

	auto vehicleRightList = ownerControl2->createChild<MultilistBox>();
	vehicleRightList->Name = "VEHICLE_RIGHT_LIST_0";
	vehicleRightList->Location.x = 20;
	vehicleRightList->Location.y = ownerControl2->Size.y;
	vehicleRightList->HoverColour = baseRightList->HoverColour;
	vehicleRightList->SelectedColour = baseRightList->SelectedColour;

	int i = 0;
	for (auto &vehicle : vehicles)
	{
		auto vehicleControl = ControlGenerator::createVehicleLargeControl(*state, vehicle);
		vehicleRightList->addItem(vehicleControl);

		this->vehicleList.push_back(vehicleControl);

		// add an agent list to each vehicle
		auto agentRightList = vehicleControl->createChild<MultilistBox>();
		agentRightList->Name = format("AGENT_RIGHT_LIST_%d", i++);
		agentRightList->Location.x = 20;
		agentRightList->Location.y = vehicleControl->Size.y;
		agentRightList->HoverColour = baseRightList->HoverColour;
		agentRightList->SelectedColour = baseRightList->SelectedColour;

		this->agentGroupList.push_back(agentRightList);

		for (auto &agent : agents)
		{
			if (agent->currentVehicle == vehicle)
			{
				auto agentControl = ControlGenerator::createLargeAgentControl(*state, agent);
				agentControl->Size = {140, 30};
				agentControl->SelectionSize = {140, 30};
				agentRightList->addItem(agentControl);
			}
		}
	}
}

void AgentAssignment::updateControl(sp<Agent> agent)
{
	// auto agentList = findControlTyped<MultilistBox>("AGENT_SELECT_BOX");
	// auto agentControl = ControlGenerator::createAgentControl(*state, agent);
	// agentControl->Size = {agentList->Size.x, ControlGenerator::getFontHeight(*state) * 2};
	// agentList->replaceItem(agentControl);
}

void AgentAssignment::updateControl(sp<Vehicle> vehicle)
{
	// auto vehicleList = findControlTyped<MultilistBox>("VEHICLE_SELECT_BOX");
	// auto vehicleControl = ControlGenerator::createVehicleLargeControl(*state, vehicle);
	// vehicleControl->Size = {vehicleList->Size.x, ControlGenerator::getFontHeight(*state) * 2};
	// vehicleList->replaceItem(vehicleControl);
}

void AgentAssignment::eventOccured(Event *e)
{
	Form::eventOccured(e);

	switch (e->type())
	{
		case EVENT_FORM_INTERACTION:
			switch (e->forms().EventFlag)
			{
				case FormEventType::MouseDown: // TODO: move to callback
					for (auto list : agentGroupList)
					{
						if (e->forms().RaisedBy == list)
						{
							sourceRaisedBy = list;

							auto draggedAgents(list->getSelectedItems());
							isDragged = !draggedAgents.empty();

							if (isDragged)
							{
								positionX = e->mouse().X;
								positionY = e->mouse().Y;
								draggedList->setVisible(false);
								draggedList->clear();
								for (auto &a : draggedAgents)
								{
									a->copyTo(draggedList)
									    ->setData(a->getData<Agent>()); // TODO: copy only once
								}
							}
							break;
						}
					}
					break;

				case FormEventType::MouseUp: // TODO: may be move to callback
					if (isDragged)
					{
						if (e->forms().RaisedBy == sourceRaisedBy ||
						    e->forms().RaisedBy->Name == "FORM_AGENT_ASSIGNMENT")
						{
							// miss click
							isDragged = false;
							draggedList->setVisible(false);
							break;
						}

						bool doUpdate = false;
						for (auto vehicle : vehicleList)
						{
							if (e->forms().RaisedBy == vehicle)
							{
								for (auto agent : draggedList->Controls)
								{
									LogWarning(format("\n-- agent name:%s vehicle name:%s",
									                  agent->getData<Agent>()->name,
									                  vehicle->getData<Vehicle>()->name));
									agent->getData<Agent>()->enterVehicle(
									    *state, {state.get(), vehicle->getData<Vehicle>()});
									// TODO: add agents to vehicle's agent list here and remove from
									// sourceRaisedBy
								}
								doUpdate = true;
								break;
							}
						}

						if (e->forms().RaisedBy == agentGroupList.front())
						{
							for (auto agent : draggedList->Controls)
							{
								agent->getData<Agent>()->enterBuilding(
								    *state,
								    {state.get(),
								     vehicleList.front()->getData<Vehicle>()->currentBuilding});
								// TODO: add agents to building's list here and remove from
								// sourceRaisedBy
							}
							doUpdate = true;
						}

						if (doUpdate)
						{
							isDragged = false;
							draggedList->setVisible(false);
							updateLocation(); // TODO: remove
						}
					}
					break;
			}
			break;

		case EVENT_MOUSE_MOVE:
			// if dragged - change location of dragged list
			if (isDragged)
			{
				int distance = (positionX - e->mouse().X) * (positionX - e->mouse().X) +
				               (positionX - e->mouse().Y) * (positionX - e->mouse().Y);
				if (distance > 200)
				{
					draggedList->Location.x = e->mouse().X - this->resolvedLocation.x;
					draggedList->Location.y = e->mouse().Y - this->resolvedLocation.y;
					draggedList->setVisible(true); // TODO: set true only once (& copy too)
				}
			}
			break;
	}
}

void AgentAssignment::update() { Form::update(); }
}