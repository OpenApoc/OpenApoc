
#pragma once

#include "forms/forms.h"
#include "framework/stage.h"
#include "game/ui/tileview/tileview.h"
#include "library/sp.h"

namespace OpenApoc
{

class BaseSelectScreen : public TileView
{
  private:
	static const int COUNTER_MAX = 90;

	sp<Form> menuform;
	StageCmd stageCmd;

	sp<GameState> state;
	int counter;

  public:
	BaseSelectScreen(sp<GameState> state, Vec3<float> centerPos);
	~BaseSelectScreen();
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
