#pragma once

#include "framework/stage.h"
#include "library/sp.h"

namespace OpenApoc
{

class GameState;
class Building;
class Form;
class AgentAssignment;

class AlertScreen : public Stage
{
  private:
	sp<Form> menuform;
	sp<GameState> state;
	sp<Building> building;
	sp<AgentAssignment> agentAssignment;

  public:
	AlertScreen(sp<GameState> state, sp<Building> building);
	~AlertScreen() override;
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
