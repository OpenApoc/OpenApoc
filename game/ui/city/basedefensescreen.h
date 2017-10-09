#pragma once

#include "framework/stage.h"
#include "game/state/stateobject.h"
#include "library/sp.h"

namespace OpenApoc
{

class Form;
class GameState;
class Base;
class Organisation;

class BaseDefenseScreen : public Stage
{
  private:
	sp<Form> menuform;

	sp<GameState> state;
	StateRef<Base> base;
	StateRef<Organisation> attacker;

	void initiateDefenseMission(StateRef<Base> base, StateRef<Organisation> attacker);

  public:
	BaseDefenseScreen(sp<GameState> state, StateRef<Base> base, StateRef<Organisation> attacker);
	~BaseDefenseScreen() override;
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
