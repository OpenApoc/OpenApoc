#pragma once

#include "forms/forms.h"
#include "framework/stage.h"
#include "library/sp.h"

namespace OpenApoc
{

class GameState;

class UfopaediaView : public Stage
{
  private:
	sp<Form> menuform;
	StageCmd stageCmd;
	sp<GameState> state;

  public:
	UfopaediaView(sp<GameState> state);
	~UfopaediaView() override;
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
