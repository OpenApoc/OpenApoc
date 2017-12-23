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
		// 0-7 for bases
		// 8 for economy
		std::vector<int> initialStock;
		// Current stock
		// 0-7 for bases
		// 8 for economy
		std::vector<int> currentStock;
		int indexLeft = 0;
		int indexRight = 0;
		bool isAmmo = false;
		bool isBio = false;
		UString manufacturer;
		bool manufacturerHostile = false;
		bool manufacturerUnavailable = false;

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

		static sp<TransactionControl> createControl(UString id, Type type, UString name,
		                                            UString manufacturer, bool isAmmo, bool isBio,
		                                            bool manufacturerHostile,
		                                            bool manufacturerUnavailable, int price,
		                                            int storeSpace, std::vector<int> &initialStock,
		                                            int indexLeft, int indexRight);

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

	sp<Label> textViewSecondBase;
	sp<Label> textViewBaseStatic;

	Type type;
	// Wether player must conform to limits even on bases which did not change
	bool forceLimits;
	std::map<Type, std::list<sp<TransactionControl>>> transactionControls;

	int lq2Delta = 0;
	int cargo2Delta = 0;
	int bio2Delta = 0;
	int moneyDelta = 0;

	UString confirmClosure;

	// Methods

	std::function<void(FormsEvent *e)> onScrollChange;
	std::function<void(FormsEvent *e)> onHover;
	//
	void setDisplayType(Type type);

	virtual int getLeftIndex();
	virtual int getRightIndex();

	void populateControlsVehicle();
	void populateControlsAgentEquipment();
	void populateControlsVehicleEquipment();
	void populateControlsAlien();

	virtual void updateFormValues(bool queueHighlightUpdate = true);
	virtual void updateBaseHighlight();
	void fillBaseBar(bool left, int percent);
	void displayItem(sp<TransactionControl> control);

	//
	bool isClosable() const;
	//
	void attemptCloseScreen();
	//
	virtual void closeScreen(bool forced = false) = 0;
	// Execute orders given in the screen
	virtual void executeOrders() = 0;
	//
	virtual void initViewSecondBase();

  public:
	TransactionScreen(sp<GameState> state, bool forceLimits = false);
	~TransactionScreen() override;

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
