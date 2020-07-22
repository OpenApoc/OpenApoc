#pragma once

#include "forms/control.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include <list>
#include <vector>

namespace OpenApoc
{

class Agent;
class Organisation;
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

// Index of economy in a stock data vector
constexpr int ECONOMY_IDX = 8;

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
		// from - 0-7 for bases, 8 for economy
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
	sp<std::list<wp<TransactionControl>>> linked;
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
	bool isAmmo = false;
	bool isBio = false;
	bool isPerson = false;
	bool manufacturerHostile = false;
	bool manufacturerUnavailable = false;
	bool researched = false;
	StateRef<Organisation> manufacturer;
	UString manufacturerName;
	// Trade state
	Trade tradeState;

	// Own Methods

	void setIndexLeft(int index);
	void setIndexRight(int index);
	void updateValues();
	const decltype(linked) &getLinked() const;

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
	                                            StateRef<VAmmoType> vehicleAmmoType, int indexLeft,
	                                            int indexRight);
	// Buying vehicles
	static sp<TransactionControl> createControl(GameState &state, StateRef<VehicleType> vehicleType,
	                                            int indexLeft, int indexRight);
	// Transferring/Selling vehicles
	static sp<TransactionControl> createControl(GameState &state, StateRef<Vehicle> vehicle,
	                                            int indexLeft, int indexRight);

	static sp<TransactionControl> createControl(const UString &id, Type type, const UString &name,
	                                            StateRef<Organisation> manufacturer, bool isAmmo,
	                                            bool isBio, bool isPerson, bool researched,
	                                            bool manufacturerHostile,
	                                            bool manufacturerUnavailable, int price,
	                                            int storeSpace, std::vector<int> &initialStock,
	                                            int indexLeft, int indexRight);

	static void link(sp<TransactionControl> c1, sp<TransactionControl> c2);

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

}; // namespace OpenApoc
