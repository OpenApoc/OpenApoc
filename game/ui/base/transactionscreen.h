#pragma once

#include "forms/control.h"
#include "framework/logger.h"
#include "game/state/rules/agenttype.h"
#include "game/state/stateobject.h"
#include "game/ui/base/basestage.h"
#include "library/sp.h"
#include "library/vec.h"
#include <list>
#include <vector>

// Don't update highlight right away so that we don't slow too much
// Instead do it after user doesn't act for half a second
#define HIGHLIGHT_UPDATE_DELAY 30

namespace OpenApoc
{

class Base;
class GameState;
class Agent;
class Organisation;
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

constexpr int MAX_BASES = 8;
constexpr int ECONOMY_IDX = 8;

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
	class Trade
	{
	  private:
		// Initial stock.
		// 0-7 for bases, 8 for economy
		std::vector<int> initialStock;
		// Shipment records.
		// map<from, map<to, quantity>>
		// from, to - 0-7 for bases, 8 for economy
		std::map<int, std::map<int, int>> shipments;
		// Current indexes.
		// 0-7 for bases, 8 for economy, -1 for exception ;)
		int leftIdx = -1, rightIdx = -1;

	  public:
		// Setter for initial stock. The vector will be moved.
		void setInitialStock(std::vector<int> &&stock) { initialStock = std::move(stock); }
		// Setter for the left side index.
		void setLeftIndex(int leftIdx) { this->leftIdx = leftIdx; }
		// Getter for the left side index.
		int getLeftIndex() const { return leftIdx; }
		// Setter for the right side index.
		void setRightIndex(int rightIdx) { this->rightIdx = rightIdx; }
		// Getter for the right side index.
		int getRightIndex() const { return rightIdx; }
		// Get the sum of shipment orders from the base (economy).
		// from, to - 0-7 for bases, 8 for economy
		int shipmentsFrom(const int from, const int exclude = -1) const;
		// Get total shipment orders from(+) and to(-) the base (economy).
		// baseIdx - 0-7 for bases, 8 for economy
		int shipmentsTotal(const int baseIdx) const;
		// Get shipment order.
		int getOrder(const int from, const int to) const;
		int getLROrder() const { return getOrder(leftIdx, rightIdx); }
		// Cancel shipment order.
		void cancelOrder(const int from, const int to);
		void cancelOrder() { cancelOrder(leftIdx, rightIdx); }
		// Get current stock.
		int getStock(const int baseIdx, const int oppositeIdx, bool currentStock = false) const;
		// Get current left stock.
		int getLeftStock(bool currentStock = false) const
		{
			return getStock(leftIdx, rightIdx, currentStock);
		}
		// Get current right stock.
		int getRightStock(bool currentStock = false) const
		{
			return getStock(rightIdx, leftIdx, currentStock);
		}
		// ScrollBar support. Get current value.
		int getBalance() const { return getRightStock(true); }
		// ScrollBar support. Set current value.
		int setBalance(const int balance);
	};
	class TransactionControl : public Control
	{
	  public:
		enum class Type
		{
			Soldier,
			BioChemist,
			Physicist,
			Engineer,
			VehicleType,
			Vehicle,
			AgentEquipmentBio,
			AgentEquipmentCargo,
			VehicleEquipment,
			VehicleAmmo
		};

	  private:
		// Resources
		static sp<Image> bgLeft;
		static sp<Image> bgRight;
		static sp<Image> purchaseBoxIcon;
		static sp<Image> purchaseXComIcon;
		static sp<Image> purchaseArrow;
		static sp<Image> alienContainedDetain;
		static sp<Image> alienContainedKill;
		static sp<Image> scrollLeft;
		static sp<Image> scrollRight;
		static sp<Image> transactionShade;
		static sp<BitmapFont> labelFont;
		static bool resourcesInitialised;
		static void initResources();

	  protected:
		// Link
		std::list<sp<TransactionControl>> linked; // TODO: remove
		bool suspendUpdates = false;              // TODO: remove

		// Subcontrols
		sp<ScrollBar> scrollBar;
		sp<Label> deltaLeft;
		sp<Label> deltaRight;
		sp<Label> stockLeft;
		sp<Label> stockRight;

		void setScrollbarValues();

	  public:
		// Item id
		UString itemId;
		// Item type
		Type itemType;
		// Item price
		int price;
		// Item store size
		int storeSpace;
		bool isAmmo = false;
		bool isBio = false;
		bool isPerson = false;
		bool manufacturerHostile = false;
		bool manufacturerUnavailable = false;
		bool unknownArtifact = false;
		StateRef<Organisation> manufacturer;
		UString manufacturerName;
		// Trade state
		Trade tradeState;

		// Own Methods

		void setIndexLeft(int index);
		void setIndexRight(int index);
		void updateValues();
		void link(sp<TransactionControl> control);                  // TODO: remove
		const std::list<sp<TransactionControl>> &getLinked() const; // TODO: remove

		// Transferring/Buying/selling agent equipment and ammo
		// Transferring agents
		static sp<TransactionControl> createControl(GameState &state, StateRef<Agent> agent,
		                                            int indexLeft, int indexRight);
		// Transferring/Sacking alien containment
		static sp<TransactionControl> createControl(GameState &state,
		                                            StateRef<AEquipmentType> agentEquipmentType,
		                                            int indexLeft, int indexRight);
		// Transferring/Buying/selling vehicle equipment
		static sp<TransactionControl> createControl(GameState &state,
		                                            StateRef<VEquipmentType> vehicleEquipmentType,
		                                            int indexLeft, int indexRight);
		// Transferring/Buying/selling vehicle ammo and fuel
		static sp<TransactionControl> createControl(GameState &state,
		                                            StateRef<VAmmoType> vehicleAmmoType,
		                                            int indexLeft, int indexRight);
		// Buying vehicles
		static sp<TransactionControl> createControl(GameState &state,
		                                            StateRef<VehicleType> vehicleType,
		                                            int indexLeft, int indexRight);
		// Transferring/Selling vehicles
		static sp<TransactionControl> createControl(GameState &state, StateRef<Vehicle> vehicle,
		                                            int indexLeft, int indexRight);

		static sp<TransactionControl>
		createControl(const UString &id, Type type, const UString &name,
		              StateRef<Organisation> manufacturer, bool isAmmo, bool isBio, bool isPerson,
		              bool unknownArtifact, bool manufacturerHostile, bool manufacturerUnavailable,
		              int price, int storeSpace, std::vector<int> &initialStock, int indexLeft,
		              int indexRigh);

		void setupCallbacks();

		int getCrewDelta(int index) const;
		int getCargoDelta(int index) const;
		int getBioDelta(int index) const;
		int getPriceDelta() const;

		// Control Methods

	  protected:
		void onRender() override;
		void postRender() override;

	  public:
		//~TransactionControl() override;

		// void eventOccured(Event *e) override;
		// void update() override;
		void unloadResources() override;
	};

  protected:
	void changeBase(sp<Base> newBase) override;

	int framesUntilHighlightUpdate = 0;

	sp<Form> formItemAgent;
	sp<Form> formItemVehicle;
	sp<Form> formAgentStats;
	sp<Form> formPersonelStats;

	sp<Label> textViewBaseStatic;

	std::vector<sp<Image>> bigUnitRanks; // TODO: move to transferscreen

	Type type;
	// Wether player must conform to limits even on bases which did not change
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
	void displayItem(sp<TransactionControl> control);

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
