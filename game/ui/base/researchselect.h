#pragma once

#include "forms/forms.h"
#include "framework/stage.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/vec.h"

namespace OpenApoc
{

class GameState;
class Lab;
class Base;

class ResearchSelect : public Stage
{
  private:
	sp<Form> form;
	StageCmd stageCmd;
	StateRef<Base> base;
	sp<Lab> lab;

	sp<GameState> state;

	void redrawResearchList();

  public:
	ResearchSelect(sp<GameState> state, StateRef<Base> base, sp<Lab> lab);
	~ResearchSelect();
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
