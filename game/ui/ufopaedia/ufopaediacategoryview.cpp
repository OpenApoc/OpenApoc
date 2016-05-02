#include "game/ui/ufopaedia/ufopaediacategoryview.h"
#include "forms/forms.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/gamestate.h"
#include "library/sp.h"

namespace OpenApoc
{

UfopaediaCategoryView::UfopaediaCategoryView(sp<GameState> state, sp<UfopaediaCategory> cat)
    : Stage(), menuform(ui().GetForm("FORM_UFOPAEDIA_BASE")), state(state), category(cat)
{
}

UfopaediaCategoryView::~UfopaediaCategoryView() {}

void UfopaediaCategoryView::Begin()
{
	auto infoLabel = menuform->FindControlTyped<Label>("TEXT_INFO");
	auto entryList = menuform->FindControlTyped<ListBox>("LISTBOX_SHORTCUTS");
	entryList->Clear();
	entryList->ItemSize = infoLabel->GetFont()->GetFontHeight() + 2;
	for (auto &pair : this->category->entries)
	{
		auto entry = pair.second;
		// Skip non-visible entries
		if (!entry->isVisible())
		{
			continue;
		}

		auto entryControl = mksp<TextButton>(tr(entry->title), infoLabel->GetFont());
		entryControl->Name = "ENTRY_SHORTCUT";
		entryControl->RenderStyle = TextButton::ButtonRenderStyle::Flat;
		entryControl->TextHAlign = HorizontalAlignment::Left;
		entryControl->TextVAlign = VerticalAlignment::Centre;
		entryControl->SetData(entry);
		entryList->AddItem(entryControl);
	}
	baseY = infoLabel->Location.y;
	for (int i = 0; i < 9; i++)
	{
		auto labelName = UString::format("LABEL_%d", i + 1);
		auto label = menuform->FindControlTyped<Label>(labelName);
		if (!label)
		{
			LogError("Failed to find UI control matching \"%s\"", labelName.c_str());
		}
		label->SetText("");
		statsLabels.push_back(label);

		auto valueName = UString::format("VALUE_%d", i + 1);
		auto value = menuform->FindControlTyped<Label>(valueName);
		if (!value)
		{
			LogError("Failed to find UI control matching \"%s\"", valueName.c_str());
		}
		value->SetText("");
		statsValues.push_back(value);
	}
	for (int i = 0; i < 4; i++)
	{
		auto labelName = UString::format("ORG_LABEL_%d", i + 1);
		auto label = menuform->FindControlTyped<Label>(labelName);
		if (!label)
		{
			LogError("Failed to find UI control matching \"%s\"", labelName.c_str());
		}
		label->SetText("");
		orgLabels.push_back(label);

		auto valueName = UString::format("ORG_VALUE_%d", i + 1);
		auto value = menuform->FindControlTyped<Label>(valueName);
		if (!value)
		{
			LogError("Failed to find UI control matching \"%s\"", valueName.c_str());
		}
		value->SetText("");
		orgValues.push_back(value);
	}
	// Start with the intro page
	this->position_iterator = this->category->entries.end();
	this->setFormData();
}

void UfopaediaCategoryView::Pause() {}

void UfopaediaCategoryView::Resume() {}

void UfopaediaCategoryView::Finish() {}

void UfopaediaCategoryView::EventOccurred(Event *e)
{
	menuform->EventOccured(e);

	if (e->Type() == EVENT_KEY_DOWN)
	{
		if (e->Keyboard().KeyCode == SDLK_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}

	if (e->Type() == EVENT_FORM_INTERACTION && e->Forms().EventFlag == FormEventType::ButtonClick)
	{
		if (e->Forms().RaisedBy->Name == "BUTTON_QUIT")
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
		if (e->Forms().RaisedBy->Name == "BUTTON_NEXT_TOPIC")
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
		if (e->Forms().RaisedBy->Name == "BUTTON_PREVIOUS_TOPIC")
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
		if (e->Forms().RaisedBy->Name == "BUTTON_NEXT_SECTION")
		{
			auto it = state->ufopaedia.begin();
			// First find myself
			while (it->second != this->category)
			{
				it++;
				if (it == state->ufopaedia.end())
				{
					LogError("Failed to find current category \"%s\"",
					         this->category->title.c_str());
				}
			}
			// Increment it once to get the next
			it++;
			// Loop around to the beginning
			if (it == state->ufopaedia.end())
			{
				it = state->ufopaedia.begin();
			}
			stageCmd.cmd = StageCmd::Command::REPLACE;
			stageCmd.nextStage = mksp<UfopaediaCategoryView>(state, it->second);
			return;
		}
		if (e->Forms().RaisedBy->Name == "BUTTON_PREVIOUS_SECTION")
		{
			auto it = state->ufopaedia.begin();
			// First find myself
			while (it->second != this->category)
			{
				it++;
				if (it == state->ufopaedia.end())
				{
					LogError("Failed to find current category \"%s\"",
					         this->category->title.c_str());
				}
			}
			// Loop around to the beginning
			if (it == state->ufopaedia.begin())
			{
				it = state->ufopaedia.end();
			}
			// Decrement it once to get the previous
			it--;
			stageCmd.cmd = StageCmd::Command::REPLACE;
			stageCmd.nextStage = mksp<UfopaediaCategoryView>(state, it->second);
			return;
		}
		if (e->Forms().RaisedBy->Name == "ENTRY_SHORTCUT")
		{
			auto entry = e->Forms().RaisedBy->GetData<UfopaediaEntry>();
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
					LogError("Failed to find current category \"%s\"",
					         this->category->title.c_str());
				}
			}
			this->position_iterator = it;
			this->setFormData();
		}
		if (e->Forms().RaisedBy->Name == "BUTTON_INFORMATION")
		{
			menuform->FindControl("INFORMATION_PANEL")->Visible =
			    !menuform->FindControl("INFORMATION_PANEL")->Visible;
			return;
		}
	}
}

