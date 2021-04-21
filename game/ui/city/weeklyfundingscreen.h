#pragma once

#include "framework/stage.h"
#include "game/state/stateobject.h"
#include "game/state/playerstatesnapshot.h"
#include "library/sp.h"
#include "library/colour.h"

namespace OpenApoc
{

class GameState;
class Form;
class Label;

class WeeklyFundingScreen : public Stage
{
  private:
	sp<Form> menuform;
	PlayerStateSnapshot state;

	sp<Label> labelCurrentIncome;
	sp<Label> valueCurrentIncome;

	sp<Label> labelRatingDescription;

	sp<Label> labelAdjustment;
	sp<Label> valueAdjustment;

	sp<Label> labelNextWeekIncome;
	sp<Label> valueNextWeekIncome;

	void setLabel(sp<Label> label, UString text, const Colour color);
	void setValueField(sp<Label> valueField, sp<Label> labelField, unsigned int amount,
	                  const Colour color);

  public:
	WeeklyFundingScreen(PlayerStateSnapshot state);
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
