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

	sp<BitmapFont> labelFont;
	sp<Image> healthImage;
	sp<Image> shieldImage;
	sp<Image> stunImage;
	sp<Image> iconShade;
	std::vector<sp<Image>> unitRanks;
	std::vector<sp<Image>> bigUnitRanks;
	std::vector<sp<Image>> unitSelect;

  public:
	sp<Agent> currentAgent;
	sp<Vehicle> currentVehicle;

	AgentAssignment(sp<GameState> state);

	void init(sp<Form> form, Vec2<int> location, Vec2<int> size);

	sp<Control> createAgentControl(sp<Agent> agent);

	void setLocation(sp<Agent> agent);
	void setLocation(sp<Vehicle> vehicle);
	void setLocation(sp<Building> building);
	void setLocation();
	void updateLocation();

	void update() override;
};

} // namespace OpenApoc
