#include "game/ui/base/transactionscreen.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/listbox.h"
#include "forms/radiobutton.h"
#include "forms/scrollbar.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "framework/logger.h"
#include "framework/renderer.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vequipment.h"
#include "game/state/gamestate.h"
#include "game/state/rules/city/vammotype.h"
#include "game/state/rules/city/vehicletype.h"
#include "game/state/rules/city/vequipmenttype.h"
#include "game/state/shared/organisation.h"
#include "game/ui/general/aequipmentsheet.h"
#include "game/ui/general/messagebox.h"
#include "game/ui/general/transactioncontrol.h"
#include "game/ui/general/vehiclesheet.h"
#include <array>

namespace OpenApoc
{

TransactionScreen::TransactionScreen(sp<GameState> state, bool forceLimits)
    : BaseStage(state), forceLimits(forceLimits)
{
	// Load resources
	form = ui().getForm("transactionscreen");
	formItemAgent = form->findControlTyped<Form>("AGENT_ITEM_VIEW");
	formItemVehicle = form->findControlTyped<Form>("VEHICLE_ITEM_VIEW");
	formAgentStats = form->findControlTyped<Form>("AGENT_STATS_VIEW");
	formPersonnelStats = form->findControlTyped<Form>("PERSONNEL_STATS_VIEW");

	formItemAgent->setVisible(false);
	formItemVehicle->setVisible(false);
	formAgentStats->setVisible(false);
	formPersonnelStats->setVisible(false);

	// Assign event handlers
	onScrollChange = [this](FormsEvent *) { this->updateFormValues(); };
	onHover = [this](FormsEvent *e) {
		auto tctrl = std::dynamic_pointer_cast<TransactionControl>(e->forms().RaisedBy);
		if (!tctrl)
		{
			LogError("Non-Transaction Control called a callback? WTF?");
			return;
		}
		this->displayItem(tctrl);
	};

	// Assign main form contents
	textViewBaseStatic = form->findControlTyped<Label>("TEXT_BUTTON_BASE_STATIC");
}

void TransactionScreen::changeBase(sp<Base> newBase)
{
	BaseStage::changeBase(newBase);
	textViewBaseStatic->setText(state->current_base->name);

	// Set index for all controls
	int index = getLeftIndex();
	for (auto &l : transactionControls)
	{
		for (auto &c : l.second)
		{
			c->setIndexLeft(index);
		}
	}
	// Apply display type and base highlight
	setDisplayType(type);
}

void TransactionScreen::setDisplayType(Type type)
{
	this->type = type;

	formItemAgent->setVisible(false);
	formItemVehicle->setVisible(false);
	formAgentStats->setVisible(false);
	formPersonnelStats->setVisible(false);

	form->findControlTyped<ScrollBar>("LIST_SCROLL")->setValue(0);
	auto list = form->findControlTyped<ListBox>("LIST");
	list->clear();

	// Controls already populated - just add them below, otherwise create them first
	if (transactionControls.find(type) == transactionControls.end())
	{
		// Controls not populated - create them
		switch (type)
		{
			case Type::Soldier:
				populateControlsPeople(AgentType::Role::Soldier);
				break;
			case Type::BioChemist:
				populateControlsPeople(AgentType::Role::BioChemist);
				break;
			case Type::Physicist:
				populateControlsPeople(AgentType::Role::Physicist);
				break;
			case Type::Engineer:
				populateControlsPeople(AgentType::Role::Engineer);
				break;
			case Type::Vehicle:
				populateControlsVehicle();
				break;
			case Type::AgentEquipment:
				populateControlsAgentEquipment();
				break;
			case Type::FlyingEquipment:
			case Type::GroundEquipment:
				populateControlsVehicleEquipment();
				break;
			case Type::Aliens:
				populateControlsAlien();
				break;
		}
	}
	// Highlight
	switch (type)
	{
		case Type::Soldier:
		case Type::BioChemist:
		case Type::Physicist:
		case Type::Engineer:
			viewHighlight = BaseGraphics::FacilityHighlight::Quarters;
			break;
		case Type::Vehicle:
			viewHighlight = BaseGraphics::FacilityHighlight::None;
			break;
		case Type::AgentEquipment:
			viewHighlight = BaseGraphics::FacilityHighlight::Stores;
			break;
		case Type::FlyingEquipment:
		case Type::GroundEquipment:
			viewHighlight = BaseGraphics::FacilityHighlight::Stores;
			break;
		case Type::Aliens:
			viewHighlight = BaseGraphics::FacilityHighlight::Aliens;
			break;
	}
	// Finally add all controls
	for (auto &c : transactionControls[type])
	{
		list->addItem(c);
	}
	// Update display for bases
	updateFormValues(false);
}

int TransactionScreen::getLeftIndex()
{
	int index = 0;
	for (auto &b : state->player_bases)
	{
		if (b.first == state->current_base.id)
		{
			return index;
		}
		index++;
	}
	return ECONOMY_IDX;
}

int TransactionScreen::getRightIndex() { return ECONOMY_IDX; }

void TransactionScreen::populateControlsPeople(AgentType::Role role)
{
	int leftIndex = getLeftIndex();
	int rightIndex = getRightIndex();

	for (auto &a : state->agents)
	{
		if (a.second->owner == state->getPlayer() && a.second->type->role == role)
		{
			auto control = TransactionControl::createControl(
			    *state, StateRef<Agent>{state.get(), a.first}, leftIndex, rightIndex);
			if (control)
			{
				control->addCallback(FormEventType::ScrollBarChange, onScrollChange);
				control->addCallback(FormEventType::MouseMove, onHover);
				transactionControls[type].push_back(control);
			}
		}
	}
}

void TransactionScreen::populateControlsVehicle()
{
	int leftIndex = getLeftIndex();
	int rightIndex = getRightIndex();

	for (auto &v : state->vehicle_types)
	{
		if (state->economy.find(v.first) != state->economy.end())
		{
			auto control = TransactionControl::createControl(
			    *state, StateRef<VehicleType>{state.get(), v.first}, leftIndex, rightIndex);
			if (control)
			{
				control->addCallback(FormEventType::ScrollBarChange, onScrollChange);
				control->addCallback(FormEventType::MouseMove, onHover);
				transactionControls[type].push_back(control);
			}
		}
	}
	for (auto &v : state->vehicles)
	{
		if (v.second->owner == state->getPlayer())
		{
			auto control = TransactionControl::createControl(
			    *state, StateRef<Vehicle>{state.get(), v.first}, leftIndex, rightIndex);
			if (control)
			{
				control->addCallback(FormEventType::ScrollBarChange, onScrollChange);
				control->addCallback(FormEventType::MouseMove, onHover);
				transactionControls[type].push_back(control);
			}
		}
	}
}

void TransactionScreen::populateControlsAgentEquipment()
{
	static const std::list<AEquipmentType::Type> agTypes = {
	    AEquipmentType::Type::Grenade,
	    AEquipmentType::Type::Weapon,
	    // Ammo means everything else
	    AEquipmentType::Type::Ammo,
	    AEquipmentType::Type::Armor,
	    AEquipmentType::Type::Loot,
	};
	int leftIndex = getLeftIndex();
	int rightIndex = getRightIndex();
	for (auto &t : agTypes)
	{
		for (auto &ae : state->agent_equipment)
		{
			if (ae.second->bioStorage)
			{
				continue;
			}
			if (ae.second->type == AEquipmentType::Type::Ammo)
			{
				continue;
			}
			if (t == AEquipmentType::Type::Ammo)
			{
				if (std::find(agTypes.begin(), agTypes.end(), ae.second->type) != agTypes.end())
				{
					continue;
				}
			}
			else
			{
				if (ae.second->type != t)
				{
					continue;
				}
			}
			// Add equipment
			if (state->economy.find(ae.first) != state->economy.end())
			{
				auto control = TransactionControl::createControl(
				    *state, StateRef<AEquipmentType>{state.get(), ae.first}, leftIndex, rightIndex);
				if (control)
				{
					control->addCallback(FormEventType::ScrollBarChange, onScrollChange);
					control->addCallback(FormEventType::MouseMove, onHover);
					transactionControls[type].push_back(control);
				}
			}
			// Add ammo
			for (auto &ammo : ae.second->ammo_types)
			{
				if (state->economy.find(ammo.id) != state->economy.end())
				{
					auto controlAmmo =
					    TransactionControl::createControl(*state, ammo, leftIndex, rightIndex);
					if (controlAmmo)
					{
						controlAmmo->addCallback(FormEventType::ScrollBarChange, onScrollChange);
						controlAmmo->addCallback(FormEventType::MouseMove, onHover);
						transactionControls[type].push_back(controlAmmo);

						// Link to already existing
						if (auto c = findControlById(type, ammo.id))
						{
							TransactionControl::link(c, controlAmmo);
						}
					}
				}
			}
		}
	}
}

void TransactionScreen::populateControlsVehicleEquipment()
{
	bool flying = type == Type::FlyingEquipment;
	static const std::list<EquipmentSlotType> vehTypes = {EquipmentSlotType::VehicleWeapon,
	                                                      EquipmentSlotType::VehicleGeneral,
	                                                      EquipmentSlotType::VehicleEngine};
	int leftIndex = getLeftIndex();
	int rightIndex = getRightIndex();
	for (auto &t : vehTypes)
	{
		for (auto ve_it = state->vehicle_equipment.begin(); ve_it != state->vehicle_equipment.end();
		     ++ve_it)
		{
			auto &ve = *ve_it;
			if (ve.second->type != t)
			{
				continue;
			}
			if (flying &&
			    ve.second->users.find(VEquipmentType::User::Air) == ve.second->users.end())
			{
				continue;
			}
			if (!flying &&
			    ve.second->users.find(VEquipmentType::User::Ground) == ve.second->users.end())
			{
				continue;
			}
			if (state->economy.find(ve.first) == state->economy.end())
			{
				continue;
			}
			auto itemControl = TransactionControl::createControl(
			    *state, StateRef<VEquipmentType>{state.get(), ve.first}, leftIndex, rightIndex);
			if (itemControl)
			{
				itemControl->addCallback(FormEventType::ScrollBarChange, onScrollChange);
				itemControl->addCallback(FormEventType::MouseMove, onHover);
				if (auto otherControl = findControlById(ve.first))
				{
					TransactionControl::link(otherControl, itemControl);
				}
				transactionControls[type].push_back(itemControl);
			}
			// don't add ammo control if not in economy
			if (state->economy.find(ve.second->ammo_type.id) == state->economy.end())
			{
				continue;
			}
			auto veNextIterator = std::next(ve_it, 1);
			// skip this iteration if we're adding a control for the same ammo type in the
			// next iteration
			if (veNextIterator != state->vehicle_equipment.end() &&
			    veNextIterator->second->ammo_type.id == ve.second->ammo_type.id)
			{
				continue;
			}

			auto ammoControl = TransactionControl::createControl(*state, ve.second->ammo_type,
			                                                     leftIndex, rightIndex);
			if (ammoControl)
			{
				ammoControl->addCallback(FormEventType::ScrollBarChange, onScrollChange);
				ammoControl->addCallback(FormEventType::MouseMove, onHover);
				if (auto otherControl = findControlById(ve.second->ammo_type.id))
				{
					TransactionControl::link(otherControl, ammoControl);
				}
				transactionControls[type].push_back(ammoControl);
			}
		}
	}
}

void TransactionScreen::populateControlsAlien()
{
	int leftIndex = getLeftIndex();
	int rightIndex = getRightIndex();
	for (auto &ae : state->agent_equipment)
	{
		if (!ae.second->bioStorage)
		{
			continue;
		}
		// Add alien
		for (auto &b : state->player_bases)
		{
			if (b.second->inventoryBioEquipment[ae.first] > 0)
			{
				auto control = TransactionControl::createControl(
				    *state, StateRef<AEquipmentType>{state.get(), ae.first}, leftIndex, rightIndex);
				if (control)
				{
					control->addCallback(FormEventType::ScrollBarChange, onScrollChange);
					control->addCallback(FormEventType::MouseMove, onHover);
					transactionControls[type].push_back(control);
				}
			}
		}
	}
}

void TransactionScreen::updateFormValues(bool queueHighlightUpdate)
{
	int leftIndex = getLeftIndex();
	int rightIndex = getRightIndex();

	// Crew
	lqDelta = 0;
	lq2Delta = 0;

	// Update storage
	cargoDelta = 0;
	cargo2Delta = 0;
	bioDelta = 0;
	bio2Delta = 0;
	moneyDelta = 0;

	for (auto &l : transactionControls)
	{
		for (auto &c : l.second)
		{
			// if this control is linked to others, only update calculations for one control of the
			// linked group
			// if this control is not linked, update calculations
			if (!c->getLinked() || c->getLinked()->front().lock() == c)
			{
				lqDelta += c->getCrewDelta(leftIndex);
				lq2Delta += c->getCrewDelta(rightIndex);
				cargoDelta += c->getCargoDelta(leftIndex);
				bioDelta += c->getBioDelta(leftIndex);
				cargo2Delta += c->getCargoDelta(rightIndex);
				bio2Delta += c->getBioDelta(rightIndex);
				moneyDelta += c->getPriceDelta();
			}
		}
	}

	if (queueHighlightUpdate)
	{
		framesUntilHighlightUpdate = HIGHLIGHT_UPDATE_DELAY;
	}
	else
	{
		updateBaseHighlight();
	}
}

void TransactionScreen::updateBaseHighlight()
{
	if (viewHighlightPrevious != viewHighlight)
	{
		int i = 0;
		for (auto &b : state->player_bases)
		{
			auto viewName = format("BUTTON_BASE_%d", ++i);
			auto view = form->findControlTyped<GraphicButton>(viewName);
			auto viewImage = drawMiniBase(*b.second, viewHighlight, viewFacility);
			view->setImage(viewImage);
			view->setDepressedImage(viewImage);
		}
		viewHighlightPrevious = viewHighlight;
	}

	switch (viewHighlight)
	{
		case BaseGraphics::FacilityHighlight::Quarters:
		{
			auto facilityPic = form->findControlTyped<Graphic>("FACILITY_FIRST_PIC");
			facilityPic->setVisible(true);
			facilityPic->setImage(state->facility_types["FACILITYTYPE_LIVING_QUARTERS"]->sprite);
			form->findControlTyped<Graphic>("FACILITY_FIRST_BAR")->setVisible(true);
			int usage =
			    state->current_base->getUsage(*state, FacilityType::Capacity::Quarters, lqDelta);
			fillBaseBar(true, usage);
			auto facilityLabel = form->findControlTyped<Label>("FACILITY_FIRST_TEXT");
			facilityLabel->setVisible(true);
			facilityLabel->setText(format("%d%%", usage));
			break;
		}
		case BaseGraphics::FacilityHighlight::Stores:
		{
			auto facilityPic = form->findControlTyped<Graphic>("FACILITY_FIRST_PIC");
			facilityPic->setVisible(true);
			facilityPic->setImage(state->facility_types["FACILITYTYPE_STORES"]->sprite);
			form->findControlTyped<Graphic>("FACILITY_FIRST_BAR")->setVisible(true);
			int usage =
			    state->current_base->getUsage(*state, FacilityType::Capacity::Stores, cargoDelta);
			fillBaseBar(true, usage);
			auto facilityLabel = form->findControlTyped<Label>("FACILITY_FIRST_TEXT");
			facilityLabel->setVisible(true);
			facilityLabel->setText(format("%d%%", usage));
			break;
		}
		case BaseGraphics::FacilityHighlight::Aliens:
		{
			auto facilityPic = form->findControlTyped<Graphic>("FACILITY_FIRST_PIC");
			facilityPic->setVisible(true);
			facilityPic->setImage(state->facility_types["FACILITYTYPE_ALIEN_CONTAINMENT"]->sprite);
			form->findControlTyped<Graphic>("FACILITY_FIRST_BAR")->setVisible(true);
			int usage =
			    state->current_base->getUsage(*state, FacilityType::Capacity::Aliens, bioDelta);
			fillBaseBar(true, usage);
			auto facilityLabel = form->findControlTyped<Label>("FACILITY_FIRST_TEXT");
			facilityLabel->setVisible(true);
			facilityLabel->setText(format("%d%%", usage));
			break;
		}
		default:
		{
			form->findControlTyped<Graphic>("FACILITY_FIRST_PIC")->setVisible(false);
			form->findControlTyped<Graphic>("FACILITY_FIRST_BAR")->setVisible(false);
			form->findControlTyped<Graphic>("FACILITY_FIRST_FILL")->setVisible(false);
			form->findControlTyped<Label>("FACILITY_FIRST_TEXT")->setVisible(false);
			break;
		}
	}
}

void TransactionScreen::fillBaseBar(bool left, int percent)
{
	auto facilityBar = left ? form->findControlTyped<Graphic>("FACILITY_FIRST_FILL")
	                        : form->findControlTyped<Graphic>("FACILITY_SECOND_FILL");
	facilityBar->setVisible(true);

	auto progressImage = mksp<RGBImage>(facilityBar->Size);
	int redHeight = progressImage->size.y * std::min(100, percent) / 100;
	{
		RGBImageLock l(progressImage);
		for (int x = 0; x < 2; x++)
		{
			for (int y = 1; y <= progressImage->size.y; y++)
			{
				if (y <= redHeight)
				{
					l.set({x, progressImage->size.y - y}, COLOUR_RED);
				}
			}
		}
	}
	facilityBar->setImage(progressImage);
}

void TransactionScreen::displayItem(sp<TransactionControl> control)
{
	formItemAgent->setVisible(false);
	formItemVehicle->setVisible(false);
	formAgentStats->setVisible(false);
	formPersonnelStats->setVisible(false);

	switch (control->itemType)
	{
		case TransactionControl::Type::AgentEquipmentBio:
		case TransactionControl::Type::AgentEquipmentCargo:
		{
			AEquipmentSheet(formItemAgent)
			    .display(*state->agent_equipment[control->itemId], control->researched);
			formItemAgent->setVisible(true);
			break;
		}
		case TransactionControl::Type::VehicleType:
		{
			VehicleSheet(formItemVehicle).display(state->vehicle_types[control->itemId]);
			formItemVehicle->setVisible(true);
			break;
		}
		case TransactionControl::Type::Vehicle:
		{
			VehicleSheet(formItemVehicle).display(state->vehicles[control->itemId]);
			formItemVehicle->setVisible(true);
			break;
		}
		case TransactionControl::Type::VehicleEquipment:
		{
			VehicleSheet(formItemVehicle)
			    .display(state->vehicle_equipment[control->itemId], control->researched);
			formItemVehicle->setVisible(true);
			break;
		}
		default:
			break;
	}
}

bool TransactionScreen::isClosable() const
{
	std::set<sp<TransactionControl>> linkedControls;
	for (auto &l : transactionControls)
	{
		for (auto &c : l.second)
		{
			if (linkedControls.find(c) != linkedControls.end())
			{
				continue;
			}

			int i = 0;
			for ([[maybe_unused]] auto &b : state->player_bases)
			{
				if (c->tradeState.shipmentsTotal(i++))
				{
					return false;
				}
			}

			if (c->getLinked())
			{
				for (auto &l : *c->getLinked())
				{
					linkedControls.insert(l.lock());
				}
			}
		}
	}

	return true;
}

void TransactionScreen::attemptCloseScreen()
{
	if (isClosable())
	{
		fw().stageQueueCommand({StageCmd::Command::POP});
	}
	else
	{
		fw().stageQueueCommand({StageCmd::Command::PUSH,
		                        mksp<MessageBox>(
		                            confirmClosureText, "", MessageBox::ButtonOptions::YesNoCancel,
		                            [this] { closeScreen(); },
		                            [] { fw().stageQueueCommand({StageCmd::Command::POP}); })});
	}
}

void TransactionScreen::forcedCloseScreen()
{
	// Forced means we already asked player to confirm some secondary thing
	// (like there being no free ferries right now)
	executeOrders();
	fw().stageQueueCommand({StageCmd::Command::POP});
}

void TransactionScreen::initViewSecondBase()
{
	for (int i = 1; i <= MAX_BASES; i++)
	{
		auto viewName = format("BUTTON_SECOND_BASE_%d", i);
		form->findControlTyped<GraphicButton>(viewName)->setVisible(false);
	}
	form->findControlTyped<Graphic>("FACILITY_SECOND_PIC")->setVisible(false);
	form->findControlTyped<Graphic>("FACILITY_SECOND_BAR")->setVisible(false);
	form->findControlTyped<Graphic>("FACILITY_SECOND_FILL")->setVisible(false);
	form->findControlTyped<Label>("FACILITY_SECOND_TEXT")->setVisible(false);
	form->findControlTyped<Label>("TEXT_BUTTON_SECOND_BASE")->setVisible(false);
}

void TransactionScreen::begin()
{
	BaseStage::begin();

	initViewSecondBase();
}

void TransactionScreen::pause() {}

void TransactionScreen::resume() {}

void TransactionScreen::finish() {}

void TransactionScreen::eventOccurred(Event *e)
{
	form->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		switch (e->keyboard().KeyCode)
		{
			case SDLK_ESCAPE:
			case SDLK_RETURN:
			case SDLK_SPACE:
			case SDLK_KP_ENTER:
				form->findControl("BUTTON_OK")->click();
				return;
		}
	}

