
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

	sp<GameState> state;
	int counter;

  public:
	BaseSelectScreen(sp<GameState> state, Vec3<float> centerPos);
	~BaseSelectScreen() override;
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
