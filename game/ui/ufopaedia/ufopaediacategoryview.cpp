#include "game/ui/ufopaedia/ufopaediacategoryview.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/label.h"
#include "forms/listbox.h"
#include "forms/textbutton.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/font.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/damage.h"
#include "game/state/rules/city/facilitytype.h"
#include "game/state/rules/city/ufopaedia.h"
#include "game/state/rules/city/vammotype.h"
#include "game/state/rules/city/vehicletype.h"
#include "game/state/rules/city/vequipmenttype.h"
#include "game/state/shared/organisation.h"
#include "library/sp.h"
#include "library/strings_format.h"

namespace OpenApoc
{

UfopaediaCategoryView::UfopaediaCategoryView(sp<GameState> state, sp<UfopaediaCategory> cat,
                                             sp<UfopaediaEntry> entry)
    : Stage(), menuform(ui().getForm("ufopaedia")), state(state), category(cat)
{
	// Start with the intro page
	this->position_iterator = this->category->entries.end();
	if (entry)
	{
		auto it = cat->entries.begin();
		while (it != cat->entries.end())
		{
			if (it->second == entry)
			{
				position_iterator = it;
				break;
			}
			it++;
		}
		if (it == cat->entries.end())
		{
			LogError("Failed to find UFOpaedia entry %s in category %s", entry->title, cat->title);
		}
	}
}

UfopaediaCategoryView::~UfopaediaCategoryView() = default;

void UfopaediaCategoryView::begin()
{
	auto infoLabel = menuform->findControlTyped<Label>("TEXT_INFO");
	auto entryList = menuform->findControlTyped<ListBox>("LISTBOX_SHORTCUTS");
	entryList->clear();
	entryList->ItemSize = infoLabel->getFont()->getFontHeight() + 2;
	for (auto &pair : this->category->entries)
	{
		auto entry = pair.second;
		// Skip non-visible entries
		if (!entry->isVisible())
		{
			continue;
		}

		auto entryControl = mksp<TextButton>(tr(entry->title), infoLabel->getFont());
		entryControl->Name = "ENTRY_SHORTCUT";
		entryControl->RenderStyle = TextButton::ButtonRenderStyle::Flat;
		entryControl->TextHAlign = HorizontalAlignment::Left;
		entryControl->TextVAlign = VerticalAlignment::Centre;
		entryControl->setData(entry);
		entryList->addItem(entryControl);
	}
	baseY = infoLabel->Location.y;
	for (int i = 0; i < 9; i++)
	{
		auto labelName = format("LABEL_%d", i + 1);
		auto label = menuform->findControlTyped<Label>(labelName);
		if (!label)
		{
			LogError("Failed to find UI control matching \"%s\"", labelName);
		}
		label->setText("");
		statsLabels.push_back(label);

		auto valueName = format("VALUE_%d", i + 1);
		auto value = menuform->findControlTyped<Label>(valueName);
		if (!value)
		{
			LogError("Failed to find UI control matching \"%s\"", valueName);
		}
		value->setText("");
		statsValues.push_back(value);
	}
	for (int i = 0; i < 4; i++)
	{
		auto labelName = format("ORG_LABEL_%d", i + 1);
		auto label = menuform->findControlTyped<Label>(labelName);
		if (!label)
		{
			LogError("Failed to find UI control matching \"%s\"", labelName);
		}
		label->setText("");
		orgLabels.push_back(label);

		auto valueName = format("ORG_VALUE_%d", i + 1);
		auto value = menuform->findControlTyped<Label>(valueName);
		if (!value)
		{
			LogError("Failed to find UI control matching \"%s\"", valueName);
		}
		value->setText("");
		orgValues.push_back(value);
	}
	this->setFormData();
}

void UfopaediaCategoryView::pause() {}

void UfopaediaCategoryView::resume() {}

void UfopaediaCategoryView::finish() {}

void UfopaediaCategoryView::eventOccurred(Event *e)
{
	menuform->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		switch (e->keyboard().KeyCode)
		{
			case SDLK_ESCAPE:
			case SDLK_RETURN:
			case SDLK_KP_ENTER:
				menuform->findControl("BUTTON_QUIT")->click();
				return;
			case SDLK_UP:
				setNextSection();
				return;
			case SDLK_DOWN:
				setPreviousSection();
				return;
			case SDLK_RIGHT:
				setNextTopic();
				return;
			case SDLK_LEFT:
				setPreviousTopic();
				return;
			default:
				break;
		}
	}

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().EventFlag == FormEventType::ButtonClick)
	{
		if (e->forms().RaisedBy->Name == "BUTTON_QUIT")
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
			return;
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_NEXT_TOPIC")
		{
			setNextTopic();
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_PREVIOUS_TOPIC")
		{
			setPreviousTopic();
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_NEXT_SECTION")
		{
			setNextSection();
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_PREVIOUS_SECTION")
		{
			setPreviousSection();
		}
		else if (e->forms().RaisedBy->Name == "ENTRY_SHORTCUT")
		{
			auto entry = e->forms().RaisedBy->getData<UfopaediaEntry>();
			if (!entry)
			{
				LogError("Invalid UfopaediaEntry in shortcut control");
			}
			auto it = this->category->entries.begin();
			// Find the entry iterator
			while (it->second != entry)
			{
				it++;
				if (it == this->category->entries.end())
				{
					LogError("Failed to find current category \"%s\"", this->category->title);
				}
			}
			this->position_iterator = it;
			this->setFormData();
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_INFORMATION")
		{
			menuform->findControl("INFORMATION_PANEL")
			    ->setVisible(!menuform->findControl("INFORMATION_PANEL")->isVisible());
			return;
		}
	}
}

void UfopaediaCategoryView::update() { menuform->update(); }

void UfopaediaCategoryView::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->render();
}

void UfopaediaCategoryView::setFormData()
{
	UString title;
	UString description;
	sp<Image> background;
	if (this->position_iterator == this->category->entries.end())
	{
		title = category->title;
		description = category->description;
		background = category->background->getRealImage();
	}
	else
	{
		title = this->position_iterator->second->title;
		description = this->position_iterator->second->description;
		background = this->position_iterator->second->background->getRealImage();
	}
	menuform->findControlTyped<Graphic>("BACKGROUND_PICTURE")->setImage(background);
	auto tr_description = tr(description);
	auto tr_title = tr(title);
	menuform->findControlTyped<Label>("TEXT_INFO")->setText(tr_description);
	menuform->findControlTyped<Label>("TEXT_TITLE_DATA")->setText(tr_title);

	// Every time you we change the entry reset the info panel
	menuform->findControl("INFORMATION_PANEL")->setVisible(false);

	setFormStats();
}

void UfopaediaCategoryView::setFormStats()
{
	for (unsigned int i = 0; i < statsLabels.size(); i++)
	{
		statsLabels[i]->setText("");
		statsValues[i]->setText("");
	}
	for (unsigned int i = 0; i < orgLabels.size(); i++)
	{
		orgLabels[i]->setText("");
		orgValues[i]->setText("");
	}
	unsigned int row = 0;
	if (this->position_iterator != this->category->entries.end())
	{
		UString data_id = this->position_iterator->second->data_id;
		UfopaediaEntry::Data data_type = this->position_iterator->second->data_type;
		if (data_id.length() > 0)
		{
			switch (data_type)
			{
				case UfopaediaEntry::Data::Organisation:
				{
					StateRef<Organisation> ref = {state.get(), data_id};
					StateRef<Organisation> player = state->getPlayer();
					// FIXME: Should this be hardcoded?
					if (data_id != "ORG_ALIEN")
					{
						orgLabels[1]->setText(tr("Balance"));
						orgValues[1]->setText(format("$%d", ref->balance));
						orgLabels[2]->setText(tr("Income"));
						orgValues[2]->setText(format("$%d", ref->income));

						if (ref != player)
						{
							UString relation = tr(ref->name);
							switch (ref->isRelatedTo(player))
							{
								case Organisation::Relation::Allied:
									relation += tr(": allied towards:");
									break;
								case Organisation::Relation::Friendly:
									relation += tr(": friendly towards:");
									break;
								case Organisation::Relation::Neutral:
									relation += tr(": neutral towards:");
									break;
								case Organisation::Relation::Unfriendly:
									relation += tr(": unfriendly towards:");
									break;
								case Organisation::Relation::Hostile:
									relation += tr(": hostile towards:");
									break;
							}
							relation += UString(" ") + tr(player->name);
							orgLabels[0]->setText(relation);
							orgLabels[3]->setText(tr("Alien Infiltration"));
							orgValues[3]->setText(format("%d%%", ref->infiltrationValue / 2));
						}
					}
					break;
				}
				case UfopaediaEntry::Data::Vehicle:
				{
					StateRef<VehicleType> ref = {state.get(), data_id};
					statsLabels[row]->setText(tr("Constitution"));
					statsValues[row++]->setText(Strings::fromInteger(ref->health));
					int armour = 0;
					for (auto &slot : ref->armour)
					{
						armour += slot.second;
					}
					statsLabels[row]->setText(tr("Armor"));
					statsValues[row++]->setText(Strings::fromInteger(armour));
					statsLabels[row]->setText(tr("Weight"));
					statsValues[row++]->setText(Strings::fromInteger(ref->weight));
					statsLabels[row]->setText(tr("Passengers"));
					statsValues[row++]->setText(Strings::fromInteger(ref->passengers));
					int weaponSpace = 0;
					int weaponAmount = 0;
					int engineSpace = 0;
					int generalSpace = 0;
					for (auto &slot : ref->equipment_layout_slots)
					{
						int space = slot.bounds.size().x * slot.bounds.size().y;
						switch (slot.type)
						{
							case EquipmentSlotType::VehicleEngine:
								engineSpace += space;
								break;
							case EquipmentSlotType::VehicleWeapon:
								weaponSpace += space;
								weaponAmount++;
								break;
							case EquipmentSlotType::VehicleGeneral:
								generalSpace += space;
								break;
							default:
								break;
						}
					}
					statsLabels[row]->setText(tr("Weapons space"));
					statsValues[row++]->setText(Strings::fromInteger(weaponSpace));
					statsLabels[row]->setText(tr("Weapons slots"));
					statsValues[row++]->setText(Strings::fromInteger(weaponAmount));
					statsLabels[row]->setText(tr("Engine size"));
					statsValues[row++]->setText(Strings::fromInteger(engineSpace));
					statsLabels[row]->setText(tr("Equipment space"));
					statsValues[row++]->setText(Strings::fromInteger(generalSpace));
					break;
				}
				case UfopaediaEntry::Data::VehicleEquipment:
				{
					StateRef<VEquipmentType> ref = {state.get(), data_id};
					statsLabels[row]->setText(tr("Weight"));
					statsValues[row++]->setText(Strings::fromInteger(ref->weight));
					statsLabels[row]->setText(tr("Size"));
					statsValues[row++]->setText(
					    format("%dx%d", ref->equipscreen_size.x, ref->equipscreen_size.y));
					switch (ref->type)
					{
						case EquipmentSlotType::VehicleEngine:
							statsLabels[row]->setText(tr("Power"));
							statsValues[row++]->setText(Strings::fromInteger(ref->power));
							statsLabels[row]->setText(tr("Top Speed"));
							statsValues[row++]->setText(Strings::fromInteger(ref->top_speed));
							break;
						case EquipmentSlotType::VehicleWeapon:
							statsLabels[row]->setText(tr("Damage"));
							statsValues[row++]->setText(Strings::fromInteger(ref->damage));
							statsLabels[row]->setText(tr("Accuracy"));
							statsValues[row++]->setText(format("%d%%", ref->accuracy));
							statsLabels[row]->setText(tr("Range"));
							statsValues[row++]->setText(format("%dm", ref->getRangeInMetres()));
							statsLabels[row]->setText(tr("Fire Rate"));
							statsValues[row++]->setText(format(
							    "%.2f r/s", (float)TICKS_PER_SECOND / (float)ref->fire_delay));
							if (ref->max_ammo > 0 && ref->ammo_type)
							{
								statsLabels[row]->setText(tr("Ammo type"));
								statsValues[row++]->setText(tr(ref->ammo_type->name));
								statsLabels[row]->setText(tr("Ammo capacity"));
								statsValues[row++]->setText(Strings::fromInteger(ref->max_ammo));
							}
							if (ref->turn_rate > 0)
							{
								statsLabels[row]->setText(tr("Turn Rate"));
								statsValues[row++]->setText(Strings::fromInteger(ref->turn_rate));
							}
							break;
						case EquipmentSlotType::VehicleGeneral:
							if (ref->accuracy_modifier > 0)
							{
								statsLabels[row]->setText(tr("Accuracy"));
								statsValues[row++]->setText(
								    format("+%d%%", ref->accuracy_modifier));
							}
							if (ref->cargo_space > 0)
							{
								statsLabels[row]->setText(tr("Cargo"));
								statsValues[row++]->setText(Strings::fromInteger(ref->cargo_space));
							}
							if (ref->passengers > 0)
							{
								statsLabels[row]->setText(tr("Passengers"));
								statsValues[row++]->setText(Strings::fromInteger(ref->passengers));
							}
							if (ref->alien_space > 0)
							{
								statsLabels[row]->setText(tr("Aliens Held"));
								statsValues[row++]->setText(Strings::fromInteger(ref->alien_space));
							}
							if (ref->missile_jamming > 0)
							{
								statsLabels[row]->setText(tr("Jamming"));
								statsValues[row++]->setText(format("%d%%", ref->missile_jamming));
							}
							if (ref->shielding > 0)
							{
								statsLabels[row]->setText(tr("Shielding"));
								statsValues[row++]->setText(format("+%d", ref->shielding));
							}
							if (ref->cloaking)
							{
								statsValues[row++]->setText(tr("Cloaks Craft"));
							}
							break;
						default:
						{
							LogError("Trying to read non-vehicle equipment type %s on vehicle "
							         "equipment ufopaedia page",
							         ref->id);
							break;
						}
					}
					break;
				}
				case UfopaediaEntry::Data::Equipment:
				{
					StateRef<AEquipmentType> ref = {state.get(), data_id};
					statsLabels[row]->setText(tr("Weight"));
					statsValues[row++]->setText(Strings::fromInteger(ref->weight));
					statsLabels[row]->setText(tr("Size"));
					statsValues[row++]->setText(
					    format("%dx%d", ref->equipscreen_size.x, ref->equipscreen_size.y));
					if (ref->type == AEquipmentType::Type::Ammo ||
					    (ref->type == AEquipmentType::Type::Weapon && ref->ammo_types.empty()))
					{
						statsLabels[row]->setText(tr("Power"));
						statsValues[row++]->setText(Strings::fromInteger(ref->damage));
						statsLabels[row]->setText(tr("Damage Type"));
						statsValues[row++]->setText(ref->damage_type->name);
						statsLabels[row]->setText("Range");
						statsValues[row++]->setText(format("%dm", ref->getRangeInMetres()));
						statsLabels[row]->setText("Fire Rate");
						statsValues[row++]->setText(format("%.2f r/s", ref->getRoundsPerSecond()));
					}
					else if (ref->type == AEquipmentType::Type::Grenade)
					{
						statsLabels[row]->setText(tr("Power"));
						statsValues[row++]->setText(Strings::fromInteger(ref->damage));
						statsLabels[row]->setText(tr("Damage Type"));
						statsValues[row++]->setText(ref->damage_type->name);
					}
					else if (ref->type == AEquipmentType::Type::Weapon &&
					         ref->ammo_types.size() == 1)
					{
						const auto &ammoType = *ref->ammo_types.begin();
						statsLabels[row]->setText(tr("Power"));
						statsValues[row++]->setText(Strings::fromInteger(ammoType->damage));
						statsLabels[row]->setText(tr("Damage Type"));
						statsValues[row++]->setText(ammoType->damage_type->name);
						statsLabels[row]->setText(tr("Range"));
						statsValues[row++]->setText(format("%dm", ammoType->getRangeInMetres()));
						statsLabels[row]->setText(tr("Fire Rate"));
						statsValues[row++]->setText(
						    format("%.2f r/s", ammoType->getRoundsPerSecond()));
					}
					else if (ref->type == AEquipmentType::Type::Weapon &&
					         ref->ammo_types.size() > 1)
					{
						statsLabels[row]->setText(tr("Power"));
						statsValues[row++]->setText(tr("Depends on clip"));
						statsLabels[row]->setText(tr("Damage Type"));
						statsValues[row++]->setText(tr("Depends on clip"));
						statsLabels[row]->setText(tr("Range"));
						statsValues[row++]->setText(tr("Depends on clip"));
						statsLabels[row]->setText(tr("Fire Rate"));
						statsValues[row++]->setText(tr("Depends on clip"));
					}
					else if (ref->type == AEquipmentType::Type::Armor)
					{
						// FIXME: Not implemented
					}
					break;
				}
				case UfopaediaEntry::Data::Facility:
				{
					StateRef<FacilityType> ref = {state.get(), data_id};
					statsLabels[row]->setText(tr("Construction cost"));
					statsValues[row++]->setText(format("$%d", ref->buildCost));
					statsLabels[row]->setText(tr("Days to build"));
					statsValues[row++]->setText(Strings::fromInteger(ref->buildTime));
					statsLabels[row]->setText(tr("Weekly cost"));
					statsValues[row++]->setText(format("$%d", ref->weeklyCost));
					if (ref->capacityAmount > 0)
					{
						statsLabels[row]->setText(tr("Capacity"));
						statsValues[row++]->setText(Strings::fromInteger(ref->capacityAmount));
					}
					break;
				}
				case UfopaediaEntry::Data::Building:
				{
					LogError("Building not implemented yet");
					// FIXME: Not implemented yet
					break;
				}
				default:
					break;
			}
		}
	}
	if (row > 0)
	{
		row--;
		int y = statsLabels[row]->Location.y;
		y += statsLabels[row]->Size.y * 2;
		menuform->findControlTyped<Label>("TEXT_INFO")->Location.y = y;
	}
	else
	{
		menuform->findControlTyped<Label>("TEXT_INFO")->Location.y = baseY;
	}
}

void UfopaediaCategoryView::setNextTopic()
{
	do
	{
		if (this->position_iterator == this->category->entries.end())
		{
			this->position_iterator = this->category->entries.begin();
		}
		else
		{
			this->position_iterator++;
		}
		// Loop until we find the end (which shows the category intro screen)
		// or a visible entry
	} while (this->position_iterator != this->category->entries.end() &&
	         !this->position_iterator->second->isVisible());
	this->setFormData();
	return;
}

void UfopaediaCategoryView::setPreviousTopic()
{
	do
	{
		if (this->position_iterator == this->category->entries.begin())
		{
			this->position_iterator = this->category->entries.end();
		}
		else
		{
			this->position_iterator--;
		}
		// Loop until we find the end (which shows the category intro screen)
		// or a visible entry
	} while (this->position_iterator != this->category->entries.end() &&
	         !this->position_iterator->second->isVisible());
	this->setFormData();
	return;
}

void UfopaediaCategoryView::setNextSection()
{
	auto it = state->ufopaedia.begin();
	// First find myself
	while (it->second != this->category)
	{
		it++;
		if (it == state->ufopaedia.end())
		{
			LogError("Failed to find current category \"%s\"", this->category->title);
		}
	}
	// Increment it once to get the next
	it++;
	// Loop around to the beginning
	if (it == state->ufopaedia.end())
	{
		it = state->ufopaedia.begin();
	}
	fw().stageQueueCommand(
	    {StageCmd::Command::REPLACE, mksp<UfopaediaCategoryView>(state, it->second)});
	return;
}

void UfopaediaCategoryView::setPreviousSection()
{
	auto it = state->ufopaedia.begin();
	// First find myself
	while (it->second != this->category)
	{
		it++;
		if (it == state->ufopaedia.end())
		{
			LogError("Failed to find current category \"%s\"", this->category->title);
		}
	}
	// Loop around to the beginning
	if (it == state->ufopaedia.begin())
	{
		it = state->ufopaedia.end();
	}
	// Decrement it once to get the previous
	it--;
	fw().stageQueueCommand(
	    {StageCmd::Command::REPLACE, mksp<UfopaediaCategoryView>(state, it->second)});
	return;
}

bool UfopaediaCategoryView::isTransition() { return false; }

}; // namespace OpenApoc
