#pragma once

#include "framework/stage.h"
#include "game/state/gamestate.h"

#include "forms/forms.h"
#include "game/state/savemanager.h"

namespace OpenApoc
{
enum class SaveMenuAction : unsigned
{
	LoadNewGame = 0,
	Load = 1,
	Save = 2
};

class SaveMenu : public Stage
{
  private:
	sp<TextEdit> activeTextEdit;
	sp<Form> menuform;
	sp<GameState> currentState;
	StageCmd stageCmd;
	SaveMenuAction currentAction;
	SaveManager saveManager;

	void ClearTextEdit(sp<TextEdit> textEdit);
	void BeginEditing(sp<TextEdit> textEdit, sp<TextEdit> activeTextEdit);
	void TryToLoadWithWarning(sp<Control> parent);
	void TryToLoadGame(sp<Control> slotControl);
	void TryToSaveGame(const UString &textEdit, sp<Control> parent);

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
