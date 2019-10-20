#pragma once

#include "game/state/stateobject.h"
#include "game/ui/base/basestage.h"
#include "library/sp.h"
#include "library/vec.h"
#include <vector>

namespace OpenApoc
{

class Base;
class Facility;
class GameState;
class FacilityType;
class Graphic;
class Label;

class BaseScreen : public BaseStage
{
  private:
	static const Vec2<int> NO_SELECTION;

	Vec2<int> selection, mousePos;
	sp<Facility> selFacility;
	StateRef<FacilityType> dragFacility;
	bool drag;

	sp<Graphic> baseView, selGraphic;
	sp<Label> selText;
	std::vector<sp<Label>> statsLabels;
	std::vector<sp<Label>> statsValues;

	void changeBase(sp<Base> newBase) override;
	void renderBase();

  public:
	BaseScreen(sp<GameState> state);
	~BaseScreen() override;
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
