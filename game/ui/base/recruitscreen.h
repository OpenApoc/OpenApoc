#pragma once

#include "forms/control.h"
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
  public:
	enum class Type
	{
		Soldier,
		Bio,
		Physist,
		Engineer
	};

  private:
	void changeBase(sp<Base> newBase) override;
	sp<Graphic> arrow;
	sp<Label> textViewBaseStatic;

  public:
	RecruitScreen(sp<GameState> state);
	~RecruitScreen() override;

	Type type;
	std::vector<std::list<sp<Control>>> agentLists;

	// Methods

	std::function<void(FormsEvent *e)> onHover;

	void setDisplayType(Type type);

	int getLeftIndex();

	void updateFormValues();
	void updateBaseHighlight();
	void fillBaseBar(int percent);
	void displayAgent(sp<Agent> agent);

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
