#pragma once

#include "forms/forms.h"
#include "framework/stage.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/vec.h"
#include <list>

namespace OpenApoc
{

class Base;
class Facility;
class GameState;
class ResearchTopic;
class Agent;

class ResearchScreen : public Stage
{
  private:
	sp<Form> form;
	StageCmd stageCmd;
	StateRef<Base> base;
	sp<Facility> selected_lab;
	StateRef<ResearchTopic> current_topic;
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
