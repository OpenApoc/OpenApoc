
#pragma once

#include "framework/stage.h"

#include "forms/forms.h"

namespace OpenApoc
{

class GameState;

class InGameOptions : public Stage
{
  private:
	sp<Form> menuform;

	sp<GameState> state;

  public:
	InGameOptions(sp<GameState> state);
	~InGameOptions() override;
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
