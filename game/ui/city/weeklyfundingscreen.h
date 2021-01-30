#pragma once

#include "framework/stage.h"
#include "game/state/stateobject.h"
#include "library/sp.h"

namespace OpenApoc
{

class GameState;
class Form;
class Label;

class WeeklyFundingScreen : public Stage
{
  private:
	sp<Form> menuform;
	sp<GameState> state;

	sp<Label> labelCurrentIncome;
	sp<Label> labelRatingDescription;
	sp<Label> labelAdjustment;
	sp<Label> labelNextWeekIncome;

  public:
	WeeklyFundingScreen(sp<GameState> state);
	~WeeklyFundingScreen() override;
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
