#pragma once

#include "game/state/stateobject.h"
#include "game/ui/base/basestage.h"
#include "library/sp.h"
#include "library/vec.h"
#include <list>

namespace OpenApoc
{

class Base;
class GameState;
class Agent;
class Control;
class Image;
class Graphic;

class BuySellScreen : public BaseStage
{
  private:
	void changeBase(sp<Base> newBase) override;

  public:
	BuySellScreen(sp<GameState> state);
	~BuySellScreen() override;
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
