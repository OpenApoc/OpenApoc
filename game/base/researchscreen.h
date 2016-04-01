#pragma once

#include "forms/forms.h"
#include "framework/stage.h"
#include "game/gamestate.h"
#include "library/vec.h"

#include <unordered_map>
#include <vector>

namespace OpenApoc
{

class Base;
class Facility;
class GameState;

class ResearchScreen : public Stage
{
  private:
	sp<Form> form;
	StageCmd stageCmd;
	StateRef<Base> base;
	StateRef<Facility> selected_lab;
	std::list<StateRef<Facility>> labs;

	sp<GameState> state;

  public:
	ResearchScreen(sp<GameState> state, StateRef<Base> base, StateRef<Facility> selected_lab = {});
	~ResearchScreen();
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
