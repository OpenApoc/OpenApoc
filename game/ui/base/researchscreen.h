#pragma once

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
class Control;
class Image;
class ResearchTopic;
class Graphic;

class ResearchScreen : public BaseStage
{
  private:
	StateRef<ResearchTopic> current_topic;
	std::list<sp<Facility>> smallLabs;
	std::list<sp<Facility>> largeLabs;

	// Populating the UI lab list.
	void populateUILabList(const UString &listName, std::list<sp<Facility>> &list);
	void setCurrentLabInfo();
	void updateProgressInfo();

	int assigned_agent_count;

	sp<Graphic> arrow;

	void changeBase(sp<Base> newBase) override;

  public:
	ResearchScreen(sp<GameState> state, sp<Facility> selectedLab = nullptr);
	~ResearchScreen() override;
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
