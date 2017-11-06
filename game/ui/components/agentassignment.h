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
class MultilistBox;

class AgentAssignment : public Form
{
  private:
	sp<Agent> agent;
	sp<Vehicle> vehicle;
	sp<Building> building;
	sp<GameState> state;

	// List of dragged agents.
	sp<MultilistBox> draggedList;
	// Cache of vehile's controls.
	std::vector<sp<Control>> vehicleList;
	// Cache of agent's lists.
	std::vector<sp<MultilistBox>> agentGroupList;
	// The agents MultilistBox which selected agents was taken from.
	sp<MultilistBox> sourceRaisedBy;
	// For handle the mouse's drag&drop sensibility.
	int positionX = 0, positionY = 0;
	// State of dragged action.
	bool isDragged = false;

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

	void eventOccured(Event *e) override;
	void update() override;
};

} // namespace OpenApoc
