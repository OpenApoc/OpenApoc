#define _USE_MATH_DEFINES

#include "game/ui/agentassignment.h"
#include "game/ui/controlgenerator.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/label.h"
#include "forms/list.h"
#include "forms/ui.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/font.h"
#include "framework/framework.h"
#include "framework/renderer.h"
#include "game/state/agent.h"
#include "game/state/city/building.h"
#include "game/state/city/vehicle.h"
#include "game/state/gamestate.h"
#include <cmath>

namespace OpenApoc
{

AgentAssignment::AgentAssignment(sp<GameState> state)
    : Form(), state(state)
{
}

void AgentAssignment::init(sp<Form> form, Vec2<int> location, Vec2<int> size)
{
	form->copyControlData(shared_from_this());
	Location = location;
	Size = size;

	auto agentList = findControlTyped<ListBox>("AGENT_SELECT_BOX");
	agentList->addCallback(FormEventType::ListBoxChangeSelected, [this](FormsEvent *e) {
		auto list = std::static_pointer_cast<ListBox>(e->forms().RaisedBy);
		auto agent = list->getSelectedData<Agent>();
		if (!agent)
		{
			LogError("No agent in selected data");
			return;
		}
		this->currentAgent = agent;
		if (currentVehicle && currentAgent->currentVehicle != currentVehicle)
		{
			currentAgent->enterVehicle(*this->state, {this->state.get(), currentVehicle});
			updateLocation();
		}
		else if (this->building)
		{
			currentAgent->enterBuilding(*this->state, {this->state.get(), this->building});
			updateLocation();
		}
	});
	auto vehicleList = findControlTyped<ListBox>("VEHICLE_SELECT_BOX");
	vehicleList->addCallback(FormEventType::ListBoxChangeSelected, [this](FormsEvent *e) {
		auto list = std::static_pointer_cast<ListBox>(e->forms().RaisedBy);
		auto vehicle = list->getSelectedData<Vehicle>();
		this->currentVehicle = vehicle;
	});
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

	agent = nullptr;
	vehicle = nullptr;
	building = nullptr;

	this->agent = agent;
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
	this->vehicle = nullptr;
	this->building = nullptr;

	this->vehicle = vehicle;
	updateLocation();
}

void AgentAssignment::setLocation(sp<Building> building)
{
	this->agent = nullptr;
	this->vehicle = nullptr;
	this->building = nullptr;

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
	std::list<sp<Agent>> agents;
	std::list<sp<Vehicle>> vehicles;

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

	auto agentList = findControlTyped<ListBox>("AGENT_SELECT_BOX");
	agentList->clear();
	auto owner = state->getPlayer();
	for (auto &agent : agents)
	{
		auto agentControl = ControlGenerator::createAgentControl(*state, agent);
		agentList->addItem(agentControl);
		if (agent == currentAgent)
		{
			agentList->setSelected(agentControl);
		}
	}
	agentList->ItemSize = ControlGenerator::getFontHeight(*state) * 2;

	auto vehicleList = findControlTyped<ListBox>("VEHICLE_SELECT_BOX");
	vehicleList->clear();
	if (building)
	{
		vehicleList->addItem(mksp<Graphic>());
	}
	for (auto &vehicle : vehicles)
	{
		auto vehicleControl = ControlGenerator::createVehicleControl(*state, vehicle);
		vehicleList->addItem(vehicleControl);
		if (vehicle == currentVehicle)
		{
			vehicleList->setSelected(vehicleControl);
		}
	}
	vehicleList->ItemSize = ControlGenerator::getFontHeight(*state) * 2;
}

void AgentAssignment::update() 
{ 
	Form::update(); 
}

}