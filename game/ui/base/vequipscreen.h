#pragma once

#include "framework/stage.h"
#include "game/state/rules/vequipment_type.h"
#include "game/state/stateobject.h"
#include "library/colour.h"
#include "library/rect.h"
#include "library/sp.h"
#include <map>

namespace OpenApoc
{

class Vehicle;
class Form;
class Palette;
class VEquipment;
class BitmapFont;
class GameState;
class Control;
class VEquipmentType;

class VEquipScreen : public Stage
{
  private:
	sp<Form> form;
	sp<Vehicle> selected;
	VEquipmentType::Type selectionType;
	sp<Palette> pal;
	sp<BitmapFont> labelFont;

	sp<Vehicle> highlightedVehicle;
	StateRef<VEquipmentType> highlightedEquipment;

	bool drawHighlightBox;
	Colour highlightBoxColour;
	Rect<int> highlightBox;

	Vec2<int> draggedEquipmentOffset;
	StateRef<VEquipmentType> draggedEquipment;

	static const Vec2<int> EQUIP_GRID_SLOT_SIZE;
	static const Vec2<int> EQUIP_GRID_SLOTS;

	// List of screen-space rects for all equipped items
	std::list<std::pair<Rect<int>, sp<VEquipment>>> equippedItems;
	// List of screen-space rects for all inventory items
	std::list<std::pair<Rect<int>, StateRef<VEquipmentType>>> inventoryItems;

	std::map<sp<Control>, sp<Vehicle>> vehicleSelectionControls;

	sp<GameState> state;

	float glowCounter;

  public:
	VEquipScreen(sp<GameState> state);
	~VEquipScreen() override;

	void begin() override;
	void pause() override;
	void resume() override;
	void finish() override;
	void eventOccurred(Event *e) override;
	void update() override;
	void render() override;
	bool isTransition() override;

	void setSelectedVehicle(sp<Vehicle> vehicle);
};

} // namespace OpenApoc
