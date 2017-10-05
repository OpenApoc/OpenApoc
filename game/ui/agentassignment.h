#pragma once

#include "forms/form.h"
#include <array>
#include <set>

namespace OpenApoc
{

class Building;
class Agent;
class Vehicle;
class GameState;
class BitmapFont;
class Image;

class AgentAssignment : public Form
{
  private:
	sp<Agent> agent;
	sp<Vehicle> vehicle;
	sp<Building> building;
	sp<GameState> state;

  public:
	std::list<sp<Agent>> agents;
	std::list<sp<Vehicle>> vehicles;

	sp<Agent> currentAgent;
	sp<Vehicle> currentVehicle;

	AgentAssignment(sp<GameState> state);

	void init(sp<Form> form, Vec2<int> location, Vec2<int> size);

	void setLocation(sp<Agent> agent);
	void setLocation(sp<Vehicle> vehicle);
	void setLocation(sp<Building> building);
	void setLocation();
	void updateLocation();

	void updateControl(sp<Agent> agent);
	void updateControl(sp<Vehicle> vehicle);

	void update() override;
};

} // namespace OpenApoc
