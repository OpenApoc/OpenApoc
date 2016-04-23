
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
	sp<Form> menuform;
	StageCmd stageCmd;

	sp<GameState> state;
	StateRef<City> city;

  public:
	BaseSelectScreen(sp<GameState> state, StateRef<City> city, Vec3<float> centerPos);
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
