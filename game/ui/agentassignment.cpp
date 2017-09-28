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
    : Form(), state(state), labelFont(ui().getFont("smalfont"))
{
	auto img = mksp<RGBImage>(Vec2<int>{1, 2});
	{
		RGBImageLock l(img);
		l.set({0, 0}, Colour{255, 255, 219});
		l.set({0, 1}, Colour{215, 0, 0});
	}
	this->healthImage = img;
	img = mksp<RGBImage>(Vec2<int>{1, 2});
	{
		RGBImageLock l(img);
		l.set({0, 0}, Colour{160, 236, 252});
		l.set({0, 1}, Colour{4, 100, 252});
	}
	this->shieldImage = img;
	img = mksp<RGBImage>(Vec2<int>{1, 2});
	{
		RGBImageLock l(img);
		l.set({0, 0}, Colour{150, 150, 150});
		l.set({0, 1}, Colour{97, 101, 105});
	}
	this->stunImage = img;
	this->iconShade = fw().data->loadImage("battle/battle-icon-shade.png");
	for (int i = 28; i <= 34; i++)
	{
		unitRanks.push_back(
		    fw().data->loadImage(format("PCK:xcom3/tacdata/tacbut.pck:xcom3/tacdata/"
		                                "tacbut.tab:%d:xcom3/tacdata/tactical.pal",
		                                i)));
	}
	for (int i = 12; i <= 18; i++)
	{
		bigUnitRanks.push_back(
		    fw().data->loadImage(format("PCK:xcom3/tacdata/tacbut.pck:xcom3/tacdata/"
		                                "tacbut.tab:%d:xcom3/tacdata/tactical.pal",
		                                i)));
	}
	unitSelect.push_back(fw().data->loadImage(
	    "PCK:xcom3/ufodata/vs_icon.pck:xcom3/ufodata/vs_icon.tab:37:xcom3/ufodata/pal_01.dat"));
	unitSelect.push_back(fw().data->loadImage("battle/battle-icon-38.png"));
	unitSelect.push_back(fw().data->loadImage("battle/battle-icon-39.png"));
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
		auto agentControl = this->createAgentControl(agent);
		agentList->addItem(agentControl);
		if (agent == currentAgent)
		{
			agentList->setSelected(agentControl);
		}
	}
	agentList->ItemSize = labelFont->getFontHeight() * 2;

	auto vehicleList = findControlTyped<ListBox>("VEHICLE_SELECT_BOX");
	vehicleList->clear();
	if (building)
	{
		vehicleList->addItem(mksp<Graphic>(unitSelect[0]));
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
	vehicleList->ItemSize = labelFont->getFontHeight() * 2;
}

void AgentAssignment::update() { Form::update(); }

sp<Control> AgentAssignment::createAgentControl(sp<Agent> agent)
{
	Vec2<int> size = {130, labelFont->getFontHeight() * 2};

	auto baseControl = mksp<Control>();
	baseControl->setData(agent);
	baseControl->Name = "AGENT_PORTRAIT";
	baseControl->Size = size;

	auto frameGraphic = baseControl->createChild<Graphic>(unitSelect[0]);
	frameGraphic->AutoSize = true;
	frameGraphic->Location = {0, 0};
	auto photoGraphic = frameGraphic->createChild<Graphic>(agent->getPortrait().icon);
	photoGraphic->AutoSize = true;
	photoGraphic->Location = {1, 1};

	// TODO: Fade portraits
	bool faded = false;

	if (faded)
	{
		auto fadeIcon = baseControl->createChild<Graphic>(iconShade);
		fadeIcon->AutoSize = true;
		fadeIcon->Location = {2, 1};
	}

	auto rankIcon = baseControl->createChild<Graphic>(unitRanks[(int)agent->rank]);
	rankIcon->AutoSize = true;
	rankIcon->Location = {0, 0};

	bool shield = agent->getMaxShield() > 0;

	float maxHealth;
	float currentHealth;
	float stunProportion = 0.0f;
	if (shield)
	{
		currentHealth = agent->getShield();
		maxHealth = agent->getMaxShield();
	}
	else
	{
		currentHealth = agent->getHealth();
		maxHealth = agent->getMaxHealth();
	}
	float healthProportion = maxHealth == 0.0f ? 0.0f : currentHealth / maxHealth;
	stunProportion = clamp(stunProportion, 0.0f, healthProportion);

	if (healthProportion > 0.0f)
	{
		// FIXME: Put these somewhere slightly less magic?
		Vec2<int> healthBarOffset = {27, 2};
		Vec2<int> healthBarSize = {3, 20};

		auto healthImg = shield ? this->shieldImage : this->healthImage;
		auto healthGraphic = frameGraphic->createChild<Graphic>(healthImg);
		// This is a bit annoying as the health bar starts at the bottom, but the coord origin is
		// top-left, so fix that up a bit
		int healthBarHeight = (int)((float)healthBarSize.y * healthProportion);
		healthBarOffset.y = healthBarOffset.y + (healthBarSize.y - healthBarHeight);
		healthBarSize.y = healthBarHeight;
		healthGraphic->Location = healthBarOffset;
		healthGraphic->Size = healthBarSize;
		healthGraphic->ImagePosition = FillMethod::Stretch;
	}
	if (stunProportion > 0.0f)
	{
		// FIXME: Put these somewhere slightly less magic?
		Vec2<int> healthBarOffset = {27, 2};
		Vec2<int> healthBarSize = {3, 20};

		auto healthImg = this->stunImage;
		auto healthGraphic = frameGraphic->createChild<Graphic>(healthImg);
		// This is a bit annoying as the health bar starts at the bottom, but the coord origin is
		// top-left, so fix that up a bit
		int healthBarHeight = (int)((float)healthBarSize.y * stunProportion);
		healthBarOffset.y = healthBarOffset.y + (healthBarSize.y - healthBarHeight);
		healthBarSize.y = healthBarHeight;
		healthGraphic->Location = healthBarOffset;
		healthGraphic->Size = healthBarSize;
		healthGraphic->ImagePosition = FillMethod::Stretch;
	}

	auto nameLabel = baseControl->createChild<Label>(agent->name, labelFont);
	nameLabel->Location = {40, 0};
	nameLabel->Size = {100, labelFont->getFontHeight() * 2};

	return baseControl;
}

}