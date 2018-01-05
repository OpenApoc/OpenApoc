#pragma once

#include "framework/stage.h"
#include "library/sp.h"

namespace OpenApoc
{

class GameState;
class Form;
class Label;

class ScoreScreen : public Stage
{
  private:
	sp<Form> menuform;
	sp<Form> formScore;
	sp<Form> formFinance;
	sp<Label> title;

	sp<GameState> state;

	// The form filling status.
	bool formScoreFilled = false;
	bool formFinanceFilled = false;

	// Setup the score mode.
	void setScoreMode();
	// Setup the finance mode.
	void setFinanceMode();

  public:
	ScoreScreen(sp<GameState> state);
	~ScoreScreen() override;

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
