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

class Base;
class GameState;
class Agent;
class Control;
class ScrollBar;
class Label;
class Image;
class Graphic;

class TransactionScreen : public BaseStage
{
  public:
	enum class Mode
	{
		AlienContainment,
		BuySell,
		Transfer
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
		sp<Image> bgLeft;
		sp<Image> bgRight;
		sp<Image> purchaseBoxIcon;
		sp<Image> purchaseXComIcon;
		sp<Image> purchaseArrow;
		sp<Image> alienContainedDetain;
		sp<Image> alienContainedKill;
		sp<Image> scrollLeft;
		sp<Image> scrollRight;
		sp<Image> transactionShade;

		// Subcontrols
		sp<ScrollBar> scrollBar;
		sp<Label> deltaLeft;
		sp<Label> deltaRight;
		sp<Label> stockLeft;
		sp<Label> stockRight;

	  public:
		// Data

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

		// Own Methods

		void setIndex(int indexLeft, int indexRight);
		void updateValues();

		/*
		// Transferring/Buying/selling agent equipment and ammo
		// Transferring/Sacking alien containment
		TransactionControl(GameState &state, StateRef<AEquipmentType> agentEquipmentType, bool
		transfer);
		// Transferring/Buying/selling vehicle equipment
		TransactionControl(GameState &state, StateRef<VEquipmentType> vehicleEquipmentType, bool
		transfer);
		// Transferring/Buying/selling vehicle ammo and fuel
		TransactionControl(GameState &state, StateRef<VAmmoType> vehicleAmmoType, bool transfer);
		// Buying vehicles
		TransactionControl(GameState &state, StateRef<VehicleType> vehicleType);
		// Transferring/Selling vehicles
		TransactionControl(GameState &state, StateRef<Vehicle> vehicle, bool transfer);
		*/
		TransactionControl(UString id, Type type, UString name, UString manufacturer, bool isAmmo,
		                   bool isBio, int price, int storeSpace, std::vector<int> initialStock);

		int getStoreDelta(int index) const;
		int getPriceDelta(int index) const;

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

  private:
	void changeBase(sp<Base> newBase) override;

  public:
	TransactionScreen(sp<GameState> state, Mode mode);
	~TransactionScreen() override;

	Mode mode;

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
