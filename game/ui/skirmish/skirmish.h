#pragma once

#include "framework/stage.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include <functional>

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

	std::function<void()> loadBattle = 0;

	void clearLocation();
	void updateLocationLabel();

	void battleInBuilding(bool hotseat, StateRef<Base> playerBase, StateRef<Building> building,
	                      bool raid, bool customAliens, std::map<StateRef<AgentType>, int> aliens,
	                      bool customGuards, int guards, bool customCivilians, int civilians,
	                      int score);
	void battleInBase(bool hotseat, StateRef<Base> base, bool customAliens,
	                  std::map<StateRef<AgentType>, int> aliens, int score);
	void battleInVehicle(bool hotseat, StateRef<Base> playerBase, StateRef<VehicleType>,
	                     bool customAliens, std::map<StateRef<AgentType>, int> aliens, int score);

  public:
	Skirmish(sp<GameState> state);
	~Skirmish() override;

	UString getLocationText();

	void setLocation(StateRef<Building> building);
	void setLocation(StateRef<VehicleType> veh);
	void setLocation(StateRef<Base> base);

	void goToBattle(bool customAliens = false, std::map<StateRef<AgentType>, int> aliens = {},
	                bool customGuards = false, int guards = 0, bool customCivilians = false,
	                int civilians = 0);
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
} // namespace OpenApoc