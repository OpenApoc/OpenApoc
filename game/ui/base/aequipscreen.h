#pragma once

#include "game/state/stateobject.h"
#include "framework/stage.h"
#include "library/sp.h"
#include "library/vec.h"
#include "library/rect.h"
#include <map>

namespace OpenApoc
{

class Form;
class GameState;
class Palette;
class Agent;
class EquipmentPaperDoll;
class Control;
class Image;
class AEquipment;
class Vehicle;
class Building;
class BitmapFont;

class AEquipScreen : public Stage
{
  private:
	enum class Mode
	{
		Battle,
		Base,
		Vehicle,
		Building,
		Agent
	};

	sp<Agent> firstAgent;

	sp<Form> formMain;
	sp<Form> formActive;
	sp<Form> formAgentStats;
	sp<Form> formItemWeapon;
	sp<Form> formItemArmor;
	sp<Form> formItemGrenade;
	sp<Form> formItemOther;

	sp<Palette> pal;
	sp<GameState> state;
	sp<BitmapFont> labelFont;
	sp<Agent> currentAgent;

	sp<EquipmentPaperDoll> paperDoll;

	Vec2<int> draggedEquipmentOffset;
	sp<AEquipment> draggedEquipment;

	// Items currently on the "ground"
	std::list<std::tuple<Rect<int>, int, sp<AEquipment>>> inventoryItems;

	// Items temporarily stored inside vehicle (go into vehicle storage when exiting)
	std::map<StateRef<Vehicle>, std::list<sp<AEquipment>>> vehicleItems;
	// Items temporarily stored inside building (vanish when exiting)
	std::map<StateRef<Building>, std::list<sp<AEquipment>>> buildingItems;
	// Items temporarily stored near agent (vanish when exiting)
	std::map<StateRef<Agent>, std::list<sp<AEquipment>>> agentItems;

	static const Vec2<int> EQUIP_GRID_SLOT_SIZE;
	static const Vec2<int> EQUIP_GRID_SLOTS;

	sp<Control> createAgentControl(Vec2<int> size, StateRef<Agent> agent);
	// FIXME: healthImage has a copy in CityView - maybe opportunity to merge?
	sp<Image> healthImage;
	sp<Image> shieldImage;
	sp<Image> stunImage;
	sp<Image> iconShade;
	std::vector<sp<Image>> unitRanks;
	std::vector<sp<Image>> bigUnitRanks;

	void displayAgent(sp<Agent> agent);
	void displayItem(sp<AEquipment> item);
	
	Mode getMode();
	
	void refreshInventoryItems();
	void populateInventoryItemsBattle();
	void populateInventoryItemsBase();
	void populateInventoryItemsVehicle();
	void populateInventoryItemsBuilding();
	void populateInventoryItemsTemporary();

	void removeItemFromInventory(sp<AEquipment> item);
	void removeItemFromInventoryBase(sp<AEquipment> item);
	void removeItemFromInventoryBattle(sp<AEquipment> item);
	void removeItemFromInventoryVehicle(sp<AEquipment> item);
	void removeItemFromInventoryBuilding(sp<AEquipment> item);
	void removeItemFromInventoryTemporary(sp<AEquipment> item);
	void addItemToInventory(sp<AEquipment> item);
	void addItemToInventoryBase(sp<AEquipment> item);
	void addItemToInventoryBattle(sp<AEquipment> item);
	void addItemToInventoryVehicle(sp<AEquipment> item);
	void addItemToInventoryBuilding(sp<AEquipment> item);
	void addItemToInventoryTemporary(sp<AEquipment> item);

	void closeScreen();

  public:
	AEquipScreen(sp<GameState> state, sp<Agent> firstAgent = nullptr);
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
