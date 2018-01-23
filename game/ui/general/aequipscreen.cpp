#include "game/ui/general/aequipscreen.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/label.h"
#include "forms/listbox.h"
#include "forms/radiobutton.h"
#include "forms/scrollbar.h"
#include "forms/textedit.h"
#include "forms/ui.h"
#include "framework/apocresources/cursor.h"
#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/font.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "framework/renderer.h"
#include "game/state/battle/battleitem.h"
#include "game/state/battle/battleunit.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/vehicle.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/damage.h"
#include "game/state/shared/aequipment.h"
#include "game/state/shared/agent.h"
#include "game/state/tilemap/tileobject_battleunit.h"
#include "game/ui/base/recruitscreen.h"
#include "game/ui/components/controlgenerator.h"
#include "game/ui/components/equipscreen.h"
#include "game/ui/general/aequipmentsheet.h"
#include "game/ui/general/agentsheet.h"
#include "game/ui/general/messagebox.h"

namespace OpenApoc
{

const Vec2<int> AEquipScreen::EQUIP_GRID_SLOT_SIZE{16, 16};
const Vec2<int> AEquipScreen::EQUIP_GRID_SLOTS{16, 16};

AEquipScreen::AEquipScreen(sp<GameState> state, sp<Agent> firstAgent)
    : Stage(), firstAgent(firstAgent), formMain(ui().getForm("aequipscreen")),
      pal(fw().data->loadPalette("xcom3/ufodata/agenteqp.pcx")), state(state),
      labelFont(ui().getFont("smalfont")), bigUnitRanks(RecruitScreen::getBigUnitRanks())
{
	this->state = state;
	formAgentStats = formMain->findControlTyped<Form>("AGENT_STATS_VIEW");
	formAgentItem = formMain->findControlTyped<Form>("AGENT_ITEM_VIEW");

	auto paperDollPlaceholder = formMain->findControlTyped<Graphic>("PAPER_DOLL");

	this->paperDoll = formMain->createChild<EquipmentPaperDoll>(
	    paperDollPlaceholder->Location, paperDollPlaceholder->Size, EQUIP_GRID_SLOT_SIZE);

	inventoryControl = formMain->findControlTyped<Graphic>("INVENTORY");

	for (int i = 12; i <= 18; i++)
	{
		bigUnitRanks.push_back(
		    fw().data->loadImage(format("PCK:xcom3/tacdata/tacbut.pck:xcom3/tacdata/"
		                                "tacbut.tab:%d:xcom3/tacdata/tactical.pal",
		                                i)));
	}

	// Agent list functionality
	auto agentList = formMain->findControlTyped<ListBox>("AGENT_SELECT_BOX");
	agentList->AlwaysEmitSelectionEvents = true;
	agentList->addCallback(FormEventType::ListBoxChangeSelected, [this](FormsEvent *e) {
		auto list = std::static_pointer_cast<ListBox>(e->forms().RaisedBy);
		auto agent = list->getSelectedData<Agent>();
		if (!agent)
		{
			LogError("No agent in selected data");
			return;
		}
		if (agent->unit && !agent->unit->isConscious())
		{
			return;
		}
		selectAgent(agent, Event::isPressed(e->forms().MouseInfo.Button, Event::MouseButton::Right),
		            modifierLCtrl || modifierRCtrl);
	});

	// Agent name edit
	formAgentStats->findControlTyped<TextEdit>("AGENT_NAME")
	    ->addCallback(FormEventType::TextEditFinish, [this](FormsEvent *e) {
		    auto currentAgent = selectedAgents.empty() ? nullptr : selectedAgents.front();
		    if (currentAgent)
		    {
			    currentAgent->name =
			        std::dynamic_pointer_cast<TextEdit>(e->forms().RaisedBy)->getText();
		    }
		});
	formAgentStats->findControlTyped<TextEdit>("AGENT_NAME")
	    ->addCallback(FormEventType::TextEditCancel, [this](FormsEvent *e) {
		    auto currentAgent = selectedAgents.empty() ? nullptr : selectedAgents.front();
		    if (currentAgent)
		    {
			    std::dynamic_pointer_cast<TextEdit>(e->forms().RaisedBy)
			        ->setText(currentAgent->name);
		    }
		});

	woundImage = fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                         "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                         258));
	FATAL_WOUND_LOCATIONS[BodyPart::Helmet] = {{310, 130}, {307, 117}, {327, 137},
	                                           {312, 128}, {309, 118}, {317, 123}};
	FATAL_WOUND_LOCATIONS[BodyPart::LeftArm] = {{287, 159}, {291, 168}, {288, 177},
	                                            {295, 192}, {289, 185}, {282, 164}};
	FATAL_WOUND_LOCATIONS[BodyPart::Body] = {{337, 207}, {302, 157}, {327, 167},
	                                         {318, 207}, {324, 198}, {315, 174}};
	FATAL_WOUND_LOCATIONS[BodyPart::RightArm] = {{340, 199}, {350, 187}, {344, 173},
	                                             {356, 192}, {352, 167}, {349, 163}};
	FATAL_WOUND_LOCATIONS[BodyPart::Legs] = {{305, 259}, {337, 205}, {319, 221},
	                                         {327, 246}, {311, 231}, {332, 217}};
}

AEquipScreen::~AEquipScreen() = default;

void AEquipScreen::begin()
{
	if (state->current_battle)
	{
		formMain->findControlTyped<Graphic>("DOLLAR")->setVisible(false);
	}
	else
	{
		formMain->findControlTyped<Graphic>("DOLLAR")->setVisible(true);
		formMain->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
	}

	auto owner = state->getPlayer();
	if (state->current_battle)
	{
		owner = state->current_battle->currentPlayer;
	}

	if (firstAgent)
	{
		selectAgent(firstAgent);
	}
	else
	{
		for (auto &agent : state->agents)
		{
			if (!checkAgent(agent.second, owner))
			{
				continue;
			}
			if (agent.second->unit && !agent.second->unit->isConscious())
			{
				continue;
			}
			selectAgent(agent.second);
			break;
		}
	}

	auto mode = getMode();
	if (mode == Mode::Enemy)
	{
		formMain->findControlTyped<Label>("EQUIP_AGENT")->setText(tr("MIND PROBE"));
	}
	else
	{
		formMain->findControlTyped<Label>("EQUIP_AGENT")->setText(tr("EQUIP AGENT"));
	}

	formMain->findControlTyped<RadioButton>("BUTTON_SHOW_WEAPONS")->setChecked(true);

	updateAgents();
}

void AEquipScreen::pause() {}

void AEquipScreen::resume()
{
	modifierLCtrl = false;
	modifierRCtrl = false;
	modifierLShift = false;
	modifierRShift = false;
}

void AEquipScreen::finish() {}

void AEquipScreen::eventOccurred(Event *e)
{
	formMain->eventOccured(e);

	// Modifiers
	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_RCTRL)
		{
			modifierRCtrl = true;
		}
		if (e->keyboard().KeyCode == SDLK_LCTRL)
		{
			modifierLCtrl = true;
		}
		if (e->keyboard().KeyCode == SDLK_RSHIFT)
		{
			modifierRShift = true;
		}
		if (e->keyboard().KeyCode == SDLK_LSHIFT)
		{
			modifierLShift = true;
		}
	}
	if (e->type() == EVENT_KEY_UP)
	{
		if (e->keyboard().KeyCode == SDLK_RCTRL)
		{
			modifierRCtrl = false;
		}
		if (e->keyboard().KeyCode == SDLK_LCTRL)
		{
			modifierLCtrl = false;
		}
		if (e->keyboard().KeyCode == SDLK_RSHIFT)
		{
			modifierRShift = false;
		}
		if (e->keyboard().KeyCode == SDLK_LSHIFT)
		{
			modifierLShift = false;
		}
	}

	// Form buttons
	if (e->type() == EVENT_FORM_INTERACTION && e->forms().EventFlag == FormEventType::ButtonClick)
	{
		if (e->forms().RaisedBy->Name == "BUTTON_OK")
		{
			attemptCloseScreen();
			return;
		}
		if (e->forms().RaisedBy->Name == "BUTTON_LEFT")
		{
			inventoryPage--;
			clampInventoryPage();
			return;
		}
		if (e->forms().RaisedBy->Name == "BUTTON_RIGHT")
		{
			inventoryPage++;
			clampInventoryPage();
			return;
		}
	}

	// Form keyboard controls
	if (e->type() == EVENT_KEY_DOWN)
	{
		if (formAgentStats->findControlTyped<TextEdit>("AGENT_NAME")->isFocused())
			return;
		switch (e->keyboard().KeyCode)
		{
			case SDLK_ESCAPE:
				attemptCloseScreen();
				return;
			case SDLK_RETURN:
				formMain->findControl("BUTTON_OK")->click();
				return;
		}
	}

	// Following is meaningless if we have no agent
	if (selectedAgents.empty())
	{
		return;
	}
	auto currentAgent = selectedAgents.front();

	// Templates:
	if (config().getBool("OpenApoc.NewFeature.EnableAgentTemplates"))
	{
		switch (e->keyboard().KeyCode)
		{
			case SDLK_1:
				processTemplate(1, modifierRCtrl || modifierLCtrl);
				return;
			case SDLK_2:
				processTemplate(2, modifierRCtrl || modifierLCtrl);
				return;
			case SDLK_3:
				processTemplate(3, modifierRCtrl || modifierLCtrl);
				return;
			case SDLK_4:
				processTemplate(4, modifierRCtrl || modifierLCtrl);
				return;
			case SDLK_5:
				processTemplate(5, modifierRCtrl || modifierLCtrl);
				return;
			case SDLK_6:
				processTemplate(6, modifierRCtrl || modifierLCtrl);
				return;
			case SDLK_7:
				processTemplate(7, modifierRCtrl || modifierLCtrl);
				return;
			case SDLK_8:
				processTemplate(8, modifierRCtrl || modifierLCtrl);
				return;
			case SDLK_9:
				processTemplate(9, modifierRCtrl || modifierLCtrl);
				return;
			case SDLK_0:
				processTemplate(0, modifierRCtrl || modifierLCtrl);
				return;
		}
	}

	// Switch between showing armor or items
	if (e->type() == EVENT_FORM_INTERACTION &&
	    e->forms().EventFlag == FormEventType::CheckBoxChange)
	{
		if (e->forms().RaisedBy->Name == "BUTTON_SHOW_WEAPONS")
		{
			refreshInventoryItems();
			return;
		}
		if (e->forms().RaisedBy->Name == "BUTTON_SHOW_ARMOUR")
		{
			refreshInventoryItems();
			return;
		}
	}

	// Check if we've moused over equipment so we can show the stats.
	if (e->type() == EVENT_MOUSE_MOVE && !this->draggedEquipment)
	{
		// Wipe any previously-highlighted stuff
		sp<AEquipment> highlightedEquipment;

		Vec2<int> mousePos{e->mouse().X, e->mouse().Y};

		// Check if we're over any equipment in the paper doll
		auto mouseSlotPos = this->paperDoll->getSlotPositionFromScreenPosition(mousePos);
		auto equipment =
		    std::dynamic_pointer_cast<AEquipment>(currentAgent->getEquipmentAt(mouseSlotPos));
		if (equipment)
		{
			highlightedEquipment = equipment;
		}

		// Check if we're over any equipment in the list at the bottom
		auto posWithinInventory = mousePos;
		posWithinInventory.x += inventoryPage * inventoryControl->Size.x;
		for (auto &tuple : this->inventoryItems)
		{
			if (std::get<0>(tuple).within(posWithinInventory))
			{
				auto mousedEquipment = std::get<2>(tuple);
				// Ensure this item is on screen
				auto rect = std::get<0>(tuple);
				auto pos = rect.p0;
				pos.x -= inventoryPage * inventoryControl->Size.x;
				if (pos.x < inventoryControl->Location.x + formMain->Location.x ||
				    pos.x >= inventoryControl->Location.x + inventoryControl->Size.x +
				                 formMain->Location.x)
				{
					break;
				}
				highlightedEquipment = mousedEquipment;
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
			formAgentStats->setVisible(true);
			formAgentItem->setVisible(false);
		}
	}

	// Item manipulation
	if (currentAgent->type->inventory && getMode() != Mode::Enemy)
	{
		// Picking up items
		if (e->type() == EVENT_MOUSE_DOWN && !this->draggedEquipment)
		{
			handleItemPickup({e->mouse().X, e->mouse().Y});
		}

		// Placing items
		if (e->type() == EVENT_MOUSE_UP && draggedEquipment)
		{
			handleItemPlacement({e->mouse().X, e->mouse().Y});
		}
	}
}

