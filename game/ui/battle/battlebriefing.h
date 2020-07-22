#pragma once

#include "framework/stage.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include <future>

namespace OpenApoc
{

class GameState;
class Organisation;
class Form;

class BattleBriefing : public Stage
{
  private:
	sp<Form> menuform;
	std::shared_future<void> loading_task;

	sp<GameState> state;

  public:
	BattleBriefing(sp<GameState> state, StateRef<Organisation> targetOrg, UString location,
	               bool isBuilding, bool isRaid, std::shared_future<void> loadingTask);
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
