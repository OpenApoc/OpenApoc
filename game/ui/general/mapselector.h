#pragma once

#include "framework/stage.h"

#include "forms/forms.h"

namespace OpenApoc
{
class GameState;
class EventMessage;
class CityView;
class Building;
class VehicleType;

class MapSelector : public Stage
{
  private:
	sp<Form> menuform;

	sp<GameState> state;

	sp<Control> createMapRowBuilding(sp<Building> building, sp<GameState> state);
	sp<Control> createMapRowVehicle(sp<VehicleType> vehicle, sp<GameState> state);

  public:
	MapSelector(sp<GameState> state);
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
