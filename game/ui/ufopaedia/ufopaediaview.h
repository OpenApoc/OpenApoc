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
