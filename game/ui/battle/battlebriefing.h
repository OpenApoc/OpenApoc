#pragma once
#include "forms/forms.h"
#include "framework/stage.h"
#include "library/sp.h"
#include <future>

namespace OpenApoc
{

class GameState;

class BattleBriefing : public Stage
{
  private:
	sp<Form> menuform;
	std::future<void> loading_task;

	sp<GameState> state;

  public:
	BattleBriefing(sp<GameState> state, std::future<void> loadingTask);
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
