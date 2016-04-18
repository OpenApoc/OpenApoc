
#pragma once

#include "framework/stage.h"

#include "forms/forms.h"

namespace OpenApoc
{

class Building;
class Base;
class GameState;

class BaseBuyScreen : public Stage
{
  private:
	sp<Form> form;
	sp<Graphic> baseView;
	StageCmd stageCmd;
	int price;

	sp<GameState> state;
	sp<Base> base;
	void RenderBase();

  public:
	BaseBuyScreen(sp<GameState> state, sp<Building> building);
	~BaseBuyScreen();
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
