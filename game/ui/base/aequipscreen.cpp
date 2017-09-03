#include "game/state/battle/battleitem.h"
#include "game/ui/general/messagebox.h"
#include "game/state/rules/damage.h"
#include "game/state/base/base.h"
#include "framework/apocresources/cursor.h"
#include "game/state/aequipment.h"
#include "game/ui/base/aequipscreen.h"
#include "forms/form.h"
#include "framework/font.h"
#include "forms/graphic.h"
#include "forms/label.h"
#include "game/state/battle/battleunit.h"
#include "forms/list.h"
#include "forms/scrollbar.h"
#include "forms/ui.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "framework/renderer.h"
#include "game/state/gamestate.h"
#include "game/ui/equipscreen.h"

namespace OpenApoc
{

static sp<Image> createStatsBar(int initialValue, int currentValue, int modifiedValue, int maxValue,
                                Colour &initialColour, Colour &currentColour, Vec2<int> imageSize)
{
	// Some agent types (e.g. Android's Psi-attack) have zero values. Break out here to avoid
	// dividing by zero.
	if (initialValue == 0 && currentValue == 0 && modifiedValue == 0 && maxValue == 0)
	{
		return mksp<RGBImage>(imageSize);
	}
	LogAssert(initialValue >= 0);
	LogAssert(currentValue >= 0);
	LogAssert(currentValue >= initialValue);
	LogAssert(modifiedValue >= 0);
	LogAssert(maxValue > 0);
	// Need at least 3 y pixels for the 'hollow' bar
	LogAssert(imageSize.x > 0 && imageSize.y > 2);

	int initialPixels = (float)imageSize.x * ((float)initialValue / (float)maxValue);
	int currentPixels = (float)imageSize.x * ((float)currentValue / (float)maxValue);
	int modifiedPixels = (float)imageSize.x * ((float)modifiedValue / (float)maxValue);
	initialPixels = clamp(initialPixels, 0, imageSize.x - 1);
	currentPixels = clamp(currentPixels, 0, imageSize.x - 1);
	modifiedPixels = clamp(modifiedPixels, 0, imageSize.x - 1);

	sp<RGBImage> img = mksp<RGBImage>(imageSize);

	int x = 0;

	RGBImageLock l(img);

	for (; x < currentPixels; x++)
	{
		if (x <= initialPixels)
		{
			// Draw a 'hollow' line if we're > modifiedPixels (and not the end)
			if (x > modifiedPixels && x < currentPixels - 1)
			{
				l.set({x, 0}, initialColour);
				l.set({x, imageSize.y - 1}, initialColour);
			}
			else
			{
				for (int y = 0; y < imageSize.y; y++)
					l.set({x, y}, initialColour);
			}
		}
		else
		{
			// Draw a 'hollow' line if we're > modifiedPixels (and not the end)
			if (x > modifiedPixels && x < currentPixels - 1)
			{
				l.set({x, 0}, initialColour);
				l.set({x, imageSize.y - 1}, currentColour);
			}
			else
			{
				for (int y = 0; y < imageSize.y; y++)
					l.set({x, y}, currentColour);
			}
		}
	}

	return img;
}
const Vec2<int> AEquipScreen::EQUIP_GRID_SLOT_SIZE{16, 16};
const Vec2<int> AEquipScreen::EQUIP_GRID_SLOTS{16, 16};

AEquipScreen::AEquipScreen(sp<GameState> state, sp<Agent> firstAgent)
    : Stage(), firstAgent(firstAgent), formMain(ui().getForm("aequipscreen_main")),
	  formAgentStats(ui().getForm("aequipscreen_agent_stats")),
	  formItemWeapon(ui().getForm("aequipscreen_item_weapon")),
	  formItemArmor(ui().getForm("aequipscreen_item_armor")),
	  formItemGrenade(ui().getForm("aequipscreen_item_grenade")),
	  formItemOther(ui().getForm("aequipscreen_item_other")),
      pal(fw().data->loadPalette("xcom3/ufodata/agenteqp.pcx")), 
	  labelFont(ui().getFont("smalfont")), state(state)

{
	this->state = state;

	auto paperDollPlaceholder = formMain->findControlTyped<Graphic>("PAPER_DOLL");

	this->paperDoll = formMain->createChild<EquipmentPaperDoll>(
	    paperDollPlaceholder->Location, paperDollPlaceholder->Size, EQUIP_GRID_SLOT_SIZE);

	auto img = mksp<RGBImage>(Vec2<int>{1, 2});
	{
		RGBImageLock l(img);
		l.set({ 0, 0 }, Colour{ 255, 255, 219 });
		l.set({ 0, 1 }, Colour{ 215, 0, 0 });
	}
	this->healthImage = img;
	img = mksp<RGBImage>(Vec2<int>{1, 2});
	{
		RGBImageLock l(img);
		l.set({ 0, 0 }, Colour{ 160, 236, 252 });
		l.set({ 0, 1 }, Colour{ 4, 100, 252 });
	}
	this->shieldImage = img;
	img = mksp<RGBImage>(Vec2<int>{1, 2});
	{
		RGBImageLock l(img);
		l.set({ 0, 0 }, Colour{ 150, 150, 150 });
		l.set({ 0, 1 }, Colour{ 97, 101, 105 });
	}
	this->stunImage = img;
	this->iconShade = fw().data->loadImage("battle/battle-icon-shade.png");
	for (int i = 28; i <= 34; i++)
	{
		unitRanks.push_back(fw().data->loadImage(format("PCK:xcom3/tacdata/tacbut.pck:xcom3/tacdata/"
			"tacbut.tab:%d:xcom3/tacdata/tactical.pal",
			i)));
	}
	for (int i = 12; i <= 18; i++)
	{
		bigUnitRanks.push_back(fw().data->loadImage(format("PCK:xcom3/tacdata/tacbut.pck:xcom3/tacdata/"
			"tacbut.tab:%d:xcom3/tacdata/tactical.pal",
			i)));
	}


	// Agent list functionality
	auto agentList = formMain->findControlTyped<ListBox>("AGENT_SELECT_BOX");
	agentList->addCallback(FormEventType::ListBoxChangeSelected, [this](Event *e) {
		LogWarning("agent selected");
		auto list = std::static_pointer_cast<ListBox>(e->forms().RaisedBy);
		auto agent = list->getSelectedData<Agent>();
		if (!agent)
		{
			LogError("No agent in selected data");
			return;
		}
		if (agent->unit && !agent->unit->isConscious())
		{
			LogWarning("Cannot select unconscious agent");
			return;
		}
		this->setSelectedAgent(agent);
	});
}

AEquipScreen::~AEquipScreen() = default;

void AEquipScreen::begin()
{
	formMain->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
	if (firstAgent)
	{
		this->setSelectedAgent(firstAgent);
	}
	else
	{
		for (auto &agent : state->agents)
		{
			if (agent.second->type->role != AgentType::Role::Soldier)
			{
				continue;
			}
			if (agent.second->unit && !agent.second->unit->isConscious())
			{
				continue;
			}
			if (state->current_battle && !agent.second->unit)
			{
				continue;
			}
			this->setSelectedAgent(agent.second);
			break;
		}
	}

	// Populate agent list
	auto agentList = formMain->findControlTyped<ListBox>("AGENT_SELECT_BOX");
	auto font = ui().getFont("smalfont");
	auto agentEntryHeight = font->getFontHeight() * 2;
	agentList->clear();
	auto owner = state->getPlayer();
	if (state->current_battle)
	{
		owner = state->current_battle->currentPlayer;
	}
	for (auto &agent : state->agents)
	{
		if (agent.second->owner != owner)
		{
			continue;
		}
		if (agent.second->type->role != AgentType::Role::Soldier)
		{
			continue;
		}
		// Unit is not participating in battle
		if (state->current_battle && !agent.second->unit)
		{
			continue;
		}

		auto agentControl =
			this->createAgentControl({ 130, agentEntryHeight }, { state.get(), agent.second });
		agentList->addItem(agentControl);
		if (agent.second == currentAgent)
		{
			agentList->setSelected(agentControl);
		}
	}
	agentList->ItemSize = agentEntryHeight;
}

void AEquipScreen::pause() {}

void AEquipScreen::resume() {}

void AEquipScreen::finish() {}

void AEquipScreen::eventOccurred(Event *e)
{
	formMain->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE)
		{
			closeScreen();
			return;
		}
	}
	if (e->type() == EVENT_FORM_INTERACTION && e->forms().EventFlag == FormEventType::ButtonClick)
	{

		if (e->forms().RaisedBy->Name == "BUTTON_OK")
		{
			closeScreen();
			return;
		}
	}
	// Check if we've moused over equipment/vehicle so we can show the stats.
	if (e->type() == EVENT_MOUSE_MOVE && !this->draggedEquipment)
	{
		// Wipe any previously-highlighted stuff
		sp<AEquipment> highlightedEquipment;

		Vec2<int> mousePos{ e->mouse().X, e->mouse().Y };

		// Check if we're over any equipment in the paper doll
		auto mouseSlotPos = this->paperDoll->getSlotPositionFromScreenPosition(mousePos);
		auto equipment =
			std::dynamic_pointer_cast<AEquipment>(this->currentAgent->getEquipmentAt(mouseSlotPos));
		if (equipment)
		{
			highlightedEquipment = equipment;
		}

		// Check if we're over any equipment in the list at the bottom
		for (auto &tuple : this->inventoryItems)
		{
			if (std::get<0>(tuple).within(mousePos))
			{
				highlightedEquipment = std::get<2>(tuple);
				break;
			}
		}

		// Finally change active form
		if (highlightedEquipment)
		{
			displayItem(highlightedEquipment);
		}
		else
		{
			formActive = formAgentStats;
		}
	}
	// Removal of equipment
	if (e->type() == EVENT_MOUSE_DOWN)
	{
		Vec2<int> mousePos{ e->mouse().X, e->mouse().Y };

		// Check if we're over any equipment in the paper doll
		auto mouseSlotPos = this->paperDoll->getSlotPositionFromScreenPosition(mousePos);
		auto equipment =
			std::dynamic_pointer_cast<AEquipment>(currentAgent->getEquipmentAt(mouseSlotPos));
		if (equipment)
		{
			// FIXME: base->addBackToInventory(item); vehicle->unequip(item);
			this->draggedEquipment = equipment;
			this->draggedEquipmentOffset = { 0, 0 };

			currentAgent->removeEquipment(equipment);
			this->paperDoll->updateEquipment();
			return;
		}

		// Check if we're over any equipment in the list at the bottom
		for (auto &tuple : this->inventoryItems)
		{
			auto rect = std::get<0>(tuple);
			if (rect.within(mousePos))
			{
				this->draggedEquipment = std::get<2>(tuple);
				this->draggedEquipmentOffset = rect.p0 - mousePos;

				removeItemFromInventory(draggedEquipment);
				refreshInventoryItems();
				return;
			}
		}
	}
	if (e->type() == EVENT_MOUSE_UP)
	{
		if (this->draggedEquipment)
		{
			// Are we over the grid? If so try to place it on the agent.
			auto paperDollControl = this->paperDoll;
			Vec2<int> equipOffset = paperDollControl->Location + formMain->Location;

			Vec2<int> equipmentPos = fw().getCursor().getPosition() + this->draggedEquipmentOffset;
			// If this is within the grid try to snap it
			Vec2<int> equipmentGridPos = equipmentPos - equipOffset;
			equipmentGridPos /= EQUIP_GRID_SLOT_SIZE;
			if (currentAgent->canAddEquipment(equipmentGridPos, this->draggedEquipment->type))
			{
				currentAgent->addEquipment(*state, equipmentGridPos, this->draggedEquipment);
				this->paperDoll->updateEquipment();
			}
			else
			{
				addItemToInventory(draggedEquipment);
				refreshInventoryItems();
			}
			this->draggedEquipment = nullptr;
			
			// FIXME: Add AP cost for inventory manipulation
		}
	}
}

