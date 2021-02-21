#pragma once

#include "framework/stage.h"
#include "game/state/stateobject.h"
#include "library/sp.h"

namespace OpenApoc
{

class GameState;
class Form;
class Label;
class Organisation;

class DiplomaticTreatyScreen : public Stage
{
  private:
	sp<Form> menuform;
	sp<GameState> state;

	sp<Label> labelOffer;
	sp<Label> labelBribe;

	StateRef<Organisation> organisation;
	int bribeAmount = 0;

  public:
	DiplomaticTreatyScreen(sp<GameState> state, StateRef<Organisation> org);
	~DiplomaticTreatyScreen() override;
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
