#include "game/base/vequipscreen.h"
#include "game/city/vequipment.h"
#include "framework/framework.h"
#include "game/city/city.h"
#include "game/city/vehicle.h"
#include "game/city/building.h"

namespace OpenApoc
{
const Vec2<int> VEquipScreen::EQUIP_GRID_SLOT_SIZE{16, 16};
const Vec2<int> VEquipScreen::EQUIP_GRID_SLOTS{16, 16};

VEquipScreen::VEquipScreen(Framework &fw)
    : Stage(fw), form(fw.gamecore->GetForm("FORM_VEQUIPSCREEN")), selected(nullptr),
      selectionType(VEquipmentType::Type::Weapon),
      pal(fw.data->load_palette("xcom3/UFODATA/VROADWAR.PCX")),
      labelFont(fw.gamecore->GetFont("SMALFONT"))

{
	sp<Vehicle> vehicle;
	for (auto &vehiclePtr : fw.state->getPlayer()->vehicles)
	{
		vehicle = vehiclePtr.lock();
		if (vehicle)
			break;
		LogError("Invalid vehicle found in list - this should never happen as they're cleaned up "
		         "at the end of each city update?");
	}
	if (vehicle == nullptr)
	{
		LogError(
		    "No vehicles - 'original' apoc didn't allow the equip screen to appear in this case");
	}
	this->setSelectedVehicle(vehicle);
}

VEquipScreen::~VEquipScreen() {}

void VEquipScreen::Begin() {}

void VEquipScreen::Pause() {}

void VEquipScreen::Resume() {}

void VEquipScreen::Finish() {}

void VEquipScreen::EventOccurred(Event *e)
{
	form->EventOccured(e);
	fw.gamecore->MouseCursor->EventOccured(e);

	if (e->Type == EVENT_KEY_DOWN)
	{
		if (e->Data.Keyboard.KeyCode == SDLK_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
		else if (e->Data.Keyboard.KeyCode == SDLK_RIGHT)
		{
			auto &vehicleList = fw.state->getPlayer()->vehicles;
			// FIXME: Debug hack to cycle through vehicles
			auto currentPos = vehicleList.begin();
			while (currentPos != vehicleList.end())
			{
				auto currentPtr = currentPos->lock();
				if (currentPtr == this->selected)
					break;
				currentPos++;
			}
			if (currentPos == vehicleList.end())
			{
				LogError("Failed to find current vehicle in list");
			}

			currentPos++;

			while (currentPos != vehicleList.end())
			{
				auto newPtr = currentPos->lock();
				if (newPtr)
				{
					this->setSelectedVehicle(newPtr);
					return;
				}
			}
			// Looping back around
			currentPos = vehicleList.begin();
			while (currentPos != vehicleList.end())
			{
				auto newPtr = currentPos->lock();
				if (newPtr)
				{
					this->setSelectedVehicle(newPtr);
					return;
				}
			}
			LogError("No vehicle found in list to progress to");
		}
	}
	if (e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.EventFlag == FormEventType::ButtonClick)
	{
		if (e->Data.Forms.RaisedBy->Name == "BUTTON_OK")
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}

	// Check if we've moused over equipment/vehicle so we can show the stats.
	if (e->Type == EVENT_MOUSE_MOVE && !this->draggedEquipment)
	{
		auto *paperDollControl = form->FindControlTyped<Graphic>("PAPER_DOLL");

		auto equipOffset = paperDollControl->Location + form->Location;
		auto equipEndPoint = equipOffset + (EQUIP_GRID_SLOT_SIZE * EQUIP_GRID_SLOTS);
		// Wipe any previously-highlighted stuff
		this->highlightedVehicle = nullptr;
		this->highlightedEquipment = nullptr;
		Vec2<int> mousePos{e->Data.Mouse.X, e->Data.Mouse.Y};
		if (mousePos.x >= equipOffset.x && mousePos.x < equipEndPoint.x &&
		    mousePos.y >= equipOffset.y && mousePos.y < equipEndPoint.y)
		{
			// We're inside the equipment grid, so check if we intersect with any equipment
			Vec2<int> slotPosition = (mousePos - equipOffset) / EQUIP_GRID_SLOT_SIZE;

			for (auto &e : this->selected->equipment)
			{
				Rect<int> r{e->equippedPosition, e->equippedPosition + e->type.equipscreen_size};
				if (r.within(slotPosition))
				{
					this->highlightedEquipment = e;
				}
			}
		}
		// Check if we're over any equipment in the paper doll

		// Check if we're over any equipment in the list at the bottom

		// Check if we're over any vehicles in the side bar
	}
}

void VEquipScreen::Update(StageCmd *const cmd)
{
	form->Update();
	*cmd = stageCmd;
	stageCmd = StageCmd();
}

void VEquipScreen::Render()
{

	fw.Stage_GetPrevious(this->shared_from_this())->Render();

	fw.renderer->setPalette(this->pal);

	fw.renderer->drawFilledRect({0, 0}, fw.Display_GetSize(), Colour{0, 0, 0, 128});

	// The labels/values in the stats column are used for lots of different things, so keep them
	// around clear them and keep them around in a vector so we don't have 5 copies of the same
	// "reset unused entries" code around
	std::vector<Label *> statsLabels;
	std::vector<Label *> statsValues;
	for (int i = 0; i < 9; i++)
	{
		auto labelName = UString::format("LABEL_%d", i + 1);
		auto *label = form->FindControlTyped<Label>(labelName);
		if (!label)
		{
			LogError("Failed to find UI control matching \"%s\"", labelName.c_str());
		}
		label->SetText("");
		statsLabels.push_back(label);

		auto valueName = UString::format("VALUE_%d", i + 1);
		auto *value = form->FindControlTyped<Label>(valueName);
		if (!value)
		{
			LogError("Failed to find UI control matching \"%s\"", valueName.c_str());
		}
		value->SetText("");
		statsValues.push_back(value);
	}
	auto *nameLabel = form->FindControlTyped<Label>("NAME");
	auto *iconGraphic = form->FindControlTyped<Graphic>("SELECTED_ICON");
	// If no vehicle/equipment is highlighted (mouse-over), or if we're dragging equipment around
	// show the currently selected vehicle stats.
	//
	// Otherwise we show the stats of the vehicle/equipment highlighted.

	if (highlightedEquipment)
	{
		iconGraphic->SetImage(highlightedEquipment->type.equipscreen_sprite);
		nameLabel->SetText(highlightedEquipment->type.name);
		int statsCount = 0;

		// All equipment has a weight
		statsLabels[statsCount]->SetText("Weight");
		statsValues[statsCount]->SetText(
		    UString::format("%d", (int)highlightedEquipment->type.weight));
		statsCount++;

		// Draw equipment stats
		switch (highlightedEquipment->type.type)
		{
			case VEquipmentType::Type::Engine:
			{
				auto &engineType = static_cast<const VEngineType &>(highlightedEquipment->type);
				statsLabels[statsCount]->SetText("Top Speed");
				statsValues[statsCount]->SetText(UString::format("%d", (int)engineType.top_speed));
				statsCount++;
				statsLabels[statsCount]->SetText("Power");
				statsValues[statsCount]->SetText(UString::format("%d", (int)engineType.power));
				break;
			}
			case VEquipmentType::Type::Weapon:
			{
				auto &weaponType = static_cast<const VWeaponType &>(highlightedEquipment->type);
				statsLabels[statsCount]->SetText("Damage");
				statsValues[statsCount]->SetText(UString::format("%d", (int)weaponType.damage));
				statsCount++;
				statsLabels[statsCount]->SetText("Range");
				statsValues[statsCount]->SetText(UString::format("%d", (int)weaponType.range));
				statsCount++;
				statsLabels[statsCount]->SetText("Accuracy");
				statsValues[statsCount]->SetText(UString::format("%d", (int)weaponType.accuracy));
				statsCount++;

				// Only show rounds if non-zero (IE not infinite ammo)
				if (highlightedEquipment->type.max_ammo)
				{
					statsLabels[statsCount]->SetText("Rounds");
					statsValues[statsCount]->SetText(
					    UString::format("%d", (int)highlightedEquipment->type.max_ammo));
					statsCount++;
				}
				break;
			}
			case VEquipmentType::Type::General:
			{
				auto &generalType =
				    static_cast<const VGeneralEquipmentType &>(highlightedEquipment->type);
				if (generalType.accuracy_modifier)
				{
					statsLabels[statsCount]->SetText("Accuracy");
					statsValues[statsCount]->SetText(
					    UString::format("%d", (int)generalType.accuracy_modifier));
					statsCount++;
				}
				if (generalType.cargo_space)
				{
					statsLabels[statsCount]->SetText("Cargo");
					statsValues[statsCount]->SetText(
					    UString::format("%d", (int)generalType.cargo_space));
					statsCount++;
				}
				if (generalType.passengers)
				{
					statsLabels[statsCount]->SetText("Passengers");
					statsValues[statsCount]->SetText(
					    UString::format("%d", (int)generalType.passengers));
					statsCount++;
				}
				if (generalType.alien_space)
				{
					statsLabels[statsCount]->SetText("Aliens Held");
					statsValues[statsCount]->SetText(
					    UString::format("%d", (int)generalType.alien_space));
					statsCount++;
				}
				if (generalType.missile_jamming)
				{
					statsLabels[statsCount]->SetText("Jamming");
					statsValues[statsCount]->SetText(
					    UString::format("%d", (int)generalType.missile_jamming));
					statsCount++;
				}
				if (generalType.shielding)
				{
					statsLabels[statsCount]->SetText("Shielding");
					statsValues[statsCount]->SetText(
					    UString::format("%d", (int)generalType.shielding));
					statsCount++;
				}
				if (generalType.cloaking)
				{
					statsLabels[statsCount]->SetText("Cloaks Craft");
					statsCount++;
				}
				if (generalType.teleporting)
				{
					statsLabels[statsCount]->SetText("Teleports");
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
		statsLabels[0]->SetText("Constitution");
		statsValues[0]->SetText(UString::format("%d", (int)vehicle->getConstitution()));

		statsLabels[1]->SetText("Armor");
		statsValues[1]->SetText(UString::format("%d", (int)vehicle->getArmor()));

		// FIXME: This value doesn't seem to be the same as the %age shown in the ui?
		statsLabels[2]->SetText("Accuracy");
		statsValues[2]->SetText(UString::format("%d", (int)vehicle->getAccuracy()));

		statsLabels[3]->SetText("Top Speed");
		statsValues[3]->SetText(UString::format("%d", (int)vehicle->getTopSpeed()));

		statsLabels[4]->SetText("Acceleration");
		statsValues[4]->SetText(UString::format("%d", (int)vehicle->getAcceleration()));

		statsLabels[5]->SetText("Weight");
		statsValues[5]->SetText(UString::format("%d", (int)vehicle->getWeight()));

		statsLabels[6]->SetText("Fuel");
		statsValues[6]->SetText(UString::format("%d", (int)vehicle->getFuel()));

		statsLabels[7]->SetText("Passengers");
		statsValues[7]->SetText(UString::format("%d/%d", (int)vehicle->getPassengers(),
		                                        (int)vehicle->getMaxPassengers()));

		statsLabels[8]->SetText("Cargo");
		statsValues[8]->SetText(
		    UString::format("%d/%d", (int)vehicle->getCargo(), (int)vehicle->getMaxCargo()));

		iconGraphic->SetImage(vehicle->type.equip_icon_small);
	}
	// Now draw the form, the actual equipment is then drawn on top
	form->Render();

	auto *paperDollControl = form->FindControlTyped<Graphic>("PAPER_DOLL");
	Vec2<int> equipOffset = paperDollControl->Location + form->Location;
	// Draw the equipment grid
	{
	}
	// Draw the equipped stuff
	for (auto &e : selected->equipment)
	{
		auto pos = e->equippedPosition;
		if (pos.x >= EQUIP_GRID_SLOTS.x || pos.y >= EQUIP_GRID_SLOTS.y)
		{
			LogError("Equipment at {%d,%d} outside grid", pos.x, pos.y);
		}
		pos *= EQUIP_GRID_SLOT_SIZE;
		pos += equipOffset;
		fw.renderer->draw(e->type.equipscreen_sprite, pos);
	}
	// Draw the inventory if the selected is in a building, and that is a base
	auto bld = this->selected->building.lock();
	sp<Base> base;
	if (bld)
	{
		base = bld->base;
	}
	if (base)
	{
		auto *inventoryControl = form->FindControlTyped<Graphic>("INVENTORY");
		Vec2<int> inventoryPosition = inventoryControl->Location + form->Location;
		for (auto &invPair : base->inventory)
		{
			// The gap between the bottom of the inventory image and the count label
			static const int INVENTORY_COUNT_Y_GAP = 4;
			// The gap between the end of one inventory image and the start of the next
			static const int INVENTORY_IMAGE_X_GAP = 4;
			auto equipIt = fw.rules->getVehicleEquipmentTypes().find(invPair.first);
			if (equipIt == fw.rules->getVehicleEquipmentTypes().end())
			{
				// It's not vehicle equipment, skip
				continue;
			}
			auto &equipmentType = *equipIt->second;
			if (equipmentType.type != this->selectionType)
			{
				// Skip equipment of different types
				// TODO: Hide flying/ground - only types based on selected vehicle
				continue;
			}
			int count = invPair.second;
			auto countImage = labelFont->getString(UString::format("%d", count));
			auto &equipmentImage = equipmentType.equipscreen_sprite;
			fw.renderer->draw(equipmentImage, inventoryPosition);

			Vec2<int> countLabelPosition = inventoryPosition;
			countLabelPosition.y += INVENTORY_COUNT_Y_GAP + equipmentImage->size.y;
			// FIXME: Center in X?
			fw.renderer->draw(countImage, countLabelPosition);

			// Progress inventory offset by width of image + gap
			inventoryPosition.x += INVENTORY_IMAGE_X_GAP + equipmentImage->size.x;
		}
	}
	if (this->draggedEquipment)
	{
		// Draw equipment we're currently dragging (snapping to the grid if possible)
	}
	fw.gamecore->MouseCursor->Render();
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
	auto backgroundImage = vehicle->type.equipment_screen;
	if (!backgroundImage)
	{
		LogError("Trying to view equipment screen of vehicle \"%s\" which has no equipment screen "
		         "background",
		         vehicle->type.name.c_str());
	}

	auto *backgroundControl = form->FindControlTyped<Graphic>("BACKGROUND");
	backgroundControl->SetImage(backgroundImage);
}

} // namespace OpenApoc