void AEquipScreen::update() { formMain->update(); }

void AEquipScreen::render()
{
	auto inventoryControl = formMain->findControlTyped<Graphic>("INVENTORY");
	int inventoryBottom = inventoryControl->Location.y + inventoryControl->Size.y + +formMain->Location.y;
	fw().stageGetPrevious(this->shared_from_this())->render();
	fw().renderer->setPalette(this->pal);
	formMain->render();
	if (formActive)
	{
		formActive->render();
	}

	Vec2<int> equipOffset = this->paperDoll->getLocationOnScreen();
	
	for (auto &tuple : inventoryItems)
	{
		// The gap between the bottom of the inventory image and the count label
		static const int INVENTORY_COUNT_Y_GAP = 4;
		// The gap between the end of one inventory image and the start of the next
		static const int INVENTORY_IMAGE_X_GAP = 4;
		
		auto item = std::get<2>(tuple);
		auto count = std::get<1>(tuple);
		auto rect = std::get<0>(tuple);
		auto countImage = count > 0 ? labelFont->getString(format("%d", count)) : nullptr;
		auto &equipmentImage = item->type->equipscreen_sprite;

		fw().renderer->draw(equipmentImage, rect.p0);

		if (countImage)
		{
			Vec2<int> countLabelPosition = rect.p0;
			countLabelPosition.x += equipmentImage->size.x / 2 - countImage->size.x / 2;
			countLabelPosition.y += INVENTORY_COUNT_Y_GAP + equipmentImage->size.y;
			countLabelPosition.y = std::min(countLabelPosition.y, inventoryBottom - (int)countImage->size.y);

			fw().renderer->draw(countImage, countLabelPosition);
		}
	}

	if (this->draggedEquipment)
	{
		// Draw equipment we're currently dragging (snapping to the grid if possible)
		Vec2<int> equipmentPos = fw().getCursor().getPosition() + this->draggedEquipmentOffset;
		// If this is within the grid try to snap it
		Vec2<int> equipmentGridPos = equipmentPos - equipOffset;
		equipmentGridPos /= EQUIP_GRID_SLOT_SIZE;
		if (equipmentGridPos.x < 0 || equipmentGridPos.x >= EQUIP_GRID_SLOTS.x ||
			equipmentGridPos.y < 0 || equipmentGridPos.y >= EQUIP_GRID_SLOTS.y)
		{
			// This is outside the grid
		}
		else
		{
			// Inside the grid, snap
			equipmentPos = equipmentGridPos * EQUIP_GRID_SLOT_SIZE;
			equipmentPos += equipOffset;
		}
		fw().renderer->draw(this->draggedEquipment->type->equipscreen_sprite, equipmentPos);
	}
}

