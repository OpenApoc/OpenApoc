#pragma once

#include "forms/forms.h"
#include "framework/stage.h"
#include "game/state/stateobject.h"
#include "library/vec.h"
#include <unordered_map>
#include <vector>

namespace OpenApoc
{

class Base;
class Facility;
class GameState;
class Agent;

class ResearchScreen : public Stage
{
  private:
	sp<Form> form;
	StageCmd stageCmd;
	StateRef<Base> base;
	sp<Facility> selected_lab;
	std::list<sp<Facility>> labs;

	sp<GameState> state;

	void setCurrentLabInfo();
	sp<Control> createAgentControl(Vec2<int> size, StateRef<Agent> agent);
	// FIXME: healthImage has a copy in CityView - maybe opportunity to merge?
	sp<Image> healthImage;

	int assigned_agent_count;

  public:
	ResearchScreen(sp<GameState> state, StateRef<Base> base, sp<Facility> selected_lab = nullptr);
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
