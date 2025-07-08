#include "game/ui/general/vehiclesheet.h"
#include "forms/graphic.h"
#include "forms/label.h"
#include "forms/textedit.h"
#include "framework/logger.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/damage.h"
#include "game/state/tilemap/tilemap.h"
#include "library/strings_format.h"
#include <framework/configfile.h>
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
	displayEquipImplementation(item, item->type, item->type->research_dependency.satisfied());
}

void VehicleSheet::display(sp<VEquipmentType> itemType)
{
	clear();
	displayEquipImplementation(nullptr, itemType, itemType->research_dependency.satisfied());
}

void VehicleSheet::display(sp<VEquipmentType> itemType, bool researched)
{
	clear();
	displayEquipImplementation(nullptr, itemType, researched);
}

void VehicleSheet::clear()
{
	for (int i = 0; i < 10; i++)
	{
		for (char alignment : {'L', 'R'})
		{
			auto labelName = fmt::format("LABEL_{}_{}", i + 1, alignment);
			auto label = form->findControlTyped<Label>(labelName);
			if (!label)
			{
				LogError("Failed to find UI control matching \"{}\"", labelName);
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
	    ->setText(vehicle ? fmt::format("{} / {}", vehicle->getConstitution(),
	                                    vehicle->getMaxConstitution())
	                      : fmt::format("{}", vehicleType->getMaxConstitution(it1, it2)));

	form->findControlTyped<Label>("LABEL_2_R")
	    ->setText(
	        fmt::format("{}", vehicle ? vehicle->getArmor() : vehicleType->getArmor(it1, it2)));

	form->findControlTyped<Label>("LABEL_3_R")
	    ->setText(fmt::format("{}%", vehicle ? vehicle->getAccuracy()
	                                         : vehicleType->getAccuracy(it1, it2)));
	form->findControlTyped<Label>("LABEL_4_R")
	    ->setText(fmt::format("{}", vehicle ? vehicle->getTopSpeed()
	                                        : vehicleType->getTopSpeed(it1, it2)));
	form->findControlTyped<Label>("LABEL_5_R")
	    ->setText(fmt::format("{}", vehicle ? vehicle->getAcceleration()
	                                        : vehicleType->getAcceleration(it1, it2)));
	form->findControlTyped<Label>("LABEL_6_R")
	    ->setText(
	        fmt::format("{}", vehicle ? vehicle->getWeight() : vehicleType->getWeight(it1, it2)));
	form->findControlTyped<Label>("LABEL_7_R")
	    ->setText(vehicle ? fmt::format("{}k / {}k", vehicle->getFuel(), vehicle->getMaxFuel())
	                      : fmt::format("{}k", vehicleType->getMaxFuel(it1, it2)));
	form->findControlTyped<Label>("LABEL_8_R")
	    ->setText(
	        vehicle ? fmt::format("{} / {}", vehicle->getPassengers(), vehicle->getMaxPassengers())
	                : fmt::format("{}", vehicleType->getMaxPassengers(it1, it2)));
	if (!config().getBool("OpenApoc.NewFeature.EnforceCargoLimits"))
	{
		form->findControlTyped<Label>("LABEL_9_R")
		    ->setText(vehicle ? fmt::format("{}", vehicle->getCargo())
		                      : fmt::format("{}", vehicleType->getMaxCargo(it1, it2)));
	}
	else
	{
		form->findControlTyped<Label>("LABEL_9_R")
		    ->setText(vehicle ? fmt::format("{} / {}", vehicle->getCargo(), vehicle->getMaxCargo())
		                      : fmt::format("{}", vehicleType->getMaxCargo(it1, it2)));
	}
}

void VehicleSheet::displayEquipImplementation(sp<VEquipment> item, sp<VEquipmentType> type,
                                              const bool isResearched)
{
	form->findControlTyped<Graphic>("SELECTED_IMAGE")->setImage(type->equipscreen_sprite);

	form->findControlTyped<Label>("LABEL_1_L")->setText(tr("Weight"));
	form->findControlTyped<Label>("LABEL_1_R")->setText(fmt::format("{}", type->weight));

	form->findControlTyped<Label>("LABEL_2_L")->setText(tr("Storage"));
	form->findControlTyped<Label>("LABEL_2_R")->setText(fmt::format("{}", type->store_space));

	form->findControlTyped<TextEdit>("TEXT_VEHICLE_NAME")->setText("");

	if (!isResearched)
	{
		form->findControlTyped<Label>("ITEM_NAME")->setText(tr("Alien Artifact"));
		return;
	}

	form->findControlTyped<Label>("ITEM_NAME")->setText(item ? item->type->name : type->name);

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
		default:
			LogError("Unhandled equipment type {} on vehicle", type->id);
			break;
	}
}

void VehicleSheet::displayEngine(sp<VEquipment> item [[maybe_unused]], sp<VEquipmentType> type)
{
	form->findControlTyped<Label>("LABEL_3_L")->setText(tr("Top Speed"));
	form->findControlTyped<Label>("LABEL_3_R")->setText(fmt::format("{}", type->top_speed));
	form->findControlTyped<Label>("LABEL_4_L")->setText(tr("Power"));
	form->findControlTyped<Label>("LABEL_4_R")->setText(fmt::format("{}", type->power));
}

void VehicleSheet::displayWeapon(sp<VEquipment> item, sp<VEquipmentType> type)
{
	form->findControlTyped<Label>("LABEL_3_L")->setText(tr("Damage"));
	form->findControlTyped<Label>("LABEL_3_R")->setText(fmt::format("{}", type->damage));
	form->findControlTyped<Label>("LABEL_4_L")->setText(tr("Range"));
	form->findControlTyped<Label>("LABEL_4_R")->setText(fmt::format("{}", type->getRangeInTiles()));
	form->findControlTyped<Label>("LABEL_5_L")->setText(tr("Accuracy"));
	form->findControlTyped<Label>("LABEL_5_R")->setText(fmt::format("{}%", type->accuracy));

	// Only show rounds if non-zero (IE not infinite ammo)
	if (type->max_ammo != 0)
	{
		form->findControlTyped<Label>("LABEL_5_L")->setText(tr("Rounds"));
		form->findControlTyped<Label>("LABEL_5_R")
		    ->setText(item ? fmt::format("{} / {}", item->ammo, type->max_ammo)
		                   : fmt::format("{}", type->max_ammo));
	}
}

void VehicleSheet::displayGeneral(sp<VEquipment> item [[maybe_unused]], sp<VEquipmentType> type)
{
	int statsCount = 3;
	if (type->accuracy_modifier)
	{
		form->findControlTyped<Label>(fmt::format("LABEL_{}_L", statsCount))
		    ->setText(tr("Accuracy"));
		form->findControlTyped<Label>(fmt::format("LABEL_{}_R", statsCount))
		    ->setText(fmt::format("{}%", 100 - type->accuracy_modifier));
		statsCount++;
	}
	if (type->cargo_space)
	{
		form->findControlTyped<Label>(fmt::format("LABEL_{}_L", statsCount))->setText(tr("Cargo"));
		form->findControlTyped<Label>(fmt::format("LABEL_{}_R", statsCount))
		    ->setText(fmt::format("{}", type->cargo_space));
		statsCount++;
	}
	if (type->passengers)
	{
		form->findControlTyped<Label>(fmt::format("LABEL_{}_L", statsCount))
		    ->setText(tr("Passengers"));
		form->findControlTyped<Label>(fmt::format("LABEL_{}_R", statsCount))
		    ->setText(fmt::format("{}", type->passengers));
		statsCount++;
	}
	if (type->alien_space)
	{
		form->findControlTyped<Label>(fmt::format("LABEL_{}_L", statsCount))
		    ->setText(tr("Max Samples"));
		form->findControlTyped<Label>(fmt::format("LABEL_{}_R", statsCount))
		    ->setText(fmt::format("{}", type->alien_space));
		statsCount++;
	}
	if (type->missile_jamming)
	{
		form->findControlTyped<Label>(fmt::format("LABEL_{}_L", statsCount))
		    ->setText(tr("Jamming"));
		form->findControlTyped<Label>(fmt::format("LABEL_{}_R", statsCount))
		    ->setText(fmt::format("{}", type->missile_jamming));
		statsCount++;
	}
	if (type->shielding)
	{
		form->findControlTyped<Label>(fmt::format("LABEL_{}_L", statsCount))
		    ->setText(tr("Shielding"));
		form->findControlTyped<Label>(fmt::format("LABEL_{}_R", statsCount))
		    ->setText(fmt::format("{}", type->shielding));
		statsCount++;
	}
	if (type->cloaking)
	{
		form->findControlTyped<Label>(fmt::format("LABEL_{}_L", statsCount))
		    ->setText(tr("Cloaks Craft"));
		statsCount++;
	}
	if (type->teleporting)
	{
		form->findControlTyped<Label>(fmt::format("LABEL_{}_L", statsCount))
		    ->setText(tr("Teleports"));
		statsCount++;
	}
}

}; // namespace OpenApoc
