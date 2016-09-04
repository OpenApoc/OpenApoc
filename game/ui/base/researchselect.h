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
	sp<Lab> lab;

	sp<GameState> state;
	sp<ResearchTopic> current_topic;

	std::map<sp<ResearchTopic>, sp<Control>> control_map;

	void redrawResearchList();
	void populateResearchList();

  public:
	ResearchSelect(sp<GameState> state, sp<Lab> lab);
	~ResearchSelect() override;
	// Stage control
	void begin() override;
	void pause() override;
	void resume() override;
	void finish() override;
	void eventOccurred(Event *e) override;
	void update() override;
	void render() override;
	bool isTransition() override;
};

}; // namespace OpenApoc
