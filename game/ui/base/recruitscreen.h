#pragma once

#include "forms/control.h"
#include "game/state/rules/agenttype.h"
#include "game/state/stateobject.h"
#include "game/ui/base/basestage.h"
#include "library/sp.h"
#include "library/vec.h"
#include <list>
#include <vector>

namespace OpenApoc
{

static const int HIRE_COST_SOLDIER = 600;
static const int HIRE_COST_BIO = 800;
static const int HIRE_COST_PHYSIC = 800;
static const int HIRE_COST_ENGI = 800;
static const int FIRE_COST_SOLDIER = 0;
static const int FIRE_COST_BIO = 0;
static const int FIRE_COST_PHYSIC = 0;
static const int FIRE_COST_ENGI = 0;

class Base;
class GameState;
class Agent;
class Control;
class ScrollBar;
class Label;
class Image;
class Graphic;
class BitmapFont;
class VehicleType;
class Vehicle;
class VEquipmentType;
class VAmmoType;
class AEquipmentType;

class RecruitScreen : public BaseStage
{
  private:
	void changeBase(sp<Base> newBase) override;
	sp<Graphic> arrow;
	sp<Label> textViewBaseStatic;

	std::vector<std::list<sp<Control>>> agentLists;
	std::vector<sp<Image>> bigUnitRanks;

	sp<Form> formAgentStats;
	sp<Form> formPersonnelStats;

	AgentType::Role type;
	// Event for displaying agent stats.
	std::function<void(FormsEvent *e)> onHover;
	// Populate the agentList.
	void populateAgentList();

  public:
	RecruitScreen(sp<GameState> state);
	~RecruitScreen() override;

	// Fills the form of personnel's statistics. Such as skill.
	static void personnelSheet(const Agent &agent, sp<Form> formPersonnelStats);
	// Loads and returns big pictures of ranks.
	static std::vector<sp<Image>> getBigUnitRanks();

	void setDisplayType(const AgentType::Role role);

	int getLeftIndex();

	void updateFormValues();
	void updateBaseHighlight();
	void fillBaseBar(int percent);
	void displayAgentStats(const Agent &agent);

	void attemptCloseScreen();
	void closeScreen(bool confirmed = false);
	// Execute orders given in the screen
	void executeOrders();

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
