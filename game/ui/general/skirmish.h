#pragma once

#include "framework/stage.h"
#include "library/sp.h"
#include "game/state/stateobject.h"

namespace OpenApoc
{
class GameState;
class Form;
class Building;
class VehicleType;
class Base;

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

	void battleInBuilding(StateRef<Building> building, bool raid);
	void battleInBase(StateRef<Base> base);
	void battleInVehicle(StateRef<VehicleType>);

  public:
	Skirmish(sp<GameState> state);
	~Skirmish() override;
	
	UString getLocationText();

	void setLocation(StateRef<Building> building);
	void setLocation(StateRef<VehicleType> veh);
	void setLocation(StateRef<Base> base);

	void goToBattle();
	void customizeForces();

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