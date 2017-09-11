#pragma once

#include "framework/stage.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/vec.h"
#include <set>

namespace OpenApoc
{

class GameState;
class Form;
class Image;
class Agent;
class Control;

class BattlePreStart : public Stage
{
  private:
	const int SHIFT_X = 45;
	const int SHIFT_Y = 61;
	const int ROW_HEIGHT = 30;
	const int ROW_WIDTH = 260;
	const int ROW_HEADER = 90;
	const Vec2<int> TOP_LEFT;
	
	class AgentControl
	{
	  public:
		sp<Agent> agent;
		sp<Control> normalControl;
		sp<Control> selectedControl;
		
		void setLocation(Vec2<int> pos);

		AgentControl() = default;
		AgentControl(sp<Agent> agent, sp<Control> normalControl, sp<Control> selectedControl);
	};
	  
	sp<Form> menuform;

	sp<GameState> state;

	void displayAgent(sp<Agent> agent);
	sp<Control> createAgentControl(StateRef<Agent> agent, bool selected);
	sp<Image> healthImage;
	sp<Image> shieldImage;
	std::vector<sp<Image>> unitRanks;
	std::vector<sp<Image>> bigUnitRanks;
	std::vector<sp<Image>> unitSelect;
	
	std::set<sp<AgentControl>> agents;
	sp<AgentControl> selectedAgent;
	sp<Agent> lastSelectedAgent;
	sp<AgentControl> draggedAgent;
	Vec2<int> draggedAgentOffset;
	bool draggedMoved = false;
	int draggedOrigin = 0;

  public:
	BattlePreStart(sp<GameState> state);
	
	void updateAgents();

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
