#pragma once

#include "framework/stage.h"
#include "library/sp.h"
#include "game/rules/vequipment.h"

namespace OpenApoc
{

class Vehicle;
class Form;
class Palette;
class VEquipment;
class BitmapFont;
class GameState;

class VEquipScreen : public Stage
{
  private:
	StageCmd stageCmd;
	up<Form> form;
	sp<Vehicle> selected;
	VEquipmentType::Type selectionType;
	sp<Palette> pal;
	sp<BitmapFont> labelFont;

	sp<Vehicle> highlightedVehicle;
	const VEquipmentType *highlightedEquipment;

	bool drawHighlightBox;
	Colour highlightBoxColour;
	Rect<int> highlightBox;

	Vec2<int> draggedEquipmentOffset;
	const VEquipmentType *draggedEquipment;

	static const Vec2<int> EQUIP_GRID_SLOT_SIZE;
	static const Vec2<int> EQUIP_GRID_SLOTS;

	// List of screen-space rects for all equipped items
	std::list<std::pair<Rect<int>, sp<VEquipment>>> equippedItems;
	// List of screen-space rects for all inventory items
	std::list<std::pair<Rect<int>, VEquipmentType &>> inventoryItems;

	std::map<Control *, sp<Vehicle>> vehicleSelectionControls;

	sp<GameState> state;

	float glowCounter;

  public:
	VEquipScreen(sp<GameState> state);
	virtual ~VEquipScreen();

	void Begin() override;
	void Pause() override;
	void Resume() override;
	void Finish() override;
	void EventOccurred(Event *e) override;
	void Update(StageCmd *const cmd) override;
	void Render() override;
	bool IsTransition() override;

	void setSelectedVehicle(sp<Vehicle> vehicle);
};

} // namespace OpenApoc
