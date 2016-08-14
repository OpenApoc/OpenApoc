#pragma once
#include "forms/forms.h"
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

class BaseScreen : public BaseStage
{
  private:
	static const Vec2<int> NO_SELECTION;

	StageCmd stageCmd;

	Vec2<int> selection, mousePos;
	sp<Facility> selFacility;
	StateRef<FacilityType> dragFacility;
	bool drag;

	sp<Graphic> baseView, selGraphic;
	sp<Label> selText;
	std::vector<sp<Label>> statsLabels;
	std::vector<sp<Label>> statsValues;

	void ChangeBase(sp<Base> newBase) override;
	void RenderBase();

  public:
	BaseScreen(sp<GameState> state);
	~BaseScreen();
	// Stage control
	void Begin() override;
	void Pause() override;
	void Resume() override;
	void Finish() override;
	void EventOccurred(Event *e) override;
	void Update(StageCmd *const cmd) override;
	void Render() override;
	bool IsTransition() override;
};

}; // namespace OpenApoc