void AEquipScreen::update() { formMain->update(); }

void AEquipScreen::render()
{
	int inventoryBottom =
	    inventoryControl->Location.y + inventoryControl->Size.y + formMain->Location.y;
	fw().stageGetPrevious(this->shared_from_this())->render();
	fw().renderer->setPalette(this->pal);
	formMain->render();

	// Following is meaningless if we have no agent
	if (selectedAgents.empty())
	{
		return;
	}
	auto currentAgent = selectedAgents.front();

	Vec2<int> equipOffset = this->paperDoll->getLocationOnScreen();

	for (auto &tuple : inventoryItems)
	{
		// The gap between the bottom of the inventory image and the count label
		static const int INVENTORY_COUNT_Y_GAP = 4;

		auto item = std::get<2>(tuple);
		auto count = std::get<1>(tuple);
		auto rect = std::get<0>(tuple);
		auto pos = rect.p0;
		pos.x -= inventoryPage * inventoryControl->Size.x;
		auto countImage = count > 0 ? labelFont->getString(format("%d", count)) : nullptr;
		auto &equipmentImage = item->type->equipscreen_sprite;

		if (pos.x < inventoryControl->Location.x + formMain->Location.x ||
		    pos.x >= inventoryControl->Location.x + inventoryControl->Size.x + formMain->Location.x)
		{
			continue;
		}

		fw().renderer->draw(equipmentImage, pos);

		if (countImage)
		{
			Vec2<int> countLabelPosition = pos;
			countLabelPosition.x += equipmentImage->size.x / 2 - countImage->size.x / 2;
			countLabelPosition.y += INVENTORY_COUNT_Y_GAP + item->type->equipscreen_size.y * 16;
			countLabelPosition.y =
			    std::min(countLabelPosition.y, inventoryBottom - (int)countImage->size.y);

			fw().renderer->draw(countImage, countLabelPosition);
		}
	}

	if (currentAgent->unit)
	{
		for (auto &ftw : currentAgent->unit->fatalWounds)
		{
			int wound = 0;
			while (wound < ftw.second && wound < FATAL_WOUND_LOCATIONS[ftw.first].size())
			{
				fw().renderer->draw(woundImage,
				                    formMain->Location + FATAL_WOUND_LOCATIONS[ftw.first][wound]);
				wound++;
			}
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

void AEquipScreen::handleItemPickup(Vec2<int> mousePos)
{
	// Expecting to have an agent
	auto currentAgent = selectedAgents.front();

	// Check if we're over any equipment in the paper doll
	auto slotPos = paperDoll->getSlotPositionFromScreenPosition(mousePos);
	bool alienArtifact = false;
	if (tryPickUpItem(currentAgent, slotPos, modifierLCtrl || modifierRCtrl, &alienArtifact))
	{
		draggedEquipmentOffset = paperDoll->getScreenPositionFromSlotPosition(slotPos) - mousePos;
		paperDoll->updateEquipment();
		displayAgent(currentAgent);
		updateAgentControl(currentAgent);

		// Immediate action: put at the ground
		if ((modifierLShift || modifierRShift) &&
		    config().getBool("OpenApoc.NewFeature.AdvancedInventoryControls"))
		{
			handleItemPlacement(false);
		}
	}
	else if (alienArtifact)
	{
		auto message_box = mksp<MessageBox>(
		    tr("Alien Artifact"), tr("You must research Alien technology before you can use it."),
		    MessageBox::ButtonOptions::Ok);
		fw().stageQueueCommand({StageCmd::Command::PUSH, message_box});
	}
	else // no doll equipment under cursor
	{
		// Check if we're over any equipment in the list at the bottom
		auto posWithinInventory = mousePos;
		posWithinInventory.x += inventoryPage * inventoryControl->Size.x;
		if (tryPickUpItem(posWithinInventory, &alienArtifact))
		{
			refreshInventoryItems();

			// Immediate action: try put in the agent
			if ((modifierLShift || modifierRShift) &&
			    config().getBool("OpenApoc.NewFeature.AdvancedInventoryControls"))
			{
				handleItemPlacement(true);
			}
		}
		else if (alienArtifact)
		{
			auto message_box =
			    mksp<MessageBox>(tr("Alien Artifact"),
			                     tr("You must research Alien technology before you can use it."),
			                     MessageBox::ButtonOptions::Ok);
			fw().stageQueueCommand({StageCmd::Command::PUSH, message_box});
		}
	}
}

void AEquipScreen::handleItemPlacement(Vec2<int> mousePos)
{
	// Expecting to have an agent
	auto currentAgent = selectedAgents.front();

	// Are we over the grid? If so try to place it on the agent.
	auto paperDollControl = paperDoll;
	Vec2<int> equipOffset = paperDollControl->Location + formMain->Location;
	Vec2<int> equipmentPos = mousePos + draggedEquipmentOffset;
	// If this is within the grid try to snap it
	Vec2<int> equipmentGridPos = equipmentPos - equipOffset;
	equipmentGridPos /= EQUIP_GRID_SLOT_SIZE;

	auto draggedFrom = draggedEquipmentOrigin;
	auto draggedType =
	    (draggedFrom.x == -1 && draggedFrom.y == -1) ? draggedEquipment->type : nullptr;
	auto draggedAlternative = draggedEquipmentAlternativePickup;

	bool insufficientTU = false;
	bool alienArtifact = false;
	if (tryPlaceItem(currentAgent, equipmentGridPos, &insufficientTU, &alienArtifact))
	{
		displayAgent(currentAgent);
		updateAgentControl(currentAgent);
		this->paperDoll->updateEquipment();
	}
	else
	{
		refreshInventoryItems();
	}
	if (insufficientTU)
	{
		auto message_box = mksp<MessageBox>(
		    tr("NOT ENOUGH TU'S"),
		    format("%s %d", tr("TU cost per item picked up:"), currentAgent->unit->getPickupCost()),
		    MessageBox::ButtonOptions::Ok);
		fw().stageQueueCommand({StageCmd::Command::PUSH, message_box});
	}
	else if (alienArtifact)
	{
		auto message_box = mksp<MessageBox>(
		    tr("Alien Artifact"), tr("You must research Alien technology before you can use it."),
		    MessageBox::ButtonOptions::Ok);
		fw().stageQueueCommand({StageCmd::Command::PUSH, message_box});
	}
	// Other agents
	for (auto &agent : selectedAgents)
	{
		if (agent == currentAgent)
		{
			continue;
		}
		bool pickedUp = draggedType ? tryPickUpItem(draggedType)
		                            : tryPickUpItem(agent, draggedFrom, draggedAlternative);
		if (!pickedUp)
		{
			continue;
		}
		if (tryPlaceItem(agent, equipmentGridPos))
		{
			updateAgentControl(agent);
			// Refresh if picked up from ground and placed on agent
			if (draggedType)
			{
				refreshInventoryItems();
			}
		}
		else
		{
			// Refresh if picked up from agent and placed on ground
			if (!draggedType)
			{
				refreshInventoryItems();
			}
		}
	}
}

void AEquipScreen::handleItemPlacement(bool toAgent)
{
	// Expecting to have an agent
	auto currentAgent = selectedAgents.front();

	// Are we over the grid? If so try to place it on the agent.
	auto draggedFrom = draggedEquipmentOrigin;
	auto draggedType =
	    (draggedFrom.x == -1 && draggedFrom.y == -1) ? draggedEquipment->type : nullptr;
	auto draggedAlternative = draggedEquipmentAlternativePickup;

	bool insufficientTU = false;
	if (tryPlaceItem(currentAgent, toAgent, &insufficientTU))
	{
		displayAgent(currentAgent);
		updateAgentControl(currentAgent);
		this->paperDoll->updateEquipment();
	}
	else
	{
		refreshInventoryItems();
	}
	if (insufficientTU)
	{
		auto message_box = mksp<MessageBox>(
		    tr("NOT ENOUGH TU'S"),
		    format("%s %d", tr("TU cost per item picked up:"), currentAgent->unit->getPickupCost()),
		    MessageBox::ButtonOptions::Ok);
		fw().stageQueueCommand({StageCmd::Command::PUSH, message_box});
	}
	// Other agents
	for (auto &agent : selectedAgents)
	{
		if (agent == currentAgent)
		{
			continue;
		}
		bool pickedUp = draggedType ? tryPickUpItem(draggedType)
		                            : tryPickUpItem(agent, draggedFrom, draggedAlternative);
		if (!pickedUp)
		{
			continue;
		}
		if (tryPlaceItem(agent, toAgent))
		{
			updateAgentControl(agent);
			// Refresh if picked up from ground and placed on agent
			if (draggedType)
			{
				refreshInventoryItems();
			}
		}
		else
		{
			// Refresh if picked up from agent and placed on ground
			if (!draggedType)
			{
				refreshInventoryItems();
			}
		}
	}
}

void AEquipScreen::selectAgent(sp<Agent> agent, bool inverse, bool additive)
{
	auto firstAgent = selectedAgents.empty() ? nullptr : selectedAgents.front();
	bool fullUpdate = !isInVicinity(agent) && !inverse;
	std::set<sp<Agent>> agentsToUpdate;
	auto pos = std::find(selectedAgents.begin(), selectedAgents.end(), agent);
	// Can't deselect last
	if (inverse && selectedAgents.size() <= 1)
	{
		return;
	}
	// Can't additive if not in vicinity or in battle
	if (additive && (!isInVicinity(agent) || (agent->unit && agent->unit->tileObject)))
	{
		additive = false;
	}
	if (inverse)
	{
		// Agent in selection => remove
		if (pos != selectedAgents.end())
		{
			agentsToUpdate.insert(agent);
			selectedAgents.erase(pos);
		}
	}
	else
	{
		// Agent not selected
		if (pos == selectedAgents.end())
		{
			// If additive add
			if (additive)
			{
				selectedAgents.push_front(agent);
				for (auto &a : selectedAgents)
				{
					agentsToUpdate.insert(a);
				}
			}
			else
			{
				// Agent not in selection => replace selection with agent
				for (auto &a : selectedAgents)
				{
					agentsToUpdate.insert(a);
				}
				selectedAgents.clear();
				selectedAgents.push_back(agent);
				agentsToUpdate.insert(agent);
			}
		}
		// Agent is selected
		else
		{
			if (agent != firstAgent)
			{
				selectedAgents.erase(pos);
				selectedAgents.push_front(agent);
				agentsToUpdate.insert(agent);
				agentsToUpdate.insert(firstAgent);
			}
		}
	}
	updateFirstAgent();
	if (fullUpdate)
	{
		updateAgents();
	}
	else
	{
		for (auto &a : agentsToUpdate)
		{
			updateAgentControl(a);
		}
	}
}

void AEquipScreen::displayItem(sp<AEquipment> item)
{
	bool researched = item->type->canBeUsed(*state, state->getPlayer());
	AEquipmentSheet(formAgentItem).display(item, researched);
	formAgentItem->setVisible(true);
	formAgentStats->setVisible(false);
}

AEquipScreen::Mode AEquipScreen::getMode()
{
	auto currentAgent = selectedAgents.empty() ? nullptr : selectedAgents.front();

	// If viewing an enemy
	if (currentAgent && currentAgent->unit &&
	    currentAgent->unit->owner != state->current_battle->currentPlayer)
	{
		return Mode::Enemy;
	}
	// If agent in battle
	if (currentAgent && currentAgent->unit && currentAgent->unit->tileObject)
	{
		return Mode::Battle;
	}
	// If agent in base and not in vehicle or in vehicle which is parked in base
	else if (currentAgent && getAgentBase(currentAgent))
	{
		return Mode::Base;
	}
	// If agent is in a building which is not any base or in a vehicle which is not parked in base
	else if (currentAgent && getAgentBuilding(currentAgent))
	{
		return Mode::Building;
	}
	// If agent is in vehicle which is not at any base
	else if (currentAgent && getAgentVehicle(currentAgent))
	{
		return Mode::Vehicle;
	}
	// Agent is moving somewhere by foot
	else
	{
		return Mode::Agent;
	}
}

void AEquipScreen::refreshInventoryItems()
{
	inventoryItems.clear();

	auto mode = getMode();
	switch (mode)
	{
		case Mode::Enemy:
			break;
		case Mode::Agent:
			populateInventoryItemsAgent();
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

	if (mode == Mode::Base)
	{
		formMain->findControl("BUTTON_UNDERPANTS")->setVisible(true);
		formMain->findControlTyped<RadioButton>("BUTTON_SHOW_WEAPONS")->setVisible(true);
		formMain->findControlTyped<RadioButton>("BUTTON_SHOW_ARMOUR")->setVisible(true);
	}
	else
	{
		formMain->findControl("BUTTON_UNDERPANTS")->setVisible(false);
		formMain->findControlTyped<RadioButton>("BUTTON_SHOW_WEAPONS")->setVisible(false);
		formMain->findControlTyped<RadioButton>("BUTTON_SHOW_ARMOUR")->setVisible(false);
	}

	clampInventoryPage();
}

void AEquipScreen::populateInventoryItemsBattle()
{
	// Expecting to have an agent
	auto currentAgent = selectedAgents.front();

	// The gap between the end of one inventory image and the start of the next
	static const int INVENTORY_IMAGE_X_GAP = 4;
	Vec2<int> inventoryPosition = inventoryControl->Location + formMain->Location;

	auto itemsOnGround = currentAgent->unit->tileObject->getOwningTile()->getItems();

	for (auto &item : itemsOnGround)
	{
		auto &equipmentImage = item->item->type->equipscreen_sprite;
		Vec2<int> inventoryEndPosition = inventoryPosition;
		inventoryEndPosition.x += equipmentImage->size.x;
		inventoryEndPosition.y += equipmentImage->size.y;

		inventoryItems.push_back(
		    std::make_tuple(Rect<int>{inventoryPosition, inventoryEndPosition}, 0, item->item));

		inventoryPosition.x += INVENTORY_IMAGE_X_GAP + equipmentImage->size.x;
	}
}

void AEquipScreen::populateInventoryItemsBase()
{
	// Expecting to have an agent and that agent to have a base
	auto currentAgent = selectedAgents.front();

	// The gap between the end of one inventory image and the start of the next
	static const int INVENTORY_IMAGE_X_GAP = 4;
	Vec2<int> inventoryPosition = inventoryControl->Location + formMain->Location;

	// Find base which is in the current building
	StateRef<Base> base = getAgentBase(currentAgent);

	for (auto &invPair : base->inventoryAgentEquipment)
	{
		StateRef<AEquipmentType> type = {state.get(), invPair.first};

		// Skip wrong types
		bool showArmor = formMain->findControlTyped<RadioButton>("BUTTON_SHOW_ARMOUR")->isChecked();
		bool showOther =
		    formMain->findControlTyped<RadioButton>("BUTTON_SHOW_WEAPONS")->isChecked();
		if (type->type == AEquipmentType::Type::Armor && !showArmor)
		{
			continue;
		}
		if (type->type != AEquipmentType::Type::Armor && !showOther)
		{
			continue;
		}

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

		inventoryItems.push_back(
		    std::make_tuple(Rect<int>{inventoryPosition, inventoryEndPosition}, count, equipment));

		inventoryPosition.x += INVENTORY_IMAGE_X_GAP + equipmentImage->size.x;
	}
}

void AEquipScreen::populateInventoryItemsVehicle()
{
	// Expecting to have an agent
	auto currentAgent = selectedAgents.front();

	// The gap between the end of one inventory image and the start of the next
	static const int INVENTORY_IMAGE_X_GAP = 4;
	Vec2<int> inventoryPosition = inventoryControl->Location + formMain->Location;

	sp<Vehicle> vehicle = getAgentVehicle(currentAgent);
	auto itemsOnVehicle = vehicleItems[vehicle];

	for (auto &item : itemsOnVehicle)
	{
		auto &equipmentImage = item->type->equipscreen_sprite;
		Vec2<int> inventoryEndPosition = inventoryPosition;
		inventoryEndPosition.x += equipmentImage->size.x;
		inventoryEndPosition.y += equipmentImage->size.y;

		inventoryItems.push_back(
		    std::make_tuple(Rect<int>{inventoryPosition, inventoryEndPosition}, 0, item));

		inventoryPosition.x += INVENTORY_IMAGE_X_GAP + equipmentImage->size.x;
	}
}

void AEquipScreen::populateInventoryItemsBuilding()
{
	// Expecting to have an agent
	auto currentAgent = selectedAgents.front();

	// The gap between the end of one inventory image and the start of the next
	static const int INVENTORY_IMAGE_X_GAP = 4;
	Vec2<int> inventoryPosition = inventoryControl->Location + formMain->Location;

	sp<Building> building = getAgentBuilding(currentAgent);
	auto itemsOnBuilding = buildingItems[building];

	for (auto &item : itemsOnBuilding)
	{
		auto &equipmentImage = item->type->equipscreen_sprite;
		Vec2<int> inventoryEndPosition = inventoryPosition;
		inventoryEndPosition.x += equipmentImage->size.x;
		inventoryEndPosition.y += equipmentImage->size.y;

		inventoryItems.push_back(
		    std::make_tuple(Rect<int>{inventoryPosition, inventoryEndPosition}, 0, item));

		inventoryPosition.x += INVENTORY_IMAGE_X_GAP + equipmentImage->size.x;
	}
}

void AEquipScreen::populateInventoryItemsAgent()
{
	if (selectedAgents.empty())
	{
		return;
	}
	auto currentAgent = selectedAgents.front();

	// The gap between the end of one inventory image and the start of the next
	static const int INVENTORY_IMAGE_X_GAP = 4;
	Vec2<int> inventoryPosition = inventoryControl->Location + formMain->Location;

	auto itemsOnAgent = agentItems[currentAgent->position];

	for (auto &item : itemsOnAgent)
	{
		auto &equipmentImage = item->type->equipscreen_sprite;
		Vec2<int> inventoryEndPosition = inventoryPosition;
		inventoryEndPosition.x += equipmentImage->size.x;
		inventoryEndPosition.y += equipmentImage->size.y;

		inventoryItems.push_back(
		    std::make_tuple(Rect<int>{inventoryPosition, inventoryEndPosition}, 0, item));

		inventoryPosition.x += INVENTORY_IMAGE_X_GAP + equipmentImage->size.x;
	}
}

void AEquipScreen::removeItemFromInventory(sp<AEquipment> item)
{
	switch (getMode())
	{
		case Mode::Enemy:
			LogError("Trying to remove item from inventory in enemy screen!?");
			break;
		case Mode::Agent:
			removeItemFromInventoryAgent(item);
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
	// Expecting to have an agent in a base
	auto currentAgent = selectedAgents.front();

	// Find base which is in the current building
	StateRef<Base> base = getAgentBase(currentAgent);

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

void AEquipScreen::removeItemFromInventoryAgent(sp<AEquipment> item)
{
	// Expecting to have an agent
	auto currentAgent = selectedAgents.front();

	agentItems[currentAgent->position].erase(std::find(agentItems[currentAgent->position].begin(),
	                                                   agentItems[currentAgent->position].end(),
	                                                   item));
}

void AEquipScreen::removeItemFromInventoryBuilding(sp<AEquipment> item)
{
	sp<Building> building = getAgentBuilding(selectedAgents.front());

	buildingItems[building].erase(
	    std::find(buildingItems[building].begin(), buildingItems[building].end(), item));
}

void AEquipScreen::removeItemFromInventoryVehicle(sp<AEquipment> item)
{
	sp<Vehicle> vehicle = getAgentVehicle(selectedAgents.front());

	vehicleItems[vehicle].erase(
	    std::find(vehicleItems[vehicle].begin(), vehicleItems[vehicle].end(), item));
}

void AEquipScreen::addItemToInventory(sp<AEquipment> item)
{
	switch (getMode())
	{
		case Mode::Enemy:
			LogError("Trying to add item to inventory in enemy screen!?");
			break;
		case Mode::Agent:
			addItemToInventoryAgent(item);
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
	// Expecting to have an agent
	auto currentAgent = selectedAgents.front();

	item->ownerUnit = currentAgent->unit;
	auto bi = state->current_battle->placeItem(*state, item, currentAgent->unit->position);
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
	// Expecting to have an agent
	auto currentAgent = selectedAgents.front();

	// Find base which is in the current building
	StateRef<Base> base = getAgentBase(currentAgent);

	// Unload ammunition and add it too
	sp<AEquipment> ammo = nullptr;
	if (item->payloadType)
	{
		ammo = mksp<AEquipment>();
		ammo->type = item->payloadType;
		ammo->ammo = item->ammo;
		item->ammo = 0;
		item->payloadType.clear();
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

void AEquipScreen::addItemToInventoryAgent(sp<AEquipment> item)
{
	// Expecting to have an agent
	auto currentAgent = selectedAgents.front();

	agentItems[currentAgent->position].push_back(item);
}

bool AEquipScreen::tryPickUpItem(sp<Agent> agent, Vec2<int> slotPos, bool alternative,
                                 bool *alienArtifact, bool forced)
{
	alternative = alternative && config().getBool("OpenApoc.NewFeature.AdvancedInventoryControls");
	auto equipment = std::dynamic_pointer_cast<AEquipment>(agent->getEquipmentAt(slotPos));
	if (!equipment)
	{
		return false;
	}
	if (!equipment->canBeUsed(*state) && (alternative || getMode() != Mode::Battle))
	{
		if (alienArtifact)
		{
			*alienArtifact = true;
		}
		return false;
	}
	draggedEquipmentOffset = {0, 0};
	draggedEquipmentOrigin = equipment->equippedPosition;
	draggedEquipmentAlternativePickup = false;
	if (alternative && equipment->payloadType)
	{
		draggedEquipmentAlternativePickup = true;
		draggedEquipment = equipment->unloadAmmo();
	}
	else
	{
		if (alternative && forced)
		{
			return false;
		}
		Vec2<float> slotPos = draggedEquipmentOrigin;
		for (auto &s : agent->type->equipment_layout->slots)
		{
			if (s.bounds.p0 != draggedEquipmentOrigin)
			{
				continue;
			}
			Vec2<float> offset = (s.bounds.p1 - s.bounds.p0) - equipment->type->equipscreen_size;
			slotPos += offset / 2.0f;
			break;
		}
		draggedEquipment = equipment;
		agent->removeEquipment(*state, equipment);
	}
	return true;
}

bool AEquipScreen::tryPickUpItem(Vec2<int> inventoryPos, bool *alienArtifact)
{
	for (auto &tuple : inventoryItems)
	{
		auto rect = std::get<0>(tuple);
		if (rect.within(inventoryPos))
		{
			if (!std::get<2>(tuple)->type->canBeUsed(*state, state->getPlayer()) &&
			    getMode() != Mode::Battle)
			{
				if (alienArtifact)
				{
					*alienArtifact = true;
				}
				return false;
			}

			pickUpItem(std::get<2>(tuple));
			draggedEquipmentOffset = rect.p0 - inventoryPos;
			return true;
		}
	}
	return false;
}

bool AEquipScreen::tryPickUpItem(sp<AEquipmentType> item)
{
	for (auto &tuple : inventoryItems)
	{
		if (std::get<2>(tuple)->type == item)
		{
			pickUpItem(std::get<2>(tuple));
			return true;
		}
	}
	return false;
}

void AEquipScreen::pickUpItem(sp<AEquipment> item)
{
	draggedEquipment = item;
	draggedEquipmentOffset = {0, 0};
	draggedEquipmentOrigin = {-1, -1};
	draggedEquipmentAlternativePickup = false;

	removeItemFromInventory(draggedEquipment);
}

bool AEquipScreen::tryPlaceItem(sp<Agent> agent, Vec2<int> slotPos, bool *insufficientTU,
                                bool *alienArtifact)
{
	bool canAdd = agent->canAddEquipment(slotPos, draggedEquipment->type);
	sp<AEquipment> equipmentUnderCursor = nullptr;
	// If can't add maybe we can insert a clip?
	if (!canAdd)
	{
		equipmentUnderCursor =
		    std::dynamic_pointer_cast<AEquipment>(agent->getEquipmentAt(slotPos));
		if (equipmentUnderCursor &&
		    equipmentUnderCursor->type->type == AEquipmentType::Type::Weapon &&
		    std::find(equipmentUnderCursor->type->ammo_types.begin(),
		              equipmentUnderCursor->type->ammo_types.end(),
		              draggedEquipment->type) != equipmentUnderCursor->type->ammo_types.end())
		{
			if (equipmentUnderCursor->canBeUsed(*state))
			{
				canAdd = true;
			}
			else
			{
				if (alienArtifact)
				{
					*alienArtifact = true;
				}
			}
		}
	}
	if (canAdd && agent->unit && agent->unit->tileObject &&
	    state->current_battle->mode == Battle::Mode::TurnBased && draggedEquipmentOrigin.x == -1 &&
	    draggedEquipmentOrigin.y == -1 &&
	    !agent->unit->spendTU(*state, agent->unit->getPickupCost()))
	{
		canAdd = false;
		if (insufficientTU)
		{
			*insufficientTU = true;
		}
	}
	// Can add item one way or the other
	if (canAdd)
	{
		// Adding to empty slot
		if (!equipmentUnderCursor)
		{
			agent->addEquipment(*state, slotPos, this->draggedEquipment);
		}
		// Inserting ammo
		else
		{
			equipmentUnderCursor->loadAmmo(*state, draggedEquipment);
			if (draggedEquipment->ammo > 0)
			{
				if (draggedEquipmentOrigin.x != -1 && draggedEquipmentOrigin.y != -1 &&
				    agent->canAddEquipment(draggedEquipmentOrigin, draggedEquipment->type))
				{
					agent->addEquipment(*state, draggedEquipmentOrigin, draggedEquipment);
				}
				else
				{
					addItemToInventory(draggedEquipment);
					refreshInventoryItems();
				}
			}
		}
	}
	else // cannot add to agent
	{
		addItemToInventory(draggedEquipment);
	}
	this->draggedEquipment = nullptr;
	return canAdd;
}

bool AEquipScreen::tryPlaceItem(sp<Agent> agent, bool toAgent, bool *insufficientTU)
{
	Vec2<int> slotPos = {-1, 0};
	bool canAdd = toAgent;
	if (canAdd)
	{
		if (slotPos.x == -1 && draggedEquipment->type->type == AEquipmentType::Type::Weapon)
		{
			slotPos =
			    agent->findFirstSlotByType(EquipmentSlotType::RightHand, draggedEquipment->type);
		}
		if (slotPos.x == -1 && draggedEquipment->type->type == AEquipmentType::Type::Weapon)
		{
			slotPos =
			    agent->findFirstSlotByType(EquipmentSlotType::RightHand, draggedEquipment->type);
		}
		if (slotPos.x == -1 && draggedEquipment->type->type == AEquipmentType::Type::Armor)
		{
			slotPos = agent->findFirstSlotByType(
			    AgentType::getArmorSlotType(draggedEquipment->type->body_part),
			    draggedEquipment->type);
		}
		if (slotPos.x == -1 && draggedEquipment->type->type != AEquipmentType::Type::Weapon)
		{
			slotPos =
			    agent->findFirstSlotByType(EquipmentSlotType::General, draggedEquipment->type);
		}
		if (slotPos.x == -1)
		{
			slotPos = agent->findFirstSlot(draggedEquipment->type);
		}
		canAdd = agent->canAddEquipment(slotPos, draggedEquipment->type);
	}
	if (canAdd && agent->unit && agent->unit->tileObject &&
	    state->current_battle->mode == Battle::Mode::TurnBased && draggedEquipmentOrigin.x == -1 &&
	    draggedEquipmentOrigin.y == -1 &&
	    !agent->unit->spendTU(*state, agent->unit->getPickupCost()))
	{
		canAdd = false;
		if (insufficientTU)
		{
			*insufficientTU = true;
		}
	}
	// Can add item
	if (canAdd)
	{
		// Adding to empty slot
		agent->addEquipment(*state, slotPos, this->draggedEquipment);
	}
	else // cannot add to agent
	{
		addItemToInventory(draggedEquipment);
	}
	this->draggedEquipment = nullptr;
	return canAdd;
}

void AEquipScreen::processTemplate(int idx, bool remember)
{
	if (getMode() != Mode::Base || draggedEquipment)
	{
		return;
	}
	auto &temp = state->agentEquipmentTemplates[idx];
	if (remember)
	{
		// Expecting to have an agent
		auto currentAgent = selectedAgents.front();
		// Clear template
		temp.equipment.clear();
		// Copy current inventory
		for (auto &eq : currentAgent->equipment)
		{
			temp.equipment.emplace_back(eq->equippedPosition, eq->type, eq->payloadType);
		}
		// Update
		displayAgent(currentAgent);
		updateAgentControl(currentAgent);
		this->paperDoll->updateEquipment();
		refreshInventoryItems();
	}
	else
	{
		for (auto &currentAgent : selectedAgents)
		{
			// Strip agent
			std::list<sp<AEquipment>> toRemove;
			for (auto &eq : currentAgent->equipment)
			{
				toRemove.push_back(eq);
			}
			for (auto &eq : toRemove)
			{
				currentAgent->removeEquipment(*state, eq);
				addItemToInventory(eq);
			}
			// Find base which is in the current building
			StateRef<Base> base = getAgentBase(currentAgent);

			// Equip agent according to template
			for (auto &eq : temp.equipment)
			{
				auto pos = eq.pos;
				auto type = eq.type;
				auto payloadType = eq.payloadType;

				auto countType = base->inventoryAgentEquipment[type.id];
				int ammoRemainingType = 0;
				auto countPayload = base->inventoryAgentEquipment[type.id];
				int ammoRemainingPayload = 0;

				// Find the type count
				if (type->type == AEquipmentType::Type::Ammo)
				{
					int newCountType = countType / type->max_ammo;
					ammoRemainingType = countType - newCountType * type->max_ammo;
					countType = newCountType;
					if (ammoRemainingType > 0)
					{
						countType++;
					}
				}
				// If no main type item on base item we can't add anything
				if (countType == 0)
				{
					continue;
				}

				// Make item, set payload and ammo
				auto equipment = mksp<AEquipment>();
				equipment->type = type;
				equipment->armor = type->armor;
				if (payloadType)
				{
					// Find the payloadType count
					if (payloadType->type == AEquipmentType::Type::Ammo)
					{
						int newCountPayload = countPayload / payloadType->max_ammo;
						ammoRemainingPayload =
						    countPayload - newCountPayload * payloadType->max_ammo;
						countPayload = newCountPayload;
						if (ammoRemainingPayload > 0)
						{
							countPayload++;
						}
					}

					// If no payload item on base we can't add it
					if (countPayload > 0)
					{
						equipment->payloadType = payloadType;
						if (countPayload == 1 && ammoRemainingPayload > 0)
						{
							equipment->ammo = ammoRemainingPayload;
						}
						else
						{
							equipment->ammo = payloadType->max_ammo;
						}
					}
				}
				else
				{
					if (countType == 1 && ammoRemainingType > 0)
					{
						equipment->ammo = ammoRemainingType;
					}
					else if (type->ammo_types.size() == 0)
					{
						equipment->ammo = type->max_ammo;
					}
				}
				// Actual transaction
				if (currentAgent->canAddEquipment(pos, equipment->type))
				{
					// Give item to agent
					currentAgent->addEquipment(*state, pos, equipment);
					// Remove item from base
					if (type->type == AEquipmentType::Type::Ammo)
					{
						base->inventoryAgentEquipment[type->id] -= equipment->ammo;
					}
					else
					{
						base->inventoryAgentEquipment[type->id]--;
					}
					// Remove payload from base
					if (payloadType && countPayload > 0)
					{
						base->inventoryAgentEquipment[payloadType->id] -= equipment->ammo;
					}
				}
				else
				{
					LogError("Agent %s cannot apply template, fail at pos %s item %s",
					         currentAgent->name, pos, type.id);
				}
			}
			updateAgentControl(currentAgent);
		}
		displayAgent(selectedAgents.front());
		this->paperDoll->updateEquipment();
		refreshInventoryItems();
	}
}

void AEquipScreen::addItemToInventoryBuilding(sp<AEquipment> item)
{
	// Expecting to have an agent
	auto currentAgent = selectedAgents.front();

	buildingItems[getAgentBuilding(currentAgent)].push_back(item);
}

void AEquipScreen::addItemToInventoryVehicle(sp<AEquipment> item)
{
	// Expecting to have an agent
	auto currentAgent = selectedAgents.front();

	vehicleItems[getAgentVehicle(currentAgent)].push_back(item);
}

void AEquipScreen::closeScreen()
{
	// Try to dump gear
	if (config().getBool("OpenApoc.NewFeature.StoreDroppedEquipment"))
	{
		for (auto &entry : vehicleItems)
		{
			if (!entry.second.empty())
			{
				// First unload clips
				std::list<sp<AEquipment>> clips;
				for (auto &e : entry.second)
				{
					auto clip = e->unloadAmmo();
					if (clip)
					{
						clips.push_back(clip);
					}
				}
				for (auto &c : clips)
				{
					entry.second.push_back(c);
				}
				// Then create cargo (ferry to vehicle's home base)
				for (auto &e : entry.second)
				{
					int price = 0;
					entry.first->cargo.emplace_back(
					    *state, e->type, e->type->type == AEquipmentType::Type::Ammo ? e->ammo : 1,
					    price, nullptr, entry.first->homeBuilding);
				}
			}
		}
		for (auto &entry : buildingItems)
		{
			if (!entry.second.empty())
			{
				// First unload clips
				std::list<sp<AEquipment>> clips;
				for (auto &e : entry.second)
				{
					auto clip = e->unloadAmmo();
					if (clip)
					{
						clips.push_back(clip);
					}
				}
				for (auto &c : clips)
				{
					entry.second.push_back(c);
				}
				// Then create cargo (ferry to current base)
				for (auto &e : entry.second)
				{
					int price = 0;
					entry.first->cargo.emplace_back(
					    *state, e->type, e->type->type == AEquipmentType::Type::Ammo ? e->ammo : 1,
					    price, nullptr, state->current_base->building);
				}
			}
		}
		for (auto &entry : agentItems)
		{
			if (!entry.second.empty())
			{
				// First unload clips
				std::list<sp<AEquipment>> clips;
				for (auto &e : entry.second)
				{
					auto clip = e->unloadAmmo();
					if (clip)
					{
						clips.push_back(clip);
					}
				}
				for (auto &c : clips)
				{
					entry.second.push_back(c);
				}
				// First agent in this position
				sp<Agent> dropperAgent;
				for (auto &a : state->agents)
				{
					if (a.second->owner == state->getPlayer() && !a.second->currentBuilding &&
					    !a.second->currentVehicle && (Vec3<int>)a.second->position == entry.first)
					{
						dropperAgent = a.second;
						break;
					}
				}
				if (!dropperAgent)
				{
					LogError("Somehow items got dropped but no agent dropped them!?");
					return;
				}
				// Find building to drop to
				StateRef<Building> buildingToDropTo;
				Vec2<int> pos = {entry.first.x, entry.first.y};
				for (auto &b : dropperAgent->city->buildings)
				{
					if (b.second->bounds.within(pos))
					{
						buildingToDropTo = {state.get(), b.first};
						break;
					}
				}
				if (buildingToDropTo)
				{
					// Create cargo
					for (auto &e : entry.second)
					{
						int price = 0;
						buildingToDropTo->cargo.emplace_back(
						    *state, e->type,
						    e->type->type == AEquipmentType::Type::Ammo ? e->ammo : 1, price,
						    nullptr, dropperAgent->homeBuilding);
					}
				}
			}
		}
	}
	if (!selectedAgents.empty() && state->current_battle)
	{
		auto currentAgent = selectedAgents.front();
		if (currentAgent != firstAgent && selectedAgents.front()->unit->tileObject)
		{
			auto pos =
			    std::find(state->current_battle->battleViewSelectedUnits.begin(),
			              state->current_battle->battleViewSelectedUnits.end(), currentAgent->unit);

			if (pos == state->current_battle->battleViewSelectedUnits.end())
			{
				// Unit not in selection => replace selection with unit
				state->current_battle->battleViewSelectedUnits.clear();
				state->current_battle->battleViewSelectedUnits.push_back(currentAgent->unit);
			}
			// Unit is selected
			else
			{
				state->current_battle->battleViewSelectedUnits.erase(pos);
				state->current_battle->battleViewSelectedUnits.push_front(currentAgent->unit);
			}
			if (currentAgent->unit->squadNumber != -1)
			{
				state->current_battle->battleViewSquadIndex = currentAgent->unit->squadNumber;
			}
		}
	}
	fw().stageQueueCommand({StageCmd::Command::POP});
}

bool AEquipScreen::isInVicinity(sp<Agent> agent)
{
	if (state->current_battle || selectedAgents.empty())
	{
		return true;
	}
	auto currentAgent = selectedAgents.front();
	switch (getMode())
	{
		case Mode::Enemy:
		case Mode::Battle:
			LogError(
			    "Should not be possible for enemy or battle mode to be with no current battle?");
			return true;
		case Mode::Agent:
			return (Vec3<int>)agent->position == (Vec3<int>)currentAgent->position &&
			       !agent->currentVehicle && !agent->currentBuilding;
		case Mode::Base:
		case Mode::Building:
			return getAgentBuilding(agent) &&
			       (getAgentBuilding(agent) == getAgentBuilding(currentAgent));
		case Mode::Vehicle:
			return getAgentVehicle(agent) &&
			       (getAgentVehicle(agent) == getAgentVehicle(currentAgent));
			break;
	}
	return false;
}

StateRef<Building> AEquipScreen::getAgentBuilding(sp<Agent> agent)
{
	return agent->currentBuilding
	           ? agent->currentBuilding
	           : ((agent->currentVehicle && agent->currentVehicle->currentBuilding)
	                  ? agent->currentVehicle->currentBuilding
	                  : nullptr);
}

StateRef<Vehicle> AEquipScreen::getAgentVehicle(sp<Agent> agent)
{
	return agent->currentBuilding ? nullptr : agent->currentVehicle;
}

StateRef<Base> AEquipScreen::getAgentBase(sp<Agent> agent)
{
	return getAgentBuilding(agent) ? getAgentBuilding(agent)->base : nullptr;
}

void AEquipScreen::attemptCloseScreen()
{
	auto owner = state->getPlayer();
	if (state->current_battle)
	{
		owner = state->current_battle->currentPlayer;
	}
	bool empty = true;
	for (auto &entry : vehicleItems)
	{
		if (!entry.second.empty())
		{
			empty = false;

			for (auto &a : state->agents)
			{
				if (!checkAgent(a.second, owner))
				{
					continue;
				}

				if (getAgentVehicle(a.second) == entry.first)
				{
					selectAgent(a.second);
					break;
				}
			}
			break;
		}
	}
	for (auto &entry : buildingItems)
	{
		if (!entry.second.empty())
		{
			empty = false;

			for (auto &a : state->agents)
			{
				if (!checkAgent(a.second, owner))
				{
					continue;
				}

				if (getAgentBuilding(a.second) == entry.first)
				{
					selectAgent(a.second);
					break;
				}
			}
			break;
		}
	}
	for (auto &entry : agentItems)
	{
		if (!entry.second.empty())
		{
			empty = false;

			for (auto &a : state->agents)
			{
				if (!checkAgent(a.second, owner))
				{
					continue;
				}

				if (getAgentBase(a.second) || getAgentBuilding(a.second) ||
				    getAgentVehicle(a.second))
				{
					continue;
				}
				if (entry.first == (Vec3<int>)a.second->position)
				{
					selectAgent(a.second);
					break;
				}
			}
			break;
		}
	}
	if (!empty)
	{
		fw().stageQueueCommand(
		    {StageCmd::Command::PUSH,
		     mksp<MessageBox>(tr("WARNING"), tr("You will lose any equipment left on the floor. "
		                                        "Are you sure you wish to leave this agent?"),
		                      MessageBox::ButtonOptions::YesNo, [this] { this->closeScreen(); })});
	}
	else
	{
		closeScreen();
	}
}

void AEquipScreen::displayAgent(sp<Agent> agent)
{
	formMain->findControlTyped<Graphic>("BACKGROUND")->setImage(agent->type->inventoryBackground);

	AgentSheet(formAgentStats).display(agent, bigUnitRanks, isTurnBased());

	formAgentStats->setVisible(true);
	formAgentItem->setVisible(false);
}

bool AEquipScreen::checkAgent(sp<Agent> agent, sp<Organisation> owner)
{
	if (agent->owner != owner)
	{
		return false;
	}
	// Battle: Unit is not participating in battle or dead
	if (state->current_battle)
	{
		if (!agent->unit || agent->unit->retreated || agent->unit->isDead())
		{
			return false;
		}
	}
	// City: Unit not a soldier
	else
	{
		if (agent->type->role != AgentType::Role::Soldier)
		{
			return false;
		}
	}
	// Unit does not allow direct control
	if (!agent->type->allowsDirectControl)
	{
		return false;
	}
	// Agent checks out
	return true;
}

void AEquipScreen::updateAgents()
{
	auto agentList = formMain->findControlTyped<ListBox>("AGENT_SELECT_BOX");
	agentList->clear();
	auto owner = state->getPlayer();
	if (state->current_battle)
	{
		owner = state->current_battle->currentPlayer;
	}
	if (getMode() != Mode::Enemy)
	{
		for (auto &agent : state->agents)
		{
			sp<Agent> agentSp = agent.second;
			if (!checkAgent(agentSp, owner))
			{
				continue;
			}
			updateAgentControl(agentSp);
		}
	}
	agentList->ItemSize = labelFont->getFontHeight() * 2;
}

void AEquipScreen::updateAgentControl(sp<Agent> agent)
{
	UnitSelectionState selstate = UnitSelectionState::Unselected;
	if (!selectedAgents.empty())
	{
		auto pos = std::find(selectedAgents.begin(), selectedAgents.end(), agent);
		if (pos == selectedAgents.begin())
		{
			selstate = UnitSelectionState::FirstSelected;
		}
		else if (pos != selectedAgents.end())
		{
			selstate = UnitSelectionState::Selected;
		}
	}

	auto agentList = formMain->findControlTyped<ListBox>("AGENT_SELECT_BOX");
	auto control = ControlGenerator::createLargeAgentControl(*state, agent, false, selstate,
	                                                         !isInVicinity(agent));
	control->addCallback(FormEventType::MouseEnter, [this, agent](FormsEvent *e) {
		AgentSheet(formAgentStats).display(agent, bigUnitRanks, isTurnBased());
		formAgentStats->setVisible(true);
		formAgentItem->setVisible(false);
	});
	control->addCallback(FormEventType::MouseLeave, [this](FormsEvent *e) {
		AgentSheet(formAgentStats).display(selectedAgents.front(), bigUnitRanks, isTurnBased());
		formAgentStats->setVisible(true);
		formAgentItem->setVisible(false);
	});
	agentList->replaceItem(control);
}

void AEquipScreen::updateFirstAgent()
{
	auto agent = selectedAgents.empty() ? nullptr : selectedAgents.front();
	this->paperDoll->setObject(agent);
	displayAgent(agent);
	refreshInventoryItems();
}

void AEquipScreen::clampInventoryPage()
{
	if (inventoryPage < 0)
	{
		inventoryPage = 0;
	}
	if (inventoryPage > 0)
	{
		int inventoryLeft = inventoryControl->Location.x + formMain->Location.x;
		int maxPageSeen = 0;
		for (auto &item : inventoryItems)
		{
			auto rect = std::get<0>(item);
			maxPageSeen = (rect.p0.x - inventoryLeft) / inventoryControl->Size.x;
		}
		if (inventoryPage > maxPageSeen)
		{
			inventoryPage = maxPageSeen;
		}
	}
}

bool AEquipScreen::isTurnBased() const
{
	return state->current_battle && state->current_battle->mode == Battle::Mode::TurnBased;
}

} // namespace OpenApoc
