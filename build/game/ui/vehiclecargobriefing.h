#pragma once

#include "framework/stage.h"
#include "library/sp.h"
#include <functional>
#include <game/state/city/vehicle.h>

namespace OpenApoc
{

class GameState;
class Form;
class CityView;
//class EventMessage;
//class BattleView;
//class Control;
//class FormsEvent;

class Vehiclecargobriefing : public Stage
{
  private:
	sp<Form> menuform;
	sp<GameState> state;
	//sp<Control> createCargoRow(sp<GameState> state, CityView &cityView);

  public:
	Vehiclecargobriefing(sp<Vehicle> vehicle);
	Vehiclecargobriefing(sp<GameState> state, sp<Vehicle> vehicle);
	~Vehiclecargobriefing() override;

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
