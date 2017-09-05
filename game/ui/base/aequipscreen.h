#pragma once

#include "framework/stage.h"
#include "game/state/stateobject.h"
#include "library/rect.h"
#include "library/sp.h"
#include "library/vec.h"
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
class Graphic;
enum class BodyPart;

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

	std::map<BodyPart, std::vector<Vec2<int>>> FATAL_WOUND_LOCATIONS;

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
	sp<Graphic> inventoryControl;

	Vec2<int> draggedEquipmentOffset;
	Vec2<int> draggedEquipmentOrigin;
	sp<AEquipment> draggedEquipment;

	// Items currently on the "ground"
	std::list<std::tuple<Rect<int>, int, sp<AEquipment>>> inventoryItems;
	int inventoryPage = 0;
	void clampInventoryPage();

	// Items temporarily stored inside vehicle (go into vehicle storage when exiting)
	std::map<sp<Vehicle>, std::list<sp<AEquipment>>> vehicleItems;
	// Items temporarily stored inside building (vanish when exiting)
	std::map<sp<Building>, std::list<sp<AEquipment>>> buildingItems;
	// Items temporarily stored near agent (vanish when exiting)
	std::map<sp<Agent>, std::list<sp<AEquipment>>> agentItems;

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
	std::vector<sp<Image>> unitSelect;
	sp<Image> woundImage;

	void displayAgent(sp<Agent> agent);
	void displayItem(sp<AEquipment> item);

	Mode getMode();

	void refreshInventoryItems();
	void populateInventoryItemsBattle();
	void populateInventoryItemsBase();
	void populateInventoryItemsVehicle();
	void populateInventoryItemsBuilding();
	void populateInventoryItemsAgent();

	void removeItemFromInventory(sp<AEquipment> item);
	void removeItemFromInventoryBase(sp<AEquipment> item);
	void removeItemFromInventoryBattle(sp<AEquipment> item);
	void removeItemFromInventoryVehicle(sp<AEquipment> item);
	void removeItemFromInventoryBuilding(sp<AEquipment> item);
	void removeItemFromInventoryAgent(sp<AEquipment> item);
	void addItemToInventory(sp<AEquipment> item);
	void addItemToInventoryBase(sp<AEquipment> item);
	void addItemToInventoryBattle(sp<AEquipment> item);
	void addItemToInventoryVehicle(sp<AEquipment> item);
	void addItemToInventoryBuilding(sp<AEquipment> item);
	void addItemToInventoryAgent(sp<AEquipment> item);

	void attemptCloseScreen();
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
