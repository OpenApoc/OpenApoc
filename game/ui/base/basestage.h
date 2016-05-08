#pragma once
#include "forms/forms.h"
#include "framework/stage.h"
#include "game/ui/base/basegraphics.h"
#include "library/sp.h"
#include <vector>

namespace OpenApoc
{

class Base;
class Facility;
class GameState;

class BaseStage : public Stage
{
  protected:
	sp<Form> form;

	std::vector<sp<GraphicButton>> miniViews;
	sp<Label> textFunds, textViewBase;

	sp<GraphicButton> currentView;
	BaseGraphics::FacilityHighlight viewHighlight;
	sp<Facility> viewFacility;

	sp<GameState> state;
	virtual void ChangeBase(sp<Base> newBase = nullptr);
	void RefreshView();

  public:
	BaseStage(sp<GameState> state);
	~BaseStage();
	// Stage control
	void Begin() override;
	void Render() override;
};

}; // namespace OpenApoc
