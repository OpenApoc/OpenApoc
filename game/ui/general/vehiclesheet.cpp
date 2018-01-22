#include "game/ui/general/vehiclesheet.h"
#include "forms/graphic.h"
#include "forms/label.h"
#include "forms/textedit.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/damage.h"
#include "game/state/tilemap/tilemap.h"
#include <list>

namespace OpenApoc
{

VehicleSheet::VehicleSheet(sp<Form> destForm) : form(destForm) {}

void VehicleSheet::display(sp<Vehicle> vehicle)
{
	clear();
	displayImplementation(vehicle, vehicle->type);
}

void VehicleSheet::display(sp<VehicleType> vehicleType)
{
	clear();
	displayImplementation(nullptr, vehicleType);
}

void VehicleSheet::display(sp<VEquipment> item)
{
	clear();
	displayEquipImplementation(item, item->type);
}

void VehicleSheet::display(sp<VEquipmentType> itemType)
{
	clear();
	displayEquipImplementation(nullptr, itemType);
}

void VehicleSheet::clear()
{
	for (int i = 0; i < 10; i++)
	{
		for (char alignment : {'L', 'R'})
		{
			auto labelName = format("LABEL_%d_%c", i + 1, alignment);
			auto label = form->findControlTyped<Label>(labelName);
			if (!label)
			{
				LogError("Failed to find UI control matching \"%s\"", labelName);
			}
			else
			{
				label->setText("");
			}
		}
	}
}

void VehicleSheet::displayImplementation(sp<Vehicle> vehicle, sp<VehicleType> vehicleType)
{
	form->findControlTyped<Label>("ITEM_NAME")->setText("");
	form->findControlTyped<TextEdit>("TEXT_VEHICLE_NAME")
	    ->setText(vehicle ? vehicle->name : vehicleType->name);
	form->findControlTyped<Graphic>("SELECTED_IMAGE")->setImage(vehicleType->equip_icon_small);

	form->findControlTyped<Label>("LABEL_1_L")->setText(tr("Constitution"));
	form->findControlTyped<Label>("LABEL_2_L")->setText(tr("Armor"));
	form->findControlTyped<Label>("LABEL_3_L")->setText(tr("Accuracy"));
	form->findControlTyped<Label>("LABEL_4_L")->setText(tr("Top Speed"));
	form->findControlTyped<Label>("LABEL_5_L")->setText(tr("Acceleration"));
	form->findControlTyped<Label>("LABEL_6_L")->setText(tr("Weight"));
	form->findControlTyped<Label>("LABEL_7_L")->setText(tr("Fuel"));
	form->findControlTyped<Label>("LABEL_8_L")->setText(tr("Passengers"));
	form->findControlTyped<Label>("LABEL_9_L")->setText(tr("Cargo"));

	std::list<sp<VEquipmentType>> defaultEquipment;
	for (auto &e : vehicleType->initial_equipment_list)
	{
		defaultEquipment.push_back(e.second.getSp());
	}
	auto it1 = defaultEquipment.begin();
	auto it2 = defaultEquipment.end();

	form->findControlTyped<Label>("LABEL_1_R")
	    ->setText(vehicle
	                  ? format("%d / %d", vehicle->getConstitution(), vehicle->getMaxConstitution())
	                  : format("%d", vehicleType->getMaxConstitution(it1, it2)));

	form->findControlTyped<Label>("LABEL_2_R")
	    ->setText(format("%d", vehicle ? vehicle->getArmor() : vehicleType->getArmor(it1, it2)));

	form->findControlTyped<Label>("LABEL_3_R")
	    ->setText(
	        format("%d%%", vehicle ? vehicle->getAccuracy() : vehicleType->getAccuracy(it1, it2)));
	form->findControlTyped<Label>("LABEL_4_R")
	    ->setText(
	        format("%d", vehicle ? vehicle->getTopSpeed() : vehicleType->getTopSpeed(it1, it2)));
	form->findControlTyped<Label>("LABEL_5_R")
	    ->setText(format(
	        "%d", vehicle ? vehicle->getAcceleration() : vehicleType->getAcceleration(it1, it2)));
	form->findControlTyped<Label>("LABEL_6_R")
	    ->setText(format("%d", vehicle ? vehicle->getWeight() : vehicleType->getWeight(it1, it2)));
	form->findControlTyped<Label>("LABEL_7_R")
	    ->setText(vehicle ? format("%dk / %dk", vehicle->getFuel(), vehicle->getMaxFuel())
	                      : format("%dk", vehicleType->getMaxFuel(it1, it2)));
	form->findControlTyped<Label>("LABEL_8_R")
	    ->setText(vehicle ? format("%d / %d", vehicle->getPassengers(), vehicle->getMaxPassengers())
	                      : format("%d", vehicleType->getMaxPassengers(it1, it2)));
	form->findControlTyped<Label>("LABEL_9_R")
	    ->setText(vehicle ? format("%d / %d", vehicle->getCargo(), vehicle->getMaxCargo())
	                      : format("%d", vehicleType->getMaxCargo(it1, it2)));
}

void VehicleSheet::displayEquipImplementation(sp<VEquipment> item, sp<VEquipmentType> type)
{
	form->findControlTyped<TextEdit>("TEXT_VEHICLE_NAME")->setText("");
	form->findControlTyped<Label>("ITEM_NAME")->setText(item ? item->type->name : type->name);
	form->findControlTyped<Graphic>("SELECTED_IMAGE")->setImage(type->equipscreen_sprite);

	form->findControlTyped<Label>("LABEL_1_L")->setText(tr("Weight"));
	form->findControlTyped<Label>("LABEL_1_R")->setText(format("%d", type->weight));

	// Draw equipment stats
	switch (type->type)
	{
		case EquipmentSlotType::VehicleEngine:
			displayEngine(item, type);
			break;
		case EquipmentSlotType::VehicleWeapon:
			displayWeapon(item, type);
			break;
		case EquipmentSlotType::VehicleGeneral:
			displayGeneral(item, type);
			break;
	}
}

void VehicleSheet::displayEngine(sp<VEquipment> item, sp<VEquipmentType> type)
{
	form->findControlTyped<Label>("LABEL_2_L")->setText(tr("Top Speed"));
	form->findControlTyped<Label>("LABEL_2_R")->setText(format("%d", type->top_speed));
	form->findControlTyped<Label>("LABEL_3_L")->setText(tr("Power"));
	form->findControlTyped<Label>("LABEL_3_R")->setText(format("%d", type->power));
}

void VehicleSheet::displayWeapon(sp<VEquipment> item, sp<VEquipmentType> type)
{
	form->findControlTyped<Label>("LABEL_2_L")->setText(tr("Damage"));
	form->findControlTyped<Label>("LABEL_2_R")->setText(format("%d", type->damage));
	form->findControlTyped<Label>("LABEL_3_L")->setText(tr("Range"));
	form->findControlTyped<Label>("LABEL_3_R")
	    ->setText(format("%d", type->range / (int)VELOCITY_SCALE_CITY.x));
	form->findControlTyped<Label>("LABEL_4_L")->setText(tr("Accuracy"));
	form->findControlTyped<Label>("LABEL_4_R")->setText(format("%d%%", type->accuracy));

	// Only show rounds if non-zero (IE not infinite ammo)
	if (type->max_ammo != 0)
	{
		form->findControlTyped<Label>("LABEL_5_L")->setText(tr("Rounds"));
		form->findControlTyped<Label>("LABEL_5_R")
		    ->setText(item ? format("%d / %d", item->ammo, type->max_ammo)
		                   : format("%d", type->max_ammo));
	}
}

void VehicleSheet::displayGeneral(sp<VEquipment> item, sp<VEquipmentType> type)
{
	int statsCount = 2;
	if (type->accuracy_modifier)
	{
		form->findControlTyped<Label>(format("LABEL_%d_L", statsCount))->setText(tr("Accuracy"));
		form->findControlTyped<Label>(format("LABEL_%d_R", statsCount))
		    ->setText(format("%d%%", 100 - type->accuracy_modifier));
		statsCount++;
	}
	if (type->cargo_space)
	{
		form->findControlTyped<Label>(format("LABEL_%d_L", statsCount))->setText(tr("Cargo"));
		form->findControlTyped<Label>(format("LABEL_%d_R", statsCount))
		    ->setText(format("%d", type->cargo_space));
		statsCount++;
	}
	if (type->passengers)
	{
		form->findControlTyped<Label>(format("LABEL_%d_L", statsCount))->setText(tr("Passengers"));
		form->findControlTyped<Label>(format("LABEL_%d_R", statsCount))
		    ->setText(format("%d", type->passengers));
		statsCount++;
	}
	if (type->alien_space)
	{
		form->findControlTyped<Label>(format("LABEL_%d_L", statsCount))->setText(tr("Aliens Held"));
		form->findControlTyped<Label>(format("LABEL_%d_R", statsCount))
		    ->setText(format("%d", type->alien_space));
		statsCount++;
	}
	if (type->missile_jamming)
	{
		form->findControlTyped<Label>(format("LABEL_%d_L", statsCount))->setText(tr("Jamming"));
		form->findControlTyped<Label>(format("LABEL_%d_R", statsCount))
		    ->setText(format("%d", type->missile_jamming));
		statsCount++;
	}
	if (type->shielding)
	{
		form->findControlTyped<Label>(format("LABEL_%d_L", statsCount))->setText(tr("Shielding"));
		form->findControlTyped<Label>(format("LABEL_%d_R", statsCount))
		    ->setText(format("%d", type->shielding));
		statsCount++;
	}
	if (type->cloaking)
	{
		form->findControlTyped<Label>(format("LABEL_%d_L", statsCount))
		    ->setText(tr("Cloaks Craft"));
		statsCount++;
	}
	if (type->teleporting)
	{
		form->findControlTyped<Label>(format("LABEL_%d_L", statsCount))->setText(tr("Teleports"));
		statsCount++;
	}
}

}; // namespace OpenApoc