bool AEquipScreen::isTransition() { return false; }

void AEquipScreen::displayItem(sp<AEquipment> item)
{			   
	switch (item->type->type)
	{
		case AEquipmentType::Type::Weapon:
		{	
			formItemWeapon->findControlTyped<Label>("ITEM_NAME")->setText(item->type->name);
			formItemWeapon->findControlTyped<Graphic>("SELECTED_IMAGE")
				->setImage(item->getEquipmentImage());
			
			formItemWeapon->findControlTyped<Label>("LABEL_AMMO_TYPE_1")->setVisible(false);
			formItemWeapon->findControlTyped<Label>("LABEL_AMMO_TYPE_2")->setVisible(false);
			formItemWeapon->findControlTyped<Label>("LABEL_AMMO_TYPE_3")->setVisible(false);
			formItemWeapon->findControlTyped<Label>("LABEL_AMMO_TYPE_4")->setVisible(false);

			if (item->getPayloadType())
			{
				formItemWeapon->findControlTyped<Label>("LABEL_AMMO_TYPES")->setVisible(false);
				
				formItemWeapon->findControlTyped<Label>("LABEL_AMMO_TYPE")->setVisible(true);
				formItemWeapon->findControlTyped<Label>("VALUE_AMMO_TYPE")->setVisible(true);
				formItemWeapon->findControlTyped<Label>("LABEL_POWER")->setVisible(true);
				formItemWeapon->findControlTyped<Label>("VALUE_POWER")->setVisible(true);
				formItemWeapon->findControlTyped<Label>("LABEL_ROUNDS")->setVisible(true);
				formItemWeapon->findControlTyped<Label>("VALUE_ROUNDS")->setVisible(true);
				formItemWeapon->findControlTyped<Label>("LABEL_RECHARGES")->setVisible(item->getPayloadType()->recharge > 0);
				formItemWeapon->findControlTyped<Label>("LABEL_ACCURACY")->setVisible(true);
				
				formItemWeapon->findControlTyped<Label>("VALUE_AMMO_TYPE")->setText(item->getPayloadType()->damage_type->name);
				formItemWeapon->findControlTyped<Label>("VALUE_POWER")->setText(format("%d", item->getPayloadType()->damage));
				formItemWeapon->findControlTyped<Label>("VALUE_ROUNDS")->setText(format("%d", item->ammo));

				formItemWeapon->findControlTyped<Label>("VALUE_ACCURACY")->setText(format("%d", item->getPayloadType()->accuracy));
				formItemWeapon->findControlTyped<Label>("VALUE_FIRE_RATE")->setText(format("%d", item->getPayloadType()->fire_delay));
				formItemWeapon->findControlTyped<Label>("VALUE_RANGE")->setText(format("%d", item->getPayloadType()->range));
			}
			else
			{
				formItemWeapon->findControlTyped<Label>("LABEL_AMMO_TYPES")->setVisible(true);

				formItemWeapon->findControlTyped<Label>("LABEL_AMMO_TYPE")->setVisible(false);
				formItemWeapon->findControlTyped<Label>("VALUE_AMMO_TYPE")->setVisible(false);
				formItemWeapon->findControlTyped<Label>("LABEL_POWER")->setVisible(false);
				formItemWeapon->findControlTyped<Label>("VALUE_POWER")->setVisible(false);
				formItemWeapon->findControlTyped<Label>("LABEL_ROUNDS")->setVisible(false);
				formItemWeapon->findControlTyped<Label>("VALUE_ROUNDS")->setVisible(false);
				formItemWeapon->findControlTyped<Label>("LABEL_RECHARGES")->setVisible(false);
				
				int ammoNum = 1;
				for (auto &ammo : item->type->ammo_types)
				{
					formItemWeapon->findControlTyped<Label>(format("LABEL_AMMO_TYPE_%d", ammoNum))->setVisible(true);
					formItemWeapon->findControlTyped<Label>(format("LABEL_AMMO_TYPE_%d", ammoNum))->setText(ammo->name);
					ammoNum++;
					if (ammoNum > 4)
					{
						break;
					}
				}

				if (item->type->ammo_types.empty())
				{
					LogError("No ammo types exist for a weapon?");
					formItemWeapon->findControlTyped<Label>("VALUE_ACCURACY")->setText("");
					formItemWeapon->findControlTyped<Label>("VALUE_FIRE_RATE")->setText("");
					formItemWeapon->findControlTyped<Label>("VALUE_RANGE")->setText("");
				}
				else
				{
					formItemWeapon->findControlTyped<Label>("VALUE_ACCURACY")->setText(format("%d", item->type->ammo_types.front()->accuracy));
					formItemWeapon->findControlTyped<Label>("VALUE_FIRE_RATE")->setText(format("%d", item->type->ammo_types.front()->fire_delay));
					formItemWeapon->findControlTyped<Label>("VALUE_RANGE")->setText(format("%d", item->type->ammo_types.front()->range));
				}
			}

			formItemWeapon->findControlTyped<Label>("VALUE_WEIGHT")->setText(format("%d", item->type->weight));

			formActive = formItemWeapon;
		}
			break;
		case AEquipmentType::Type::Armor:
		{
			formItemArmor->findControlTyped<Label>("ITEM_NAME")->setText(item->type->name);
			formItemArmor->findControlTyped<Graphic>("SELECTED_IMAGE")
				->setImage(item->getEquipmentImage());

			formItemArmor->findControlTyped<Label>("VALUE_WEIGHT")->setText(format("%d", item->type->weight));
			formItemArmor->findControlTyped<Label>("VALUE_PROTECTION")->setText(format("%d", item->ammo));

			formActive = formItemArmor;
		}
		break;
		case AEquipmentType::Type::Ammo:
		{
			formItemWeapon->findControlTyped<Label>("ITEM_NAME")->setText(item->type->name);
			formItemWeapon->findControlTyped<Graphic>("SELECTED_IMAGE")
				->setImage(item->getEquipmentImage());

			formItemWeapon->findControlTyped<Label>("LABEL_AMMO_TYPES")->setVisible(false);
			formItemWeapon->findControlTyped<Label>("LABEL_AMMO_TYPE_1")->setVisible(false);
			formItemWeapon->findControlTyped<Label>("LABEL_AMMO_TYPE_2")->setVisible(false);
			formItemWeapon->findControlTyped<Label>("LABEL_AMMO_TYPE_3")->setVisible(false);
			formItemWeapon->findControlTyped<Label>("LABEL_AMMO_TYPE_4")->setVisible(false);

			formItemWeapon->findControlTyped<Label>("LABEL_AMMO_TYPE")->setVisible(true);
			formItemWeapon->findControlTyped<Label>("VALUE_AMMO_TYPE")->setVisible(true);
			formItemWeapon->findControlTyped<Label>("LABEL_POWER")->setVisible(true);
			formItemWeapon->findControlTyped<Label>("VALUE_POWER")->setVisible(true);
			formItemWeapon->findControlTyped<Label>("LABEL_ROUNDS")->setVisible(true);
			formItemWeapon->findControlTyped<Label>("VALUE_ROUNDS")->setVisible(true);
			formItemWeapon->findControlTyped<Label>("LABEL_RECHARGES")->setVisible(item->getPayloadType()->recharge > 0);

			formItemWeapon->findControlTyped<Label>("VALUE_WEIGHT")->setText(format("%d", item->type->weight));
			formItemWeapon->findControlTyped<Label>("VALUE_AMMO_TYPE")->setText(item->type->damage_type->name);

			formItemWeapon->findControlTyped<Label>("VALUE_ACCURACY")->setText(format("%d", item->type->accuracy));
			formItemWeapon->findControlTyped<Label>("VALUE_FIRE_RATE")->setText(format("%d", item->type->fire_delay));
			formItemWeapon->findControlTyped<Label>("VALUE_RANGE")->setText(format("%d", item->type->range));

			formItemWeapon->findControlTyped<Label>("VALUE_POWER")->setText(format("%d", item->type->damage));
			formItemWeapon->findControlTyped<Label>("VALUE_ROUNDS")->setText(format("%d", item->ammo));

			formActive = formItemWeapon;
		}
		break;
		case AEquipmentType::Type::Grenade:
		{
			formItemGrenade->findControlTyped<Label>("ITEM_NAME")->setText(item->type->name);
			formItemGrenade->findControlTyped<Graphic>("SELECTED_IMAGE")
				->setImage(item->getEquipmentImage());
			
			formItemGrenade->findControlTyped<Label>("VALUE_WEIGHT")->setText(format("%d", item->type->weight));
			formItemGrenade->findControlTyped<Label>("VALUE_AMMO_TYPE")->setText(item->type->damage_type->name);
			formItemGrenade->findControlTyped<Label>("VALUE_POWER")->setText(format("%d", item->type->damage));

			formActive = formItemGrenade;
		}
		break;
		default:
		{
			formItemOther->findControlTyped<Label>("ITEM_NAME")->setText(item->type->name);
			formItemOther->findControlTyped<Graphic>("SELECTED_IMAGE")
				->setImage(item->getEquipmentImage());

			formItemOther->findControlTyped<Label>("VALUE_WEIGHT")->setText(format("%d", item->type->weight));

			formActive = formItemOther;
		}
		break;
	}
}

