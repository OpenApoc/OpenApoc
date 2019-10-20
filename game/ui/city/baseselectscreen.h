#pragma once

#include "framework/stage.h"
#include "game/ui/tileview/citytileview.h"
#include "library/sp.h"

namespace OpenApoc
{

class Form;
class GameState;

class BaseSelectScreen : public CityTileView
{
  private:
	sp<Form> menuform;
	sp<GameState> state;

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
