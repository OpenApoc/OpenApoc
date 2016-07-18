
#pragma once

#include "framework/stage.h"
#include "game/state/gamestate.h"

#include "forms/forms.h"
#include "game/state/savemanager.h"

namespace OpenApoc
{
enum SaveMenuAction
{
	Load = 0,
	Save = 1
};

class SaveMenu : public Stage
{
  private:
	sp<Form> menuform;
	sp<GameState> currentState;
	StageCmd stageCmd;
	SaveMenuAction currentAction;
	SaveManager saveManager;

  public:
	SaveMenu(SaveMenuAction saveMenuAction, sp<GameState> gameState);
	~SaveMenu();
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
