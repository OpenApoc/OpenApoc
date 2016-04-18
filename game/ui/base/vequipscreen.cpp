#include "game/ui/base/vequipscreen.h"
#include "forms/forms.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vequipment.h"
#include "game/state/gamestate.h"
#include <cmath>

namespace OpenApoc
{
const Vec2<int> VEquipScreen::EQUIP_GRID_SLOT_SIZE{16, 16};
const Vec2<int> VEquipScreen::EQUIP_GRID_SLOTS{16, 16};

static const Colour EQUIP_GRID_COLOUR{40, 40, 40, 255};
static const Colour EQUIP_GRID_COLOUR_ENGINE{255, 255, 40, 255};
static const Colour EQUIP_GRID_COLOUR_WEAPON{255, 40, 40, 255};
static const Colour EQUIP_GRID_COLOUR_GENERAL{255, 40, 255, 255};
static const float GLOW_COUNTER_INCREMENT = M_PI / 15.0f;

VEquipScreen::VEquipScreen(sp<GameState> state)
    : Stage(), form(ui().GetForm("FORM_VEQUIPSCREEN")), selectionType(VEquipmentType::Type::Weapon),
      pal(fw().data->load_palette("xcom3/UFODATA/VROADWAR.PCX")),
      labelFont(ui().GetFont("SMALFONT")), drawHighlightBox(false), state(state), glowCounter(0)

{
	form->FindControlTyped<RadioButton>("BUTTON_SHOW_WEAPONS")->SetChecked(true);

	for (auto &v : state->vehicles)
	{
		auto vehicle = v.second;
		if (vehicle->owner != state->getPlayer())
			continue;
		this->setSelectedVehicle(vehicle);
		break;
	}
	if (!this->selected)
	{
		LogError("No vehicles found - the original apoc didn't open the equip screen in this case");
	}
}

VEquipScreen::~VEquipScreen() {}

void VEquipScreen::Begin()
{

	auto list = form->FindControlTyped<ListBox>("VEHICLE_SELECT_BOX");
	for (auto &v : state->vehicles)
	{
		auto vehicle = v.second;
		if (vehicle->owner != state->getPlayer())
			continue;
		auto graphic = mksp<Graphic>(vehicle->type->equip_icon_big);
		graphic->AutoSize = true;
		list->AddItem(graphic);
		this->vehicleSelectionControls[graphic] = vehicle;

		if (vehicle == this->selected)
		{
			list->setSelected(graphic);
		}
	}
}

void VEquipScreen::Pause() {}

void VEquipScreen::Resume() {}

void VEquipScreen::Finish() {}

void VEquipScreen::EventOccurred(Event *e)
{
	form->EventOccured(e);

	if (e->Type() == EVENT_KEY_DOWN)
	{
		if (e->Keyboard().KeyCode == SDLK_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}
	if (e->Type() == EVENT_FORM_INTERACTION && e->Forms().EventFlag == FormEventType::MouseDown)
	{
		auto it = this->vehicleSelectionControls.find(e->Forms().RaisedBy);
		if (it != this->vehicleSelectionControls.end())
		{
			this->setSelectedVehicle(it->second);
			return;
		}
	}
	else if (e->Type() == EVENT_FORM_INTERACTION &&
	         e->Forms().EventFlag == FormEventType::ButtonClick)
	{

		if (e->Forms().RaisedBy->Name == "BUTTON_OK")
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}
	else if (e->Type() == EVENT_FORM_INTERACTION &&
	         e->Forms().EventFlag == FormEventType::CheckBoxChange)
	{
		if (form->FindControlTyped<RadioButton>("BUTTON_SHOW_WEAPONS")->IsChecked())
		{
			this->selectionType = VEquipmentType::Type::Weapon;
			return;
		}
		else if (form->FindControlTyped<RadioButton>("BUTTON_SHOW_ENGINES")->IsChecked())
		{
			this->selectionType = VEquipmentType::Type::Engine;
			return;
		}
		else if (form->FindControlTyped<RadioButton>("BUTTON_SHOW_GENERAL")->IsChecked())
		{
			this->selectionType = VEquipmentType::Type::General;
			return;
		}
	}

	// Reset the highlight box even if we're dragging
	if (e->Type() == EVENT_MOUSE_MOVE)
	{
		this->drawHighlightBox = false;
	}

	// Check if we've moused over equipment/vehicle so we can show the stats.
	if (e->Type() == EVENT_MOUSE_MOVE && !this->draggedEquipment)
	{
		// Wipe any previously-highlighted stuff
		this->highlightedVehicle = nullptr;
		this->highlightedEquipment = "";

		Vec2<int> mousePos{e->Mouse().X, e->Mouse().Y};

		// Check if we're over any equipment in the paper doll
		for (auto &pair : this->equippedItems)
		{
			if (pair.first.within(mousePos))
			{
				this->highlightedEquipment = pair.second->type;
				return;
			}
		}

		// Check if we're over any equipment in the list at the bottom
		for (auto &pair : this->inventoryItems)
		{
			if (pair.first.within(mousePos))
			{
				this->highlightedEquipment = pair.second;
				this->drawHighlightBox = true;
				this->highlightBoxColour = {255, 255, 255, 255};
				this->highlightBox = pair.first;
				return;
			}
		}

		// Check if we're over any vehicles in the side bar
	}
	StateRef<Base> base;
	for (auto &b : state->player_bases)
	{
		if (b.second->building == selected->currentlyLandedBuilding)
			base = {state.get(), b.first};
	}
	// Only allow removing equipment if we're in a base, otherwise it'll disappear
	if (e->Type() == EVENT_MOUSE_DOWN && base)
	{
		Vec2<int> mousePos{e->Mouse().X, e->Mouse().Y};

		// Check if we're over any equipment in the paper doll
		for (auto &pair : this->equippedItems)
		{
			if (pair.first.within(mousePos))
			{
				// FIXME: base->addBackToInventory(item); vehicle->unequip(item);
				this->draggedEquipment = pair.second->type;
				this->draggedEquipmentOffset = pair.first.p0 - mousePos;

				// Return the equipment to the inventory
				this->selected->removeEquipment(pair.second);
				base->inventory[pair.second->type.id]++;
				// FIXME: Return ammo to inventory
				// FIXME: what happens if we don't have the stores to return?
				return;
			}
		}

		// Check if we're over any equipment in the list at the bottom
		for (auto &pair : this->inventoryItems)
		{
			if (pair.first.within(mousePos))
			{
				// Dragging an object doesn't (Yet) remove it from the inventory
				this->draggedEquipment = pair.second;
				this->draggedEquipmentOffset = pair.first.p0 - mousePos;
				return;
			}
		}
	}
	if (e->Type() == EVENT_MOUSE_UP)
	{
		if (this->draggedEquipment)
		{
			// Are we over the grid? If so try to place it on the vehicle.
			auto paperDollControl = form->FindControlTyped<Graphic>("PAPER_DOLL");
			Vec2<int> equipOffset = paperDollControl->Location + form->Location;

			Vec2<int> equipmentPos = fw().getCursorPosition() + this->draggedEquipmentOffset;
			// If this is within the grid try to snap it
			Vec2<int> equipmentGridPos = equipmentPos - equipOffset;
			equipmentGridPos /= EQUIP_GRID_SLOT_SIZE;
			if (this->selected->canAddEquipment(equipmentGridPos, this->draggedEquipment))
			{
				if (base->inventory[draggedEquipment->id] <= 0)
				{
					LogError("Trying to equip item \"%s\" with zero inventory",
					         this->draggedEquipment->id.c_str());
				}
				base->inventory[draggedEquipment->id]--;
				this->selected->addEquipment(*state, equipmentGridPos, this->draggedEquipment);
				// FIXME: Add ammo to equipment
			}
			this->draggedEquipment = "";
		}
	}
}

void VEquipScreen::Update(StageCmd *const cmd)
{
	this->glowCounter += GLOW_COUNTER_INCREMENT;
	// Loop the increment over the period, otherwise we could start getting lower precision etc. if
	// people leave the screen going for a few centuries
	while (this->glowCounter > 2.0f * M_PI)
	{
		this->glowCounter -= 2.0f * M_PI;
	}
	form->Update();
	*cmd = stageCmd;
	stageCmd = StageCmd();
}

void VEquipScreen::Render()
{
	this->equippedItems.clear();
	this->inventoryItems.clear();

	fw().Stage_GetPrevious(this->shared_from_this())->Render();

	fw().renderer->setPalette(this->pal);

	fw().renderer->drawFilledRect({0, 0}, fw().Display_GetSize(), Colour{0, 0, 0, 128});

	// The labels/values in the stats column are used for lots of different things, so keep them
	// around clear them and keep them around in a vector so we don't have 5 copies of the same
	// "reset unused entries" code around
	std::vector<sp<Label>> statsLabels;
	std::vector<sp<Label>> statsValues;
	for (int i = 0; i < 9; i++)
	{
		auto labelName = UString::format("LABEL_%d", i + 1);
		auto label = form->FindControlTyped<Label>(labelName);
		if (!label)
		{
			LogError("Failed to find UI control matching \"%s\"", labelName.c_str());
		}
		label->SetText("");
		statsLabels.push_back(label);

		auto valueName = UString::format("VALUE_%d", i + 1);
		auto value = form->FindControlTyped<Label>(valueName);
		if (!value)
		{
			LogError("Failed to find UI control matching \"%s\"", valueName.c_str());
		}
		value->SetText("");
		statsValues.push_back(value);
	}
	auto nameLabel = form->FindControlTyped<Label>("NAME");
	auto iconGraphic = form->FindControlTyped<Graphic>("SELECTED_ICON");
	// If no vehicle/equipment is highlighted (mouse-over), or if we're dragging equipment around
	// show the currently selected vehicle stats.
	//
	// Otherwise we show the stats of the vehicle/equipment highlighted.

	if (highlightedEquipment)
	{
		iconGraphic->SetImage(highlightedEquipment->equipscreen_sprite);
		nameLabel->SetText(tr(highlightedEquipment->name));
		int statsCount = 0;

		// All equipment has a weight
		statsLabels[statsCount]->SetText(tr("Weight"));
		statsValues[statsCount]->SetText(UString::format("%d", highlightedEquipment->weight));
		statsCount++;

		// Draw equipment stats
		switch (highlightedEquipment->type)
		{
			case VEquipmentType::Type::Engine:
			{
				auto &engineType = *highlightedEquipment;
				statsLabels[statsCount]->SetText(tr("Top Speed"));
				statsValues[statsCount]->SetText(UString::format("%d", engineType.top_speed));
				statsCount++;
				statsLabels[statsCount]->SetText(tr("Power"));
				statsValues[statsCount]->SetText(UString::format("%d", engineType.power));
				break;
			}
			case VEquipmentType::Type::Weapon:
			{
				auto &weaponType = *highlightedEquipment;
				statsLabels[statsCount]->SetText(tr("Damage"));
				statsValues[statsCount]->SetText(UString::format("%d", weaponType.damage));
				statsCount++;
				statsLabels[statsCount]->SetText(tr("Range"));
				statsValues[statsCount]->SetText(UString::format("%d", weaponType.range));
				statsCount++;
				statsLabels[statsCount]->SetText(tr("Accuracy"));
				statsValues[statsCount]->SetText(UString::format("%d", weaponType.accuracy));
				statsCount++;

				// Only show rounds if non-zero (IE not infinite ammo)
				if (highlightedEquipment->max_ammo)
				{
					statsLabels[statsCount]->SetText(tr("Rounds"));
					statsValues[statsCount]->SetText(
					    UString::format("%d", highlightedEquipment->max_ammo));
					statsCount++;
				}
				break;
			}
			case VEquipmentType::Type::General:
			{
				auto &generalType = *highlightedEquipment;
				if (generalType.accuracy_modifier)
				{
					statsLabels[statsCount]->SetText(tr("Accuracy"));
					statsValues[statsCount]->SetText(
					    UString::format("%d", generalType.accuracy_modifier));
					statsCount++;
				}
				if (generalType.cargo_space)
				{
					statsLabels[statsCount]->SetText(tr("Cargo"));
					statsValues[statsCount]->SetText(
					    UString::format("%d", generalType.cargo_space));
					statsCount++;
				}
				if (generalType.passengers)
				{
					statsLabels[statsCount]->SetText(tr("Passengers"));
					statsValues[statsCount]->SetText(UString::format("%d", generalType.passengers));
					statsCount++;
				}
				if (generalType.alien_space)
				{
					statsLabels[statsCount]->SetText(tr("Aliens Held"));
					statsValues[statsCount]->SetText(
					    UString::format("%d", generalType.alien_space));
					statsCount++;
				}
				if (generalType.missile_jamming)
				{
					statsLabels[statsCount]->SetText(tr("Jamming"));
					statsValues[statsCount]->SetText(
					    UString::format("%d", generalType.missile_jamming));
					statsCount++;
				}
				if (generalType.shielding)
				{
					statsLabels[statsCount]->SetText(tr("Shielding"));
					statsValues[statsCount]->SetText(UString::format("%d", generalType.shielding));
					statsCount++;
				}
				if (generalType.cloaking)
				{
					statsLabels[statsCount]->SetText(tr("Cloaks Craft"));
					statsCount++;
				}
				if (generalType.teleporting)
				{
					statsLabels[statsCount]->SetText(tr("Teleports"));
					statsCount++;
				}

				break;
			}
		}
	}
	else
	{
		auto vehicle = this->highlightedVehicle;
		if (!vehicle)
			vehicle = this->selected;

		nameLabel->SetText(vehicle->name);

		// FIXME: These stats would be great to have a generic (string?) referenced list
		statsLabels[0]->SetText(tr("Constitution"));
		if (vehicle->getConstitution() == vehicle->getMaxConstitution())
		{
			statsValues[0]->SetText(UString::format("%d", vehicle->getConstitution()));
		}
		else
		{
			statsValues[0]->SetText(UString::format("%d/%d", vehicle->getConstitution(),
			                                        vehicle->getMaxConstitution()));
		}

		statsLabels[1]->SetText(tr("Armor"));
		statsValues[1]->SetText(UString::format("%d", vehicle->getArmor()));

		// FIXME: This value doesn't seem to be the same as the %age shown in the ui?
		statsLabels[2]->SetText(tr("Accuracy"));
		statsValues[2]->SetText(UString::format("%d", vehicle->getAccuracy()));

		statsLabels[3]->SetText(tr("Top Speed"));
		statsValues[3]->SetText(UString::format("%d", vehicle->getTopSpeed()));

		statsLabels[4]->SetText(tr("Acceleration"));
		statsValues[4]->SetText(UString::format("%d", vehicle->getAcceleration()));

		statsLabels[5]->SetText(tr("Weight"));
		statsValues[5]->SetText(UString::format("%d", vehicle->getWeight()));

		statsLabels[6]->SetText(tr("Fuel"));
		statsValues[6]->SetText(UString::format("%d", vehicle->getFuel()));

		statsLabels[7]->SetText(tr("Passengers"));
		statsValues[7]->SetText(
		    UString::format("%d/%d", vehicle->getPassengers(), vehicle->getMaxPassengers()));

		statsLabels[8]->SetText(tr("Cargo"));
		statsValues[8]->SetText(
		    UString::format("%d/%d", vehicle->getCargo(), vehicle->getMaxCargo()));

		iconGraphic->SetImage(vehicle->type->equip_icon_small);
	}
	// Now draw the form, the actual equipment is then drawn on top
	form->Render();

	auto paperDollControl = form->FindControlTyped<Graphic>("PAPER_DOLL");
	Vec2<int> equipOffset = paperDollControl->Location + form->Location;
	// Draw the equipment grid
	{
		for (auto &slot : selected->type->equipment_layout_slots)
		{
			Vec2<int> p00 = (slot.bounds.p0 * EQUIP_GRID_SLOT_SIZE) + equipOffset;
			Vec2<int> p11 = (slot.bounds.p1 * EQUIP_GRID_SLOT_SIZE) + equipOffset;
			Vec2<int> p01 = {p00.x, p11.y};
			Vec2<int> p10 = {p11.x, p00.y};
			if (slot.type == selectionType)
			{
				// Scale the sin curve from (-1, 1) to (0, 1)
				float glowFactor = (sin(this->glowCounter) + 1.0f) / 2.0f;
				Colour equipColour;
				switch (selectionType)
				{
					case VEquipmentType::Type::Engine:
						equipColour = EQUIP_GRID_COLOUR_ENGINE;
						break;
					case VEquipmentType::Type::Weapon:
						equipColour = EQUIP_GRID_COLOUR_WEAPON;
						break;
					case VEquipmentType::Type::General:
						equipColour = EQUIP_GRID_COLOUR_GENERAL;
						break;
				}
				Colour selectedColour;
				selectedColour.r = mix(equipColour.r, EQUIP_GRID_COLOUR.r, glowFactor);
				selectedColour.g = mix(equipColour.g, EQUIP_GRID_COLOUR.g, glowFactor);
				selectedColour.b = mix(equipColour.b, EQUIP_GRID_COLOUR.b, glowFactor);
				selectedColour.a = 255;
				fw().renderer->drawLine(p00, p01, selectedColour, 2);
				fw().renderer->drawLine(p01, p11, selectedColour, 2);
				fw().renderer->drawLine(p11, p10, selectedColour, 2);
				fw().renderer->drawLine(p10, p00, selectedColour, 2);
			}
			else
			{
				fw().renderer->drawLine(p00, p01, EQUIP_GRID_COLOUR, 2);
				fw().renderer->drawLine(p01, p11, EQUIP_GRID_COLOUR, 2);
				fw().renderer->drawLine(p11, p10, EQUIP_GRID_COLOUR, 2);
				fw().renderer->drawLine(p10, p00, EQUIP_GRID_COLOUR, 2);
			}
		}
	}
	// Draw the equipped stuff
	for (auto &e : selected->equipment)
	{
		auto pos = e->equippedPosition;

		VehicleType::AlignmentX alignX = VehicleType::AlignmentX::Left;
		VehicleType::AlignmentY alignY = VehicleType::AlignmentY::Top;
		Rect<int> slotBounds;
		bool slotFound = false;

		for (auto &slot : this->selected->type->equipment_layout_slots)
		{
			if (slot.bounds.p0 == pos)
			{
				alignX = slot.align_x;
				alignY = slot.align_y;
				slotBounds = slot.bounds;
				slotFound = true;
				break;
			}
		}

		if (!slotFound)
		{
			LogError("No matching slot for equipment at {%d,%d}", pos.x, pos.y);
		}

		if (pos.x >= EQUIP_GRID_SLOTS.x || pos.y >= EQUIP_GRID_SLOTS.y)
		{
			LogError("Equipment at {%d,%d} outside grid", pos.x, pos.y);
		}
		pos *= EQUIP_GRID_SLOT_SIZE;
		pos += equipOffset;

		int diffX = slotBounds.getWidth() - e->type->equipscreen_size.x;
		int diffY = slotBounds.getHeight() - e->type->equipscreen_size.y;

		switch (alignX)
		{
			case VehicleType::AlignmentX::Left:
				pos.x += 0;
				break;
			case VehicleType::AlignmentX::Right:
				pos.x += diffX * EQUIP_GRID_SLOT_SIZE.x;
				break;
			case VehicleType::AlignmentX::Centre:
				pos.x += (diffX * EQUIP_GRID_SLOT_SIZE.x) / 2;
				break;
		}

		switch (alignY)
		{
			case VehicleType::AlignmentY::Top:
				pos.y += 0;
				break;
			case VehicleType::AlignmentY::Bottom:
				pos.y += diffY * EQUIP_GRID_SLOT_SIZE.y;

				break;
			case VehicleType::AlignmentY::Centre:
				pos.y += (diffY * EQUIP_GRID_SLOT_SIZE.y) / 2;
				break;
		}

		fw().renderer->draw(e->type->equipscreen_sprite, pos);
		Vec2<int> endPos = pos;
		endPos.x += e->type->equipscreen_sprite->size.x;
		endPos.y += e->type->equipscreen_sprite->size.y;
		this->equippedItems.emplace_back(std::make_pair(Rect<int>{pos, endPos}, e));
	}

	// Only draw inventory that can be used by this type of craft
	VEquipmentType::User allowedEquipmentUser;
	switch (this->selected->type->type)
	{
		case VehicleType::Type::Flying:
			allowedEquipmentUser = VEquipmentType::User::Air;
			break;
		case VehicleType::Type::Ground:
			allowedEquipmentUser = VEquipmentType::User::Ground;
			break;
		default:
			LogError(
			    "Trying to draw equipment screen of unsupported vehicle type for vehicle \"%s\"",
			    this->selected->name.c_str());
			allowedEquipmentUser = VEquipmentType::User::Air;
	}
	// Draw the inventory if the selected is in a building, and that is a base
	StateRef<Base> base;
	for (auto &b : state->player_bases)
	{
		if (b.second->building == selected->currentlyLandedBuilding)
			base = {state.get(), b.first};
	}
	if (base)
	{
		auto inventoryControl = form->FindControlTyped<Graphic>("INVENTORY");
		Vec2<int> inventoryPosition = inventoryControl->Location + form->Location;
		for (auto &invPair : base->inventory)
		{
			// The gap between the bottom of the inventory image and the count label
			static const int INVENTORY_COUNT_Y_GAP = 4;
			// The gap between the end of one inventory image and the start of the next
			static const int INVENTORY_IMAGE_X_GAP = 4;
			auto equipIt = state->vehicle_equipment.find(invPair.first);
			if (equipIt == state->vehicle_equipment.end())
			{
				// It's not vehicle equipment, skip
				continue;
			}
			auto equipmentType = StateRef<VEquipmentType>{state.get(), equipIt->first};
			if (equipmentType->type != this->selectionType)
			{
				// Skip equipment of different types
				continue;
			}
			if (!equipmentType->users.count(allowedEquipmentUser))
			{
				// The selected vehicle is not a valid user of the equipment, don't draw
				continue;
			}
			int count = invPair.second;
			if (count == 0)
			{
				// Not in stock
				continue;
			}
			auto countImage = labelFont->getString(UString::format("%d", count));
			auto &equipmentImage = equipmentType->equipscreen_sprite;
			fw().renderer->draw(equipmentImage, inventoryPosition);

			Vec2<int> countLabelPosition = inventoryPosition;
			countLabelPosition.y += INVENTORY_COUNT_Y_GAP + equipmentImage->size.y;
			// FIXME: Center in X?
			fw().renderer->draw(countImage, countLabelPosition);

			Vec2<int> inventoryEndPosition = inventoryPosition;
			inventoryEndPosition.x += equipmentImage->size.x;
			inventoryEndPosition.y += equipmentImage->size.y;

			this->inventoryItems.emplace_back(Rect<int>{inventoryPosition, inventoryEndPosition},
			                                  equipmentType);

			// Progress inventory offset by width of image + gap
			inventoryPosition.x += INVENTORY_IMAGE_X_GAP + equipmentImage->size.x;
		}
	}
	if (this->drawHighlightBox)
	{
		Vec2<int> p00 = highlightBox.p0;
		Vec2<int> p11 = highlightBox.p1;
		Vec2<int> p01 = {p00.x, p11.y};
		Vec2<int> p10 = {p11.x, p00.y};
		fw().renderer->drawLine(p00, p01, highlightBoxColour, 1);
		fw().renderer->drawLine(p01, p11, highlightBoxColour, 1);
		fw().renderer->drawLine(p11, p10, highlightBoxColour, 1);
		fw().renderer->drawLine(p10, p00, highlightBoxColour, 1);
	}
	if (this->draggedEquipment)
	{
		// Draw equipment we're currently dragging (snapping to the grid if possible)
		Vec2<int> equipmentPos = fw().getCursorPosition() + this->draggedEquipmentOffset;
		// If this is within the grid try to snap it
		Vec2<int> equipmentGridPos = equipmentPos - equipOffset;
		equipmentGridPos /= EQUIP_GRID_SLOT_SIZE;
		if (equipmentGridPos.x < 0 || equipmentGridPos.x >= EQUIP_GRID_SLOTS.x ||
		    equipmentGridPos.y < 0 || equipmentGridPos.y >= EQUIP_GRID_SLOTS.y)
		{
			// This is outside thge grid
		}
		else
		{
			// Inside the grid, snap
			equipmentPos = equipmentGridPos * EQUIP_GRID_SLOT_SIZE;
			equipmentPos += equipOffset;
		}
		fw().renderer->draw(this->draggedEquipment->equipscreen_sprite, equipmentPos);
	}
}

bool VEquipScreen::IsTransition() { return false; }

void VEquipScreen::setSelectedVehicle(sp<Vehicle> vehicle)
{
	if (!vehicle)
	{
		LogError("Trying to set invalid selected vehicle");
		return;
	}
	LogInfo("Selecting vehicle \"%s\"", vehicle->name.c_str());
	this->selected = vehicle;
	auto backgroundImage = vehicle->type->equipment_screen;
	if (!backgroundImage)
	{
		LogError("Trying to view equipment screen of vehicle \"%s\" which has no equipment screen "
		         "background",
		         vehicle->type->name.c_str());
	}

	auto backgroundControl = form->FindControlTyped<Graphic>("BACKGROUND");
	backgroundControl->SetImage(backgroundImage);
}

} // namespace OpenApoc
