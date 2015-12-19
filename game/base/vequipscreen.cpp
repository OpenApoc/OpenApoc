#include "game/base/vequipscreen.h"
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
	fw.Stage_GetPrevious(this->shared_from_this())->Render();
	fw.renderer->drawFilledRect({0, 0}, fw.Display_GetSize(), Colour{0, 0, 0, 128});
	form->Render();
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
