#pragma once

#include "forms/forms.h"
#include "game/state/stateobject.h"
#include "game/ui/base/basestage.h"
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

class ResearchScreen : public BaseStage
{
  private:
	StageCmd stageCmd;
	sp<Facility> selected_lab;
	StateRef<ResearchTopic> current_topic;
	std::list<sp<Facility>> labs;

	void setCurrentLabInfo();
	void updateProgressInfo();
	sp<Control> createAgentControl(Vec2<int> size, StateRef<Agent> agent);
	// FIXME: healthImage has a copy in CityView - maybe opportunity to merge?
	sp<Image> healthImage;

	int assigned_agent_count;

	sp<Graphic> arrow;

	void changeBase(sp<Base> newBase) override;

  public:
	ResearchScreen(sp<GameState> state, sp<Facility> selected_lab = nullptr);
	~ResearchScreen() override;
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