AEquipScreen::Mode AEquipScreen::getMode()
{
	// TODO: Finish implementation after implementing agents traveling the city by themselves and on vehicles

	// If agent in battle
	if (currentAgent->unit)
	{
		return AEquipScreen::Mode::Battle;
	}
	// If agent in base and not in vehicle or in vehicle which is parked in base
	else if (true)
	{
		return AEquipScreen::Mode::Base;
	}
	// If agent is in vehicle which is not at any base
	else if (false)
	{
		return AEquipScreen::Mode::Vehicle;
	}
	// If agent is in a building which is not any base
	else if (false)
	{
		return AEquipScreen::Mode::Building;
	}
	// Agent is moving somewhere by foot
	else
	{
		return AEquipScreen::Mode::Agent;
	}
}

void AEquipScreen::refreshInventoryItems()
{
	inventoryItems.clear();

	switch (getMode())
	{
		case Mode::Agent:
			populateInventoryItemsTemporary();
			break;
		case Mode::Base:
			populateInventoryItemsBase();
			break;
		case Mode::Battle:
			populateInventoryItemsBattle();
			break;
		case Mode::Building:
			populateInventoryItemsBuilding();
			break;
		case Mode::Vehicle:
			populateInventoryItemsVehicle();
			break;
	}
}

