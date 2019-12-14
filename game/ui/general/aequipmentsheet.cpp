#include "game/ui/general/aequipmentsheet.h"
#include "forms/graphic.h"
#include "forms/label.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/damage.h"

namespace OpenApoc
{

AEquipmentSheet::AEquipmentSheet(sp<Form> dstForm) : form(dstForm) {}

void AEquipmentSheet::display(sp<AEquipment> item, bool researched)
{
	clear();
	displayImplementation(item, *item->type, researched);
}

void AEquipmentSheet::display(const AEquipmentType &itemType, bool researched)
{
	clear();
	displayImplementation(nullptr, itemType, researched);
}

void AEquipmentSheet::clear()
{
	for (int i = 0; i < 10; i++)
	{
		for (char alignment : {'L', 'C', 'R'})
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

void AEquipmentSheet::displayImplementation(sp<AEquipment> item, const AEquipmentType &itemType,
                                            bool researched)
{
	if (!researched)
	{
		displayAlien(item, itemType);
		return;
	}

	form->findControlTyped<Label>("ITEM_NAME")->setText(itemType.name);
	form->findControlTyped<Graphic>("SELECTED_IMAGE")
	    ->setImage(item ? item->getEquipmentImage() : itemType.equipscreen_sprite);

	// when possible, the actual item's weight takes precedence
	form->findControlTyped<Label>("LABEL_1_L")->setText(tr("Weight"));
	form->findControlTyped<Label>("LABEL_1_R")
	    ->setText(format("%d", item ? item->getWeight() : itemType.weight));

	switch (itemType.type)
	{
		case AEquipmentType::Type::Grenade:
			displayGrenade(item, itemType);
			break;
		case AEquipmentType::Type::Ammo:
			displayAmmo(item, itemType);
			break;
		case AEquipmentType::Type::Weapon:
			if (itemType.ammo_types.empty()) // weapon with built-in ammo like stun grapple
			{
				displayAmmo(item, itemType);
			}
			else if (item && item->getPayloadType()) // weapon with equipped ammo
			{
				displayAmmo(item, *item->getPayloadType());
			}
			else if (itemType.ammo_types.size() ==
			         1) // weapon without ammo but a single ammo type like lawpistol
			{
				displayAmmo(item, **itemType.ammo_types.begin());
			}
			else
			{
				displayWeapon(
				    item,
				    itemType); // weapon without ammo but multiple ammo types like the autocannon
			}
			break;
		case AEquipmentType::Type::Armor:
			displayArmor(item, itemType);
			break;
		default:
			displayOther(item, itemType);
	}
}

void AEquipmentSheet::displayGrenade(sp<AEquipment> item [[maybe_unused]],
                                     const AEquipmentType &itemType)
{
	form->findControlTyped<Label>("LABEL_2_C")->setText(itemType.damage_type->name);

	form->findControlTyped<Label>("LABEL_3_L")->setText(tr("Power"));
	form->findControlTyped<Label>("LABEL_3_R")->setText(format("%d", itemType.damage));
}

void AEquipmentSheet::displayAmmo(sp<AEquipment> item, const AEquipmentType &itemType)
{
	form->findControlTyped<Label>("LABEL_2_L")->setText(tr("Accuracy"));
	form->findControlTyped<Label>("LABEL_2_R")->setText(format("%d", itemType.accuracy));

	form->findControlTyped<Label>("LABEL_3_L")->setText(tr("Fire rate"));
	form->findControlTyped<Label>("LABEL_3_R")
	    ->setText(format("%.2f", itemType.getRoundsPerSecond()));

	form->findControlTyped<Label>("LABEL_4_L")->setText(tr("Range"));
	form->findControlTyped<Label>("LABEL_4_R")->setText(format("%d", itemType.getRangeInTiles()));

	form->findControlTyped<Label>("LABEL_6_C")->setText(tr("Ammo Type:"));
	form->findControlTyped<Label>("LABEL_7_C")->setText(itemType.damage_type->name);

	form->findControlTyped<Label>("LABEL_8_L")->setText(tr("Power"));
	form->findControlTyped<Label>("LABEL_8_R")->setText(format("%d", itemType.damage));

	form->findControlTyped<Label>("LABEL_9_L")->setText(tr("Rounds"));
	form->findControlTyped<Label>("LABEL_9_R")
	    ->setText(item ? format("%d / %d", item->ammo, itemType.max_ammo)
	                   : format("%d", itemType.max_ammo));
	if (itemType.recharge > 0)
	{
		form->findControlTyped<Label>("LABEL_10_C")->setText(tr("(Recharges)"));
	}
}

void AEquipmentSheet::displayWeapon(sp<AEquipment> item [[maybe_unused]],
                                    const AEquipmentType &itemType)
{
	if (itemType.ammo_types.empty())
	{
		LogError("Ammo weapon without any ammo types?");
		return;
	}

	auto &ammoType = *itemType.ammo_types.begin();
	form->findControlTyped<Label>("LABEL_2_L")->setText(tr("Accuracy"));
	form->findControlTyped<Label>("LABEL_2_R")->setText(format("%d", ammoType->accuracy));

	form->findControlTyped<Label>("LABEL_3_L")->setText(tr("Fire rate"));
	form->findControlTyped<Label>("LABEL_3_R")
	    ->setText(format("%.2f", ammoType->getRoundsPerSecond()));

	form->findControlTyped<Label>("LABEL_4_L")->setText(tr("Range"));
	form->findControlTyped<Label>("LABEL_4_R")->setText(format("%d", ammoType->getRangeInTiles()));

	form->findControlTyped<Label>("LABEL_5_C")->setText(tr("Ammo types:"));
	int ammoNum = 1;
	for (auto &ammo : itemType.ammo_types)
	{
		form->findControlTyped<Label>(format("LABEL_%d_C", 5 + ammoNum))->setText(ammo->name);
		if (++ammoNum >= 4)
		{
			break;
		}
	}
}

void AEquipmentSheet::displayArmor(sp<AEquipment> item, const AEquipmentType &itemType)
{
	form->findControlTyped<Label>("LABEL_2_L")->setText(tr("Protection"));
	form->findControlTyped<Label>("LABEL_2_R")
	    ->setText(item ? format("%d / %d", item->armor, itemType.armor)
	                   : format("%d", itemType.armor));
}

void AEquipmentSheet::displayOther(sp<AEquipment> item [[maybe_unused]],
                                   const AEquipmentType &itemType [[maybe_unused]])
{
}

void AEquipmentSheet::displayAlien(sp<AEquipment> item, const AEquipmentType &itemType)
{
	form->findControlTyped<Label>("ITEM_NAME")
	    ->setText(itemType.bioStorage ? tr("Alien Organism") : tr("Alien Artifact"));
	form->findControlTyped<Graphic>("SELECTED_IMAGE")->setImage(itemType.equipscreen_sprite);

	form->findControlTyped<Label>("LABEL_1_L")->setText(tr("Weight"));
	form->findControlTyped<Label>("LABEL_1_R")
	    ->setText(format("%d", item ? item->getWeight() : itemType.weight));
}

}; // namespace OpenApoc
