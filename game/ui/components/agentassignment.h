#pragma once

#include "forms/form.h"
#include "game/state/stateobject.h"
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

	Colour HoverColour, SelectedColour;
	Vec2<int> renderOffset{32, 0};

	// List of dragged agents.
	sp<MultilistBox> draggedList;
	// The agents MultilistBox which selected agents was taken from.
	sp<MultilistBox> sourceRaisedBy;
	// Handle the mouse's drag&drop sensibility.
	int positionX = 0, positionY = 0, insensibility = 5 * 5;
	// State of dragged action.
	bool isDragged = false;
	// Update the vehicle's icon
	std::function<void(sp<Control>)> funcVehicleUpdate;
	// Update the agent's icon
	std::function<void(sp<Control>)> funcAgentUpdate;
	// Select/deselect individual agent
	std::function<bool(Event *, sp<Control>, bool)> funcHandleAgentSelection;
	// Select/deselect agents inside vehicle
	std::function<bool(Event *, sp<Control>, bool)> funcHandleVehicleSelection;
	// Selection render
	std::function<void(sp<Control>)> funcSelectionItemRender;
	// Hover render
	std::function<void(sp<Control>)> funcHoverItemRender;

	void addAgentsToList(sp<MultilistBox> list, const int listOffset);

	void addVehiclesToList(sp<MultilistBox> list, const int listOffset);

	void addBuildingToRightList(sp<Building> building, sp<MultilistBox> list, const int listOffset);

  public:
	static const UString AGENT_SELECT_BOX;
	static const UString AGENT_LIST_NAME;
	static const UString VEHICLE_LIST_NAME;

	std::list<sp<Agent>> agents;
	std::list<sp<Vehicle>> vehicles;
	std::list<sp<Building>> buildings;

	sp<Agent> currentAgent;
	sp<Vehicle> currentVehicle;

	AgentAssignment(sp<GameState> state);

	void init(sp<Form> form, Vec2<int> location, Vec2<int> size);

	// Call when selected agent.
	void setLocation(sp<Agent> agent);
	// Call when selected vehicle.
	void setLocation(sp<Vehicle> vehicle);
	// Call when selected building.
	void setLocation(sp<Building> building);
	// Call when the alien incident happens.
	void setLocation();
	void updateLocation();
	// Get selected agents with preservation of order.
	std::list<StateRef<Agent>> getSelectedAgents() const;
	// Get selected vehicles with preservation of order.
	std::list<StateRef<Vehicle>> getSelectedVehicles() const;

	void eventOccured(Event *e) override;
	void update() override;
};

} // namespace OpenApoc
