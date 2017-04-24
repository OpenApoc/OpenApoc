#pragma once

#include "framework/stage.h"
#include "game/state/stateobject.h"
#include "library/sp.h"

namespace OpenApoc
{
class GameState;
class Form;
class Building;
class VehicleType;
class Base;
class AgentType;

class Skirmish : public Stage
{
  private:
	sp<Form> menuform;

	GameState &state;

	StateRef<Building> locBuilding;
	StateRef<VehicleType> locVehicle;
	StateRef<Base> locBase;

	void clearLocation();
	void updateLocationLabel();

	void battleInBuilding(bool hotseat, StateRef<Base> playerBase, StateRef<Building> building,
	                      bool raid, std::map<StateRef<AgentType>, int> *aliens, int *guards,
	                      int *civilians);
	void battleInBase(bool hotseat, StateRef<Base> base,
	                  std::map<StateRef<AgentType>, int> *aliens);
	void battleInVehicle(bool hotseat, StateRef<Base> playerBase, StateRef<VehicleType>,
	                     std::map<StateRef<AgentType>, int> *aliens);

  public:
	Skirmish(sp<GameState> state);
	~Skirmish() override;

	UString getLocationText();

	void setLocation(StateRef<Building> building);
	void setLocation(StateRef<VehicleType> veh);
	void setLocation(StateRef<Base> base);

	void goToBattle(std::map<StateRef<AgentType>, int> *aliens = nullptr, int *guards = nullptr,
	                int *civilians = nullptr);
	void customizeForces(bool force = false);

	// Stage control
	void begin() override;
	void pause() override;
	void resume() override;
	void finish() override;
	void eventOccurred(Event *e) override;
	void update() override;
	void render() override;
	bool isTransition() override;
};
}