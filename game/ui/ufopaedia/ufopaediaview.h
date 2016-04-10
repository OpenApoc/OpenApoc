#pragma once

#include "forms/forms.h"
#include "framework/includes.h"
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
	~UfopaediaView();
	// Stage control
	virtual void Begin() override;
	virtual void Pause() override;
	virtual void Resume() override;
	virtual void Finish() override;
	virtual void EventOccurred(Event *e) override;
	virtual void Update(StageCmd *const cmd) override;
	virtual void Render() override;
	virtual bool IsTransition() override;
};
}; // namespace OpenApoc