void AEquipScreen::populateInventoryItemsBattle()
{
	// The gap between the end of one inventory image and the start of the next
	static const int INVENTORY_IMAGE_X_GAP = 4;
	auto inventoryControl = formMain->findControlTyped<Graphic>("INVENTORY");
	Vec2<int> inventoryPosition = inventoryControl->Location + formMain->Location;

	auto itemsOnGround = currentAgent->unit->tileObject->getOwningTile()->getItems();
	
	for (auto &item : itemsOnGround)
	{
		auto &equipmentImage = item->item->type->equipscreen_sprite;
		Vec2<int> inventoryEndPosition = inventoryPosition;
		inventoryEndPosition.x += equipmentImage->size.x;
		inventoryEndPosition.y += equipmentImage->size.y;

		inventoryItems.push_back(std::make_tuple(Rect<int>{inventoryPosition, inventoryEndPosition}, 0, item->item));

		inventoryPosition.x += INVENTORY_IMAGE_X_GAP + equipmentImage->size.x;
	}
}

void AEquipScreen::populateInventoryItemsBase()
{
	// The gap between the end of one inventory image and the start of the next
	static const int INVENTORY_IMAGE_X_GAP = 4;
	auto inventoryControl = formMain->findControlTyped<Graphic>("INVENTORY");
	Vec2<int> inventoryPosition = inventoryControl->Location + formMain->Location;

	// Find base which is in the current building
	StateRef<Base> base;
	for (auto &b : state->player_bases)
	{
		// TODO: Fix this to be the building agent is currently in
		if (b.second->building == currentAgent->home_base->building)
		{
			base = { state.get(), b.first };
		}
	}

	for (auto &invPair : base->inventoryAgentEquipment)
	{
		StateRef<AEquipmentType> type = { state.get(), invPair.first };

		// Find the item count
		int count = 0;
		int ammoRemaining = 0;
		if (type->type == AEquipmentType::Type::Ammo)
		{
			count = invPair.second / type->max_ammo;
			ammoRemaining = invPair.second - count * type->max_ammo;
			if (ammoRemaining > 0)
			{
				count++;
			}
		}
		else
		{
			count = invPair.second;
		}

		if (count == 0)
		{
			continue;
		}

		auto &equipmentImage = type->equipscreen_sprite;
		Vec2<int> inventoryEndPosition = inventoryPosition;
		inventoryEndPosition.x += equipmentImage->size.x;
		inventoryEndPosition.y += equipmentImage->size.y;

		auto equipment = mksp<AEquipment>();
		equipment->type = type;
		equipment->armor = type->armor;
		if (count == 1 && ammoRemaining > 0)
		{
			equipment->ammo = ammoRemaining;
		}
		else if (type->ammo_types.size() == 0)
		{
			equipment->ammo = type->max_ammo;
		}
		
		inventoryItems.push_back(std::make_tuple(Rect<int>{inventoryPosition, inventoryEndPosition}, count, equipment));

		inventoryPosition.x += INVENTORY_IMAGE_X_GAP + equipmentImage->size.x;
	}

}

