#pragma once

#include "framework/stage.h"

#include "forms/forms.h"

namespace OpenApoc
{

class GameState;

class MessageLogScreen : public Stage
{
  private:
	sp<Form> menuform;
	StageCmd stageCmd;

	sp<GameState> state;

  public:
	MessageLogScreen(sp<GameState> state);
	~MessageLogScreen() override;
	// Stage control
	void begin() override;
	void pause() override;
	void resume() override;
	void finish() override;
	void eventOccurred(Event *e) override;
	void update(StageCmd *const cmd) override;
	void render() override;
	bool isTransition() override;
};
}; // namespace OpenApoc
