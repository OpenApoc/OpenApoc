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

class BribeScreen : public Stage
{
  private:
	sp<Form> menuform;
	sp<GameState> state;

	sp<Label> labelFunds;
	sp<Label> labelRelation;
	sp<Label> labelOffer;

	StateRef<Organisation> organisation;
	// Sum of the bribe.
	int bribe = 0;

	// Update info about deal.
	void updateInfo();
	// Get the offer of a bribe.
	UString getOfferString(int itWillCost, const UString &newAttitude) const;

  public:
	BribeScreen(sp<GameState> state);
	~BribeScreen() override;
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