void AEquipScreen::populateInventoryItemsVehicle()
{
	// The gap between the end of one inventory image and the start of the next
	static const int INVENTORY_IMAGE_X_GAP = 4;
	auto inventoryControl = formMain->findControlTyped<Graphic>("INVENTORY");
	Vec2<int> inventoryPosition = inventoryControl->Location + formMain->Location;

	StateRef<Vehicle> vehicle; //Should be the vehicle agent is in
	LogError("Implement getting agent's vehicle");
	auto itemsOnVehicle = vehicleItems[vehicle];
		
	for (auto &item : itemsOnVehicle)
	{
		auto &equipmentImage = item->type->equipscreen_sprite;
		Vec2<int> inventoryEndPosition = inventoryPosition;
		inventoryEndPosition.x += equipmentImage->size.x;
		inventoryEndPosition.y += equipmentImage->size.y;

		inventoryItems.push_back(std::make_tuple(Rect<int>{inventoryPosition, inventoryEndPosition}, 0, item));

		inventoryPosition.x += INVENTORY_IMAGE_X_GAP + equipmentImage->size.x;
	}
}

void AEquipScreen::populateInventoryItemsBuilding()
{
	// The gap between the end of one inventory image and the start of the next
	static const int INVENTORY_IMAGE_X_GAP = 4;
	auto inventoryControl = formMain->findControlTyped<Graphic>("INVENTORY");
	Vec2<int> inventoryPosition = inventoryControl->Location + formMain->Location;

	StateRef<Building> building; //Should be the building agent is in
	LogError("Implement getting agent's building");
	auto itemsOnBuilding = buildingItems[building];

	for (auto &item : itemsOnBuilding)
	{
		auto &equipmentImage = item->type->equipscreen_sprite;
		Vec2<int> inventoryEndPosition = inventoryPosition;
		inventoryEndPosition.x += equipmentImage->size.x;
		inventoryEndPosition.y += equipmentImage->size.y;

		inventoryItems.push_back(std::make_tuple(Rect<int>{inventoryPosition, inventoryEndPosition}, 0, item));

		inventoryPosition.x += INVENTORY_IMAGE_X_GAP + equipmentImage->size.x;
	}
}

void AEquipScreen::populateInventoryItemsTemporary()
{
	// The gap between the end of one inventory image and the start of the next
	static const int INVENTORY_IMAGE_X_GAP = 4;
	auto inventoryControl = formMain->findControlTyped<Graphic>("INVENTORY");
	Vec2<int> inventoryPosition = inventoryControl->Location + formMain->Location;

	auto itemsOnAgent = agentItems[{state.get(), Agent::getId(*state, currentAgent)}];

	for (auto &item : itemsOnAgent)
	{
		auto &equipmentImage = item->type->equipscreen_sprite;
		Vec2<int> inventoryEndPosition = inventoryPosition;
		inventoryEndPosition.x += equipmentImage->size.x;
		inventoryEndPosition.y += equipmentImage->size.y;

		inventoryItems.push_back(std::make_tuple(Rect<int>{inventoryPosition, inventoryEndPosition}, 0, item));

		inventoryPosition.x += INVENTORY_IMAGE_X_GAP + equipmentImage->size.x;
	}
}

void AEquipScreen::removeItemFromInventory(sp<AEquipment> item)
{
	switch (getMode())
	{
		case Mode::Agent:
			removeItemFromInventoryTemporary(item);
			break;
		case Mode::Base:
			removeItemFromInventoryBase(item);
			break;
		case Mode::Battle:
			removeItemFromInventoryBattle(item);
			break;
		case Mode::Building:
			removeItemFromInventoryBuilding(item);
			break;
		case Mode::Vehicle:
			removeItemFromInventoryVehicle(item);
			break;
	}
}


void AEquipScreen::removeItemFromInventoryBattle(sp<AEquipment> item)
{
	auto battleItem = item->ownerItem.lock();
	if (!battleItem)
	{
		LogError("No battle item object in battle inventory?");
		return;
	}
	battleItem->die(*state, false);
}

void AEquipScreen::removeItemFromInventoryBase(sp<AEquipment> item)
{
	// Find base which is in the current building
	StateRef<Base> base;
	for (auto &b : state->player_bases)
	{
		// TODO: Fix this to be the building agent is currently in
		if (b.second->building == currentAgent->home_base->building)
		{
			base = { state.get(), b.first };
		}
	}

	// Remove item from base
	if (item->type->type == AEquipmentType::Type::Ammo)
	{
		base->inventoryAgentEquipment[item->type->id] -= item->ammo;
	}
	else
	{
		base->inventoryAgentEquipment[item->type->id]--;
	}

	// If it's a weapon try to load it
	if (item->type->type == AEquipmentType::Type::Weapon)
	{
		auto it = item->type->ammo_types.rbegin();
		while (it != item->type->ammo_types.rend())
		{
			if (base->inventoryAgentEquipment[(*it).id] > 0)
			{
				int count = std::min((*it)->max_ammo, (int)base->inventoryAgentEquipment[(*it).id]);
				base->inventoryAgentEquipment[(*it).id] -= count;
				item->ammo = count;
				item->payloadType = *it;
				break;
			}
			it++;
		}
	}
}

void AEquipScreen::removeItemFromInventoryTemporary(sp<AEquipment> item)
{

}

void AEquipScreen::removeItemFromInventoryBuilding(sp<AEquipment> item)
{

}

void AEquipScreen::removeItemFromInventoryVehicle(sp<AEquipment> item)
{

}

void AEquipScreen::addItemToInventory(sp<AEquipment> item)
{
	switch (getMode())
	{
	case Mode::Agent:
		addItemToInventoryTemporary(item);
		break;
	case Mode::Base:
		addItemToInventoryBase(item);
		break;
	case Mode::Battle:
		addItemToInventoryBattle(item);
		break;
	case Mode::Building:
		addItemToInventoryBuilding(item);
		break;
	case Mode::Vehicle:
		addItemToInventoryVehicle(item);
		break;
	}
}

void AEquipScreen::addItemToInventoryBattle(sp<AEquipment> item)
{
	item->ownerUnit = currentAgent->unit;
	auto bi = state->current_battle->placeItem(
		*state, item,
		currentAgent->unit->position);
	if (bi->findSupport())
	{
		bi->getSupport();
	}
	else
	{
		bi->falling = true;
	}
}

