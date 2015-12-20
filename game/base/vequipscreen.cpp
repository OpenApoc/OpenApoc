#include "game/base/vequipscreen.h"
#include "game/city/vequipment.h"
#include "framework/framework.h"
#include "game/city/city.h"
#include "game/city/vehicle.h"

namespace OpenApoc
{

VEquipScreen::VEquipScreen(Framework &fw)
    : Stage(fw), form(fw.gamecore->GetForm("FORM_VEQUIPSCREEN")), selected(nullptr),
      selectionType(VEquipmentType::Type::Weapon)
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
}

void VEquipScreen::Update(StageCmd *const cmd)
{
	form->Update();
	*cmd = stageCmd;
	stageCmd = StageCmd();
}

void VEquipScreen::Render()
{
	static const Vec2<int> EQUIP_GRID_SIZE{16, 16};
	fw.Stage_GetPrevious(this->shared_from_this())->Render();
	fw.renderer->drawFilledRect({0, 0}, fw.Display_GetSize(), Colour{0, 0, 0, 128});
	// FIXME: Move this to EventOccurred and only on change?
	// if (mouseIsOverEquipment){do stuff} else
	{
		auto *nameLabel = form->FindControlTyped<Label>("NAME");
		nameLabel->SetText(selected->name);

		// FIXME: These stats would be great to have a generic (string?) referenced list
		auto *label = form->FindControlTyped<Label>("LABEL_1");
		label->SetText("Constitution");
		label = form->FindControlTyped<Label>("VALUE_1");
		label->SetText(UString::format("%d", (int)selected->getConstitution()));

		label = form->FindControlTyped<Label>("LABEL_2");
		label->SetText("Armor");
		label = form->FindControlTyped<Label>("VALUE_2");
		label->SetText(UString::format("%d", (int)selected->getArmor()));

		// FIXME: This value doesn't seem to be the same as the %age shown in the ui?
		label = form->FindControlTyped<Label>("LABEL_3");
		label->SetText("Accuracy");
		label = form->FindControlTyped<Label>("VALUE_3");
		label->SetText(UString::format("%d", (int)selected->getAccuracy()));

		label = form->FindControlTyped<Label>("LABEL_4");
		label->SetText("Top Speed");
		label = form->FindControlTyped<Label>("VALUE_4");
		label->SetText(UString::format("%d", (int)selected->getTopSpeed()));

		label = form->FindControlTyped<Label>("LABEL_5");
		label->SetText("Acceleration");
		label = form->FindControlTyped<Label>("VALUE_5");
		label->SetText(UString::format("%d", (int)selected->getAcceleration()));

		label = form->FindControlTyped<Label>("LABEL_6");
		label->SetText("Weight");
		label = form->FindControlTyped<Label>("VALUE_6");
		label->SetText(UString::format("%d", (int)selected->getWeight()));

		label = form->FindControlTyped<Label>("LABEL_7");
		label->SetText("Fuel");
		label = form->FindControlTyped<Label>("VALUE_7");
		label->SetText(UString::format("%d", (int)selected->getFuel()));

		label = form->FindControlTyped<Label>("LABEL_8");
		label->SetText("Passengers");
		label = form->FindControlTyped<Label>("VALUE_8");
		label->SetText(UString::format("%d/%d", (int)selected->getPassengers(),
		                               (int)selected->getMaxPassengers()));

		label = form->FindControlTyped<Label>("LABEL_9");
		label->SetText("Cargo");
		label = form->FindControlTyped<Label>("VALUE_9");
		label->SetText(
		    UString::format("%d/%d", (int)selected->getCargo(), (int)selected->getMaxCargo()));
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
		auto pos = e->equippedPosition * EQUIP_GRID_SIZE;
		pos += equipOffset;
		fw.renderer->draw(e->type.equipscreen_sprite, pos);
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
