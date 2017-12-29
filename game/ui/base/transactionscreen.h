#pragma once

#include "forms/control.h"
#include "framework/logger.h"
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
		Bio,
		Physist,
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
		// Setter for the left side and right side indexes.
		void setIndexes(int leftIdx, int rightIdx)
		{
			this->leftIdx = leftIdx;
			this->rightIdx = rightIdx;
		}
		// Get the sum of shipment orders from the base (economy).
		// from, to - 0-7 for bases, 8 for economy
		int shipmentsFrom(const int from, const int exclude = -1) const;
		// Get total shipment orders from(+) and to(-) the base (economy).
		// baseIdx - 0-7 for bases, 8 for economy
		int shipmentsTotal(const int baseIdx) const;
		// Get shipment order.
		int getOrder(const int from, const int to) const;
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
		std::list<sp<TransactionControl>> linked;
		bool suspendUpdates = false;

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
		// Initial stock
		// 0-7 for bases, 8 for economy
		std::vector<int> initialStock; // TODO: remove
		// Current stock
		// 0-7 for bases, 8 for economy
		std::vector<int> currentStock; // TODO: remove
		int indexLeft = 0;
		int indexRight = 0;
		bool isAmmo = false;
		bool isBio = false;
		UString manufacturer;
		bool manufacturerHostile = false;
		bool manufacturerUnavailable = false;
		bool unknownArtifact = false;
		// Trade state
		Trade tradeState;

		// Own Methods

		void setIndexLeft(int index);
		void setIndexRight(int index);
		void updateValues();
		void link(sp<TransactionControl> control);
		const std::list<sp<TransactionControl>> &getLinked() const;

		// Transferring/Buying/selling agent equipment and ammo
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
		              const UString &manufacturer, bool isAmmo, bool isBio,
		              bool manufacturerHostile, bool manufacturerUnavailable, int price,
		              int storeSpace, std::vector<int> &initialStock, int indexLeft, int indexRight,
		              bool unknownArtifact);

		void setupCallbacks();

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

	sp<Label> textViewBaseStatic;

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

	void populateControlsVehicle();
	void populateControlsAgentEquipment();
	void populateControlsVehicleEquipment();
	void populateControlsAlien();

	virtual void updateFormValues(bool queueHighlightUpdate = true);
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