void AEquipScreen::addItemToInventoryBase(sp<AEquipment> item)
{
	// Find base which is in the current building
	StateRef<Base> base;
	for (auto &b : state->player_bases)
	{
		// TODO: Fix this to be the building agent is currently in
		if (b.second->building == currentAgent->home_base->building)
		{
			base = { state.get(), b.first };
		}
	}

	// Unload ammunition and add it too
	sp<AEquipment> ammo = nullptr;
	if (item->payloadType)
	{
		ammo = mksp<AEquipment>();
		ammo->type = item->payloadType;
		ammo->ammo = item->ammo;
		item->ammo = 0;
		item->payloadType = nullptr;
		addItemToInventoryBase(ammo);
	}

	if (item->type->type == AEquipmentType::Type::Ammo)
	{
		base->inventoryAgentEquipment[item->type->id] += item->ammo;
	}
	else
	{
		base->inventoryAgentEquipment[item->type->id]++;
	}

}

void AEquipScreen::addItemToInventoryTemporary(sp<AEquipment> item)
{

}

void AEquipScreen::addItemToInventoryBuilding(sp<AEquipment> item)
{

}

void AEquipScreen::addItemToInventoryVehicle(sp<AEquipment> item)
{

}
void AEquipScreen::closeScreen()
{
	bool empty = true;
	for (auto &entry : vehicleItems)
	{
		if (!entry.second.empty())
		{
			empty = false;
			break;
		}
	}
	for (auto &entry : buildingItems)
	{
		if (!entry.second.empty())
		{
			empty = false;
			break;
		}
	}
	for (auto &entry : agentItems)
	{
		if (!entry.second.empty())
		{
			empty = false;
			break;
		}
	}
	if (!empty)
	{
		fw().stageQueueCommand(
		{ StageCmd::Command::PUSH,
			mksp<MessageBox>(tr("WARNING"),
				tr("You will lose any equipment left on the floor. Are you sure you wish to leave this agent?"),
				MessageBox::ButtonOptions::YesNo, [this] {
			fw().stageQueueCommand({ StageCmd::Command::POP });
		}) });
	}
	else
	{
		fw().stageQueueCommand({ StageCmd::Command::POP });
	}
}

void AEquipScreen::displayAgent(sp<Agent> agent)
{
	formAgentStats->findControlTyped<Label>("AGENT_NAME")->setText(agent->name);
	formAgentStats->findControlTyped<Graphic>("SELECTED_PORTRAIT")
		->setImage(agent->getPortrait().photo);
	formAgentStats->findControlTyped<Graphic>("SELECTED_RANK")
		->setImage(bigUnitRanks[(int)agent->rank]);
	// FIXME: Make stats colours part of GameState
	// FIXME: 'initial' colours taken from screenshot, 'current' guessed
	Colour healthInitialColour{ 156, 4, 4 };
	Colour healthCurrentColour{ 220, 68, 68 };
	formAgentStats->findControlTyped<Graphic>("VALUE_1")->setImage(createStatsBar(
		agent->initial_stats.health, agent->current_stats.health, agent->modified_stats.health,
		100, healthInitialColour, healthCurrentColour, { 64, 4 }));
	Colour accuracyInitialColour{ 252, 176, 0 };
	Colour accuracyCurrentColour{ 255, 240, 64 };
	formAgentStats->findControlTyped<Graphic>("VALUE_2")->setImage(
		createStatsBar(agent->initial_stats.accuracy, agent->current_stats.accuracy,
			agent->modified_stats.accuracy, 100,
			accuracyInitialColour, accuracyCurrentColour, { 64, 4 }));
	Colour reactionsInitialColour{ 252, 176, 0 };
	Colour reactionsCurrentColour{ 255, 240, 64 };
	formAgentStats->findControlTyped<Graphic>("VALUE_3")->setImage(
		createStatsBar(agent->initial_stats.reactions, agent->current_stats.reactions,
			agent->modified_stats.reactions, 100,
			reactionsInitialColour, reactionsCurrentColour, { 64, 4 }));
	Colour speedInitialColour{ 12, 156, 56 };
	Colour speedCurrentColour{ 76, 220, 120 };
	formAgentStats->findControlTyped<Graphic>("VALUE_4")->setImage(createStatsBar(
		agent->initial_stats.speed, agent->current_stats.speed, agent->modified_stats.speed,
		100, speedInitialColour, speedCurrentColour, { 64, 4 }));
	Colour staminaInitialColour{ 12, 156, 56 };
	Colour staminaCurrentColour{ 76, 220, 120 };
	formAgentStats->findControlTyped<Graphic>("VALUE_5")->setImage(createStatsBar(
		agent->initial_stats.getDisplayStaminaValue(), agent->current_stats.getDisplayStaminaValue(), agent->modified_stats.getDisplayStaminaValue(),
		100, staminaInitialColour, staminaCurrentColour, { 64, 4 }));
	Colour braveryInitialColour{ 0, 128, 164 };
	Colour braveryCurrentColour{ 64, 192, 228 };
	formAgentStats->findControlTyped<Graphic>("VALUE_6")->setImage(createStatsBar(
		agent->initial_stats.bravery, agent->current_stats.bravery, agent->modified_stats.bravery,
		100, braveryInitialColour, braveryCurrentColour, { 64, 4 }));
	Colour strengthInitialColour{ 140, 136, 136 };
	Colour strengthCurrentColour{ 204, 200, 200 };
	formAgentStats->findControlTyped<Graphic>("VALUE_7")->setImage(
		createStatsBar(agent->initial_stats.strength, agent->current_stats.strength,
			agent->modified_stats.strength, 100,
			strengthInitialColour, strengthCurrentColour, { 64, 4 }));
	Colour psi_energyInitialColour{ 192, 56, 144 };
	Colour psi_energyCurrentColour{ 255, 120, 208 };
	formAgentStats->findControlTyped<Graphic>("VALUE_8")->setImage(
		createStatsBar(agent->initial_stats.psi_energy, agent->current_stats.psi_energy,
			agent->modified_stats.psi_energy, 100,
			psi_energyInitialColour, psi_energyCurrentColour, { 64, 4 }));
	Colour psi_attackInitialColour{ 192, 56, 144 };
	Colour psi_attackCurrentColour{ 255, 120, 208 };
	formAgentStats->findControlTyped<Graphic>("VALUE_9")->setImage(
		createStatsBar(agent->initial_stats.psi_attack, agent->current_stats.psi_attack,
			agent->modified_stats.psi_attack, 100,
			psi_attackInitialColour, psi_attackCurrentColour, { 64, 4 }));
	Colour psi_defenceInitialColour{ 192, 56, 144 };
	Colour psi_defenceCurrentColour{ 255, 120, 208 };
	formAgentStats->findControlTyped<Graphic>("VALUE_10")
		->setImage(
			createStatsBar(agent->initial_stats.psi_defence, agent->current_stats.psi_defence,
				agent->modified_stats.psi_defence, 100,
				psi_defenceInitialColour, psi_defenceCurrentColour, { 64, 4 }));

	formActive = formAgentStats;
}