	if (e->type() == EVENT_FORM_INTERACTION)
	{
		if (e->forms().EventFlag == FormEventType::ButtonClick)
		{
			if (e->forms().RaisedBy->Name == "BUTTON_OK")
			{
				attemptCloseScreen();
				return;
			}
		}
	}
}

void TransactionScreen::update()
{
	form->update();
	if (framesUntilHighlightUpdate > 0)
	{
		framesUntilHighlightUpdate--;
		if (framesUntilHighlightUpdate == 0)
		{
			updateBaseHighlight();
		}
	}
}

void TransactionScreen::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();

	textViewBaseStatic->setVisible(!textViewBase || !textViewBase->isVisible());

	form->render();
	BaseStage::render();
}

bool TransactionScreen::isTransition() { return false; }

sp<TransactionControl> TransactionScreen::findControlById(Type type, const UString &itemId)
{
	auto it =
	    std::find_if(transactionControls[type].begin(), transactionControls[type].end(),
	                 [itemId](const sp<TransactionControl> &c) { return c->itemId == itemId; });
	if (it != transactionControls[type].end())
	{
		return *it;
	}
	return {};
}
sp<TransactionControl> TransactionScreen::findControlById(const UString &itemId)
{
	static const std::array<Type, 9> types{
	    Type::Soldier,         Type::BioChemist,      Type::Physicist,
	    Type::Engineer,        Type::Vehicle,         Type::AgentEquipment,
	    Type::FlyingEquipment, Type::GroundEquipment, Type::Aliens};
	for (auto type : types)
	{
		auto it = transactionControls.find(type);
		if (it != transactionControls.end())
		{
			auto it2 = std::find_if(
			    it->second.begin(), it->second.end(),
			    [itemId](const sp<TransactionControl> &c) { return c->itemId == itemId; });
			if (it2 != it->second.end())
			{
				return *it2;
			}
		}
	}
	return {};
}

}; // namespace OpenApoc
