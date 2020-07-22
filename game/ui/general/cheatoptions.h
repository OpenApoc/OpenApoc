#pragma once

#include "framework/stage.h"
#include "library/sp.h"

namespace OpenApoc
{

class GameState;
class Form;

class CheatOptions : public Stage
{
  private:
	sp<Form> menuform;
	sp<GameState> state;

  public:
	CheatOptions(sp<GameState> state);
	~CheatOptions() override;

	// Stage control
	void begin() override;
	void pause() override;
	void resume() override;
	void finish() override;
	void eventOccurred(Event *e) override;
	void update() override;
	void render() override;
	bool isTransition() override;

  protected:
	// scrollbars store ints but multipliers are float so we provide methods to scale the values
	int scaleMultiplierToScrollbar(float multiplierValue, float multMin, float multMax,
	                               int scrollMin, int scrollMax);
	float scaleScrollbarToMultiplier(int scrollbarValue, float multMin, float multMax,
	                                 int scrollMin, int scrollMax);
	void updateMultiplierText(UString controlName, float multMin, float multMax);
};
} // namespace OpenApoc
