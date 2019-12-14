#pragma once

#include "game/state/rules/agenttype.h"
#include "game/state/stateobject.h"
#include "game/ui/base/basestage.h"
#include "library/sp.h"
#include <functional>
#include <list>
#include <vector>

namespace OpenApoc
{

class Base;
class GameState;
class Agent;
class Organisation;
class Control;
class FormsEvent;
class ScrollBar;
class Label;
class Image;
class Graphic;
class BitmapFont;
class TransactionControl;
class VehicleType;
class Vehicle;
class VEquipmentType;
class VAmmoType;
class AEquipmentType;

// TODO: move to base?
constexpr int MAX_BASES = 8;
// Don't update highlight right away so that we don't slow too much
// Instead do it after user doesn't act for half a second
constexpr int HIGHLIGHT_UPDATE_DELAY = 30;

class TransactionScreen : public BaseStage
{
  public:
	enum class Type
	{
		Soldier,
		BioChemist,
		Physicist,
		Engineer,
		Vehicle,
		AgentEquipment,
		FlyingEquipment,
		GroundEquipment,
		Aliens
	};

  protected:
	void changeBase(sp<Base> newBase) override;

	// The counter of the highlight update countdown.
	int framesUntilHighlightUpdate = 0;
	// Keeps previous highlight. That allows not to redraw the mini-view buttons by every click.
	BaseGraphics::FacilityHighlight viewHighlightPrevious = BaseGraphics::FacilityHighlight::None;

	sp<Form> formItemAgent;
	sp<Form> formItemVehicle;
	sp<Form> formAgentStats;
	sp<Form> formPersonnelStats;

	sp<Label> textViewBaseStatic;

	Type type;
	// Whether player must conform to limits even on bases which did not change
	bool forceLimits;
	std::map<Type, std::list<sp<TransactionControl>>> transactionControls;

	int lq2Delta = 0;
	int cargo2Delta = 0;
	int bio2Delta = 0;
	int moneyDelta = 0;
	// The text of message box which ask about confirmation to close the screen.
	UString confirmClosureText;

	// Methods

	std::function<void(FormsEvent *e)> onScrollChange;
	std::function<void(FormsEvent *e)> onHover;
	// What equipment should be shown.
	void setDisplayType(Type type);
	// Get the left side index.
	virtual int getLeftIndex();
	// Get the right side index.
	virtual int getRightIndex();

	void populateControlsPeople(AgentType::Role role);
	void populateControlsVehicle();
	void populateControlsAgentEquipment();
	void populateControlsVehicleEquipment();
	void populateControlsAlien();

	// Update statistics on TransactionControls.
	virtual void updateFormValues(bool queueHighlightUpdate = true);
	// Update highlight of facilities on the mini-view.
	virtual void updateBaseHighlight();
	void fillBaseBar(bool left, int percent);
	virtual void displayItem(sp<TransactionControl> control);

	// Is it possible to close the screen without consequences?
	bool isClosable() const;
	// Attempt to close and ask user if necessary.
	void attemptCloseScreen();
	// Close the screen without asking.
	void forcedCloseScreen();
	// Checking conditions and limitations before the execution of orders.
	virtual void closeScreen() = 0;
	// Execute orders given in the screen.
	virtual void executeOrders() = 0;
	// Initialisation the mini view for the second base.
	virtual void initViewSecondBase();

	sp<TransactionControl> findControlById(Type type, const UString &itemId);
	sp<TransactionControl> findControlById(const UString &itemId);

  public:
	TransactionScreen(sp<GameState> state, bool forceLimits = false);

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
