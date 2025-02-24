#include "game/ui/city/vehiclecargobriefing.h"
#include <forms/form.h>
#include <forms/label.h>
#include <forms/listbox.h>
#include <forms/scrollbar.h>
#include <forms/ui.h>
#include <framework/event.h>
#include <framework/framework.h>

namespace OpenApoc
{

VehicleCargoBriefing::VehicleCargoBriefing(sp<Vehicle> vehicle)
    : Stage(), menuform(ui().getForm("city/vehicle_cargo"))
{
	this->vehicleCargo = vehicle->cargo;
	removeZeroCountItems(this->vehicleCargo);
	sortItems(this->vehicleCargo);
}

VehicleCargoBriefing::~VehicleCargoBriefing() = default;

void VehicleCargoBriefing::refreshListBoxes()
{
	auto listboxCargo = menuform->findControlTyped<ListBox>("cargoList");
	auto listboxPiece = menuform->findControlTyped<ListBox>("pieceList");
	listboxCargo->clear();
	listboxPiece->clear();
	for (const auto &item : this->vehicleCargo)
	{
		// name
		auto nameOfCargo = mksp<Label>(convertToText(item.id), ui().getFont("smalfont"));
		nameOfCargo->Size = {150, 15};
		nameOfCargo->TextHAlign = HorizontalAlignment::Left;
		nameOfCargo->TextVAlign = VerticalAlignment::Centre;

		// piece
		auto pieceOfCargo = mksp<Label>(std::to_string(item.count), ui().getFont("smalfont"));
		pieceOfCargo->Size = {150, 15};
		pieceOfCargo->TextHAlign = HorizontalAlignment::Centre;
		pieceOfCargo->TextVAlign = VerticalAlignment::Centre;

		// adding
		listboxCargo->addItem(nameOfCargo);
		listboxPiece->addItem(pieceOfCargo);
	}
	this->update();
	listboxCargo->scroller->scrollMin();
}
void VehicleCargoBriefing::sortItems(std::list<Cargo> &list, bool byName, bool asc)
{
	list.sort(
	    [byName, asc](const Cargo &itemA, const Cargo &itemB)
	    {
		    if (byName)
		    {
			    return asc ? itemA.id < itemB.id : itemA.id > itemB.id;
		    }
		    else
		    {
			    return asc ? itemA.count < itemB.count : itemA.count > itemB.count;
		    }
	    });
}
void VehicleCargoBriefing::removeZeroCountItems(std::list<Cargo> &list)
{
	list.remove_if(
	    [](const Cargo &item)
	    {
		    return item.count == 0; // Töröljük, ha Count 0
	    });
}
UString VehicleCargoBriefing::convertToText(const UString &item)
{
	UString output = "";
	auto sResult = split(item, "_");
	for (size_t i = 1; i < sResult.size(); i++)
	{
		for (size_t j = 0; j < sResult[i].size(); j++)
		{
			if (i == 1 && j == 0)
			{
				output += std::toupper(sResult[i][j]);
			}
			else
			{
				output += std::tolower(sResult[i][j]);
			}
		}
		output += " ";
	}

	return output;
}

// Stage control
void VehicleCargoBriefing::begin() { refreshListBoxes(); }
void VehicleCargoBriefing::pause() {}
void VehicleCargoBriefing::resume() {}
void VehicleCargoBriefing::finish() {}
void VehicleCargoBriefing::eventOccurred(Event *e)
{
	menuform->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		LogInfo("Leave Cargo Briefing form..");
		fw().stageQueueCommand({StageCmd::Command::POP});
		return;
	}

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().EventFlag == FormEventType::ButtonClick)
	{
		LogInfo("Cargo briefing button click..");
		if (e->forms().RaisedBy->Name == "BTN_Cargo_Name")
		{
			sortItems(this->vehicleCargo, true, this->asc ? false : true);
		}
		else if (e->forms().RaisedBy->Name == "BTN_Cargo_Piece")
		{
			sortItems(this->vehicleCargo, false, this->asc ? false : true);
		}

		this->asc = this->asc ? false : true;
		refreshListBoxes();
		return;
	}
}
void VehicleCargoBriefing::update() { menuform->update(); }
void VehicleCargoBriefing::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->render();
}
bool VehicleCargoBriefing::isTransition() { return false; }

}; // namespace OpenApoc