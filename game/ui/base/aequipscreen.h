#pragma once

#include "framework/stage.h"
#include "library/sp.h"
#include <map>

namespace OpenApoc
{

class Form;
class GameState;
class Palette;
class Agent;

class AEquipScreen : public Stage
{
  private:
	sp<Form> form;
	sp<Palette> pal;
	sp<GameState> state;
	sp<Agent> currentAgent;

  public:
	AEquipScreen(sp<GameState> state);
	~AEquipScreen() override;

	void begin() override;
	void pause() override;
	void resume() override;
	void finish() override;
	void eventOccurred(Event *e) override;
	void update() override;
	void render() override;
	bool isTransition() override;

	void setSelectedAgent(sp<Agent> agent);
};

} // namespace OpenApoc
