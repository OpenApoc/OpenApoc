#pragma once

#include "framework/stage.h"
#include "game/state/stateobject.h"
#include "library/sp.h"

namespace OpenApoc
{
class GameState;
class EventMessage;
class CityView;
class Building;
class VehicleType;
class Form;
class Skirmish;
class Control;
class Base;

class MapSelector : public Stage
{
  private:
	sp<Form> menuform;

	Skirmish &skirmish;

	sp<Control> createMapRowBuilding(StateRef<Building> building, sp<GameState> state);
	sp<Control> createMapRowVehicle(StateRef<VehicleType> vehicle, sp<GameState> state);
	sp<Control> createMapRowBase(StateRef<Base> base, sp<GameState> state);

  public:
	MapSelector(sp<GameState> state, Skirmish &skirmish);
	~MapSelector() override;

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
}; // namespace OpenApoc
