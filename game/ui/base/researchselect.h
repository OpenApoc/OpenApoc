#pragma once

#include "forms/forms.h"
#include "framework/stage.h"
#include "game/state/research.h"
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
	sp<Lab> lab;

	sp<GameState> state;
	sp<ResearchTopic> current_topic;

	std::map<sp<ResearchTopic>, sp<Control>> control_map;

	void redrawResearchList();
	void populateResearchList();

  public:
	ResearchSelect(sp<GameState> state, sp<Lab> lab);
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