void AEquipScreen::setSelectedAgent(sp<Agent> agent)
{
	this->currentAgent = agent;
	this->paperDoll->setObject(agent);
	displayAgent(agent);
	refreshInventoryItems();
}

// FIXME: Put this in the rules somewhere?
// FIXME: This could be shared with the citview ICON_RESOURCES?
static const UString agentFramePath =
"PCK:xcom3/ufodata/vs_icon.pck:xcom3/ufodata/vs_icon.tab:37:xcom3/ufodata/pal_01.dat";

sp<Control> AEquipScreen::createAgentControl(Vec2<int> size, StateRef<Agent> agent)
{
	auto baseControl = mksp<Control>();
	baseControl->setData(agent.getSp());
	baseControl->Name = "AGENT_PORTRAIT";
	baseControl->Size = size;

	auto frameGraphic = baseControl->createChild<Graphic>(fw().data->loadImage(agentFramePath));
	frameGraphic->AutoSize = true;
	frameGraphic->Location = { 0, 0 };
	auto photoGraphic = frameGraphic->createChild<Graphic>(agent->getPortrait().icon);
	photoGraphic->AutoSize = true;
	photoGraphic->Location = { 1, 1 };

	// TODO: Fade portraits
	bool faded = false;

	if (faded)
	{
		auto fadeIcon = baseControl->createChild<Graphic>(iconShade);
		fadeIcon->AutoSize = true;
		fadeIcon->Location = { 2, 1 };
	}

	auto rankIcon = baseControl->createChild<Graphic>(unitRanks[(int)agent->rank]);
	rankIcon->AutoSize = true;
	rankIcon->Location = { 0, 0 };

	bool shield = agent->getMaxShield() > 0;
	
	float maxHealth;
	float currentHealth;
	float stunProportion;
	if (shield)
	{
		currentHealth = agent->getShield();
		maxHealth = agent->getMaxShield();
	}
	else
	{
		currentHealth = agent->getHealth();
		maxHealth = agent->getMaxHealth();
		if (agent->unit)
		{ 
			float stunHealth = agent->unit->stunDamage;
			stunProportion = stunHealth / maxHealth;
		}
	}
	float healthProportion = maxHealth == 0.0f ? 0.0f : currentHealth / maxHealth;
	stunProportion = clamp(stunProportion, 0.0f, healthProportion);
	
	if (healthProportion > 0.0f)
	{
		// FIXME: Put these somewhere slightly less magic?
		Vec2<int> healthBarOffset = { 27, 2 };
		Vec2<int> healthBarSize = { 3, 20 };

		auto healthImg = shield ? this->shieldImage : this->healthImage;
		auto healthGraphic = frameGraphic->createChild<Graphic>(healthImg);
		// This is a bit annoying as the health bar starts at the bottom, but the coord origin is
		// top-left, so fix that up a bit
		int healthBarHeight = (int)((float)healthBarSize.y * healthProportion);
		healthBarOffset.y = healthBarOffset.y + (healthBarSize.y - healthBarHeight);
		healthBarSize.y = healthBarHeight;
		healthGraphic->Location = healthBarOffset;
		healthGraphic->Size = healthBarSize;
		healthGraphic->ImagePosition = FillMethod::Stretch;
	}
	if (stunProportion > 0.0f)
	{
		// FIXME: Put these somewhere slightly less magic?
		Vec2<int> healthBarOffset = { 27, 2 };
		Vec2<int> healthBarSize = { 3, 20 };

		auto healthImg = this->stunImage;
		auto healthGraphic = frameGraphic->createChild<Graphic>(healthImg);
		// This is a bit annoying as the health bar starts at the bottom, but the coord origin is
		// top-left, so fix that up a bit
		int healthBarHeight = (int)((float)healthBarSize.y * stunProportion);
		healthBarOffset.y = healthBarOffset.y + (healthBarSize.y - healthBarHeight);
		healthBarSize.y = healthBarHeight;
		healthGraphic->Location = healthBarOffset;
		healthGraphic->Size = healthBarSize;
		healthGraphic->ImagePosition = FillMethod::Stretch;
	}
	
	auto font = ui().getFont("smalfont");

	auto nameLabel = baseControl->createChild<Label>(agent->name, font);
	nameLabel->Location = { 40, 0 };
	nameLabel->Size = { 100, font->getFontHeight() * 2 };

	return baseControl;
}

} // namespace OpenApoc
