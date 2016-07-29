#pragma once

#include "framework/stage.h"
#include "game/state/gamestate.h"

#include "forms/forms.h"
#include "game/state/savemanager.h"

namespace OpenApoc
{
enum class SaveMenuAction : unsigned
{
	// load without prompt to leave game
	LoadNewGame = 0,
	Load = 100,
	Save = 200,
	Delete = 300
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

	void clearTextEdit(sp<TextEdit> textEdit);
	void beginEditing(sp<TextEdit> textEdit, sp<TextEdit> activeTextEdit);

	
	void loadWithWarning(sp<Control> parent);
	// these functions will display prompt to execute action
	void tryToLoadGame(sp<Control> slotControl);
	void tryToSaveGame(const UString &textEdit, sp<Control> parent);
	void tryToDeleteSavedGame(sp<Control>& control);

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