void UfopaediaCategoryView::Update(StageCmd *const cmd)
{
	menuform->Update();
	*cmd = this->stageCmd;
	// Reset the command to default
	this->stageCmd = StageCmd();
}

void UfopaediaCategoryView::Render()
{
	fw().Stage_GetPrevious(this->shared_from_this())->Render();
	// fw().renderer->drawFilledRect({0, 0}, fw().Display_GetSize(), Colour{0, 0, 0, 128});
	menuform->Render();
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
	menuform->FindControlTyped<Graphic>("BACKGROUND_PICTURE")->SetImage(background);
	auto tr_description = tr(description);
	auto tr_title = tr(title);
	menuform->FindControlTyped<Label>("TEXT_INFO")->SetText(tr_description);
	menuform->FindControlTyped<Label>("TEXT_TITLE_DATA")->SetText(tr_title);

	// Every time you we change the entry reset the info panel
	menuform->FindControl("INFORMATION_PANEL")->Visible = false;

	setFormStats();
}

void UfopaediaCategoryView::setFormStats()
{
	for (unsigned int i = 0; i < statsLabels.size(); i++)
	{
		statsLabels[i]->SetText("");
		statsValues[i]->SetText("");
	}
	for (unsigned int i = 0; i < orgLabels.size(); i++)
	{
		orgLabels[i]->SetText("");
		orgValues[i]->SetText("");
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
						orgLabels[1]->SetText(tr("Balance"));
						orgValues[1]->SetText(UString::format("$%d", ref->balance));
						orgLabels[2]->SetText(tr("Income"));
						orgValues[2]->SetText(UString::format("$%d", ref->income));

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
							orgLabels[0]->SetText(relation);
							orgLabels[3]->SetText(tr("Alien Infiltration"));
							orgValues[3]->SetText(
							    UString::format("%d%%", 0)); // FIXME: Not implemented yet
						}
					}
				}
				break;
				case UfopaediaEntry::Data::Vehicle:
				{
					StateRef<VehicleType> ref = {state.get(), data_id};
					statsLabels[row]->SetText(tr("Constitution"));
					statsValues[row++]->SetText(Strings::FromInteger(ref->health));
					int armour = 0;
					for (auto &slot : ref->armour)
					{
						armour += slot.second;
					}
					statsLabels[row]->SetText(tr("Armor"));
					statsValues[row++]->SetText(Strings::FromInteger(armour));
					statsLabels[row]->SetText(tr("Weight"));
					statsValues[row++]->SetText(Strings::FromInteger(ref->weight));
					statsLabels[row]->SetText(tr("Passengers"));
					statsValues[row++]->SetText(Strings::FromInteger(ref->passengers));
					int weaponSpace = 0;
					int weaponAmount = 0;
					int engineSpace = 0;
					int generalSpace = 0;
					for (auto &slot : ref->equipment_layout_slots)
					{
						int space = slot.bounds.size().x * slot.bounds.size().y;
						switch (slot.type)
						{
							case VEquipmentType::Type::Engine:
								engineSpace += space;
								break;
							case VEquipmentType::Type::Weapon:
								weaponSpace += space;
								weaponAmount++;
								break;
							case VEquipmentType::Type::General:
								generalSpace += space;
								break;
						}
					}
					statsLabels[row]->SetText(tr("Weapons space"));
					statsValues[row++]->SetText(Strings::FromInteger(weaponSpace));
					statsLabels[row]->SetText(tr("Weapons slots"));
					statsValues[row++]->SetText(Strings::FromInteger(weaponAmount));
					statsLabels[row]->SetText(tr("Engine size"));
					statsValues[row++]->SetText(Strings::FromInteger(engineSpace));
					statsLabels[row]->SetText(tr("Equipment space"));
					statsValues[row++]->SetText(Strings::FromInteger(generalSpace));
				}
				break;
				case UfopaediaEntry::Data::VehicleEquipment:
				{
					StateRef<VEquipmentType> ref = {state.get(), data_id};
					statsLabels[row]->SetText(tr("Weight"));
					statsValues[row++]->SetText(Strings::FromInteger(ref->weight));
					statsLabels[row]->SetText(tr("Size"));
					statsValues[row++]->SetText(
					    UString::format("%dx%d", ref->equipscreen_size.x, ref->equipscreen_size.y));
					switch (ref->type)
					{
						case VEquipmentType::Type::Engine:
							statsLabels[row]->SetText(tr("Power"));
							statsValues[row++]->SetText(Strings::FromInteger(ref->power));
							statsLabels[row]->SetText(tr("Top Speed"));
							statsValues[row++]->SetText(Strings::FromInteger(ref->top_speed));
							break;
						case VEquipmentType::Type::Weapon:
							statsLabels[row]->SetText(tr("Damage"));
							statsValues[row++]->SetText(Strings::FromInteger(ref->damage));
							statsLabels[row]->SetText(tr("Accuracy"));
							statsValues[row++]->SetText(UString::format("%d%%", ref->accuracy));
							statsLabels[row]->SetText(tr("Range"));
							statsValues[row++]->SetText(UString::format("%dm", ref->range));
							statsLabels[row]->SetText(tr("Fire Rate"));
							statsValues[row++]->SetText(
							    UString::format("%d.00 r/s", ref->fire_delay));
							if (ref->max_ammo > 0)
							{
								statsLabels[row]->SetText(tr("Ammo type"));
								statsValues[row++]->SetText(tr(ref->ammo_type));
								statsLabels[row]->SetText(tr("Ammo capacity"));
								statsValues[row++]->SetText(Strings::FromInteger(ref->max_ammo));
							}
							if (ref->turn_rate > 0)
							{
								statsLabels[row]->SetText(tr("Turn Rate"));
								statsValues[row++]->SetText(Strings::FromInteger(ref->turn_rate));
							}
							break;
						case VEquipmentType::Type::General:
							if (ref->accuracy_modifier > 0)
							{
								statsLabels[row]->SetText(tr("Accuracy"));
								statsValues[row++]->SetText(
								    UString::format("+%d%%", ref->accuracy_modifier));
							}
							if (ref->cargo_space > 0)
							{
								statsLabels[row]->SetText(tr("Cargo"));
								statsValues[row++]->SetText(Strings::FromInteger(ref->cargo_space));
							}
							if (ref->passengers > 0)
							{
								statsLabels[row]->SetText(tr("Passengers"));
								statsValues[row++]->SetText(Strings::FromInteger(ref->passengers));
							}
							if (ref->alien_space > 0)
							{
								statsLabels[row]->SetText(tr("Aliens Held"));
								statsValues[row++]->SetText(Strings::FromInteger(ref->alien_space));
							}
							if (ref->missile_jamming > 0)
							{
								statsLabels[row]->SetText(tr("Jamming"));
								statsValues[row++]->SetText(
								    UString::format("%d%%", ref->missile_jamming));
							}
							if (ref->shielding > 0)
							{
								statsLabels[row]->SetText(tr("Shielding"));
								statsValues[row++]->SetText(UString::format("+%d", ref->shielding));
							}
							if (ref->cloaking)
							{
								statsValues[row++]->SetText(tr("Cloaks Craft"));
							}
							break;
					}
				}
				break;
				case UfopaediaEntry::Data::Equipment:
				{
					// FIXME: Not implemented yet
				}
				break;
				case UfopaediaEntry::Data::Facility:
				{
					StateRef<FacilityType> ref = {state.get(), data_id};
					statsLabels[row]->SetText(tr("Construction cost"));
					statsValues[row++]->SetText(UString::format("$%d", ref->buildCost));
					statsLabels[row]->SetText(tr("Days to build"));
					statsValues[row++]->SetText(Strings::FromInteger(ref->buildTime));
					statsLabels[row]->SetText(tr("Weekly cost"));
					statsValues[row++]->SetText(UString::format("$%d", ref->weeklyCost));
					if (ref->capacityAmount > 0)
					{
						statsLabels[row]->SetText(tr("Capacity"));
						statsValues[row++]->SetText(Strings::FromInteger(ref->capacityAmount));
					}
				}
				break;
				case UfopaediaEntry::Data::Building:
				{
					// FIXME: Not implemented yet
				}
				break;
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
		menuform->FindControlTyped<Label>("TEXT_INFO")->Location.y = y;
	}
	else
	{
		menuform->FindControlTyped<Label>("TEXT_INFO")->Location.y = baseY;
	}
}

bool UfopaediaCategoryView::IsTransition() { return false; }

}; // namespace OpenApoc
