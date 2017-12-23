#include "game/ui/base/transactiontransfer.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/listbox.h"
#include "forms/radiobutton.h"
#include "forms/scrollbar.h"
#include "forms/ui.h"
#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/font.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "framework/logger.h"
#include "framework/renderer.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/gamestate.h"
#include "game/state/rules/city/vammotype.h"
#include "game/state/shared/organisation.h"
#include "game/ui/base/basestage.h"
#include "game/ui/general/messagebox.h"

namespace OpenApoc
{

TransactionTransfer::TransactionTransfer(sp<GameState> state, bool forceLimits)
    : TransactionScreen(state, forceLimits)
{
	form->findControlTyped<Label>("TITLE")->setText(tr("TRANSFER"));
	form->findControlTyped<Graphic>("BG")->setImage(
	    fw().data->loadImage("xcom3/ufodata/transfer.pcx"));
	form->findControlTyped<Graphic>("DOLLAR_ICON")->setVisible(false);
	form->findControlTyped<Label>("TEXT_FUNDS_DELTA")->setVisible(false);
	form->findControlTyped<Graphic>("DELTA_UNDERPANTS")->setVisible(false);

	form->findControlTyped<RadioButton>("BUTTON_SOLDIERS")->setVisible(true);
	form->findControlTyped<RadioButton>("BUTTON_BIOSCIS")->setVisible(true);
	form->findControlTyped<RadioButton>("BUTTON_PHYSCIS")->setVisible(true);
	form->findControlTyped<RadioButton>("BUTTON_ENGINRS")->setVisible(true);
	form->findControlTyped<RadioButton>("BUTTON_ALIENS")->setVisible(true);

	form->findControlTyped<RadioButton>("BUTTON_VEHICLES")->setVisible(true);
	form->findControlTyped<RadioButton>("BUTTON_VEHICLES")->Location.y = 200;
	form->findControlTyped<RadioButton>("BUTTON_AGENTS")->setVisible(true);
	form->findControlTyped<RadioButton>("BUTTON_AGENTS")->Location.y = 240;
	form->findControlTyped<RadioButton>("BUTTON_FLYING")->setVisible(true);
	form->findControlTyped<RadioButton>("BUTTON_FLYING")->Location.y = 280;
	form->findControlTyped<RadioButton>("BUTTON_GROUND")->setVisible(true);
	form->findControlTyped<RadioButton>("BUTTON_GROUND")->Location.y = 320;

	form->findControlTyped<RadioButton>("BUTTON_SOLDIERS")->setChecked(true);

	confirmClosure = tr("Confirm Transfers");
	type = Type::Soldier;

	textViewSecondBaseStatic = form->findControlTyped<Label>("TEXT_BUTTON_SECOND_BASE_STATIC");

	// Find first base that isn't current
	for (auto &b : state->player_bases)
	{
		if (b.first != state->current_base.id)
		{
			second_base = {state.get(), b.first};
			textViewSecondBaseStatic->setText(second_base->name);
			break;
		}
	}
}

void TransactionTransfer::changeSecondBase(sp<Base> newBase)
{
	second_base = newBase->building->base;
	textViewSecondBaseStatic->setText(second_base->name);

	// Set index for all controls
	int index = getRightIndex();
	for (auto &l : transactionControls)
	{
		for (auto &c : l.second)
		{
			c->setIndexRight(index);
		}
	}
	// Apply display type and base highlight
	setDisplayType(type);
}

int TransactionTransfer::getRightIndex()
{
	int index = 0;
	for (auto &b : state->player_bases)
	{
		if (b.first == second_base.id)
		{
			return index;
		}
		index++;
	}
	return 8;
}

void TransactionTransfer::updateBaseHighlight()
{
	// Update first base
	TransactionScreen::updateBaseHighlight();

	// Update second base
	switch (viewHighlight)
	{
		case BaseGraphics::FacilityHighlight::Quarters:
		{
			auto facilityPic = form->findControlTyped<Graphic>("FACILITY_SECOND_PIC");
			facilityPic->setVisible(true);
			facilityPic->setImage(state->facility_types["FACILITYTYPE_LIVING_QUARTERS"]->sprite);
			form->findControlTyped<Graphic>("FACILITY_SECOND_BAR")->setVisible(true);
			int usage = second_base->getUsage(*state, FacilityType::Capacity::Quarters, lq2Delta);
			fillBaseBar(false, usage);
			auto facilityLabel = form->findControlTyped<Label>("FACILITY_SECOND_TEXT");
			facilityLabel->setVisible(true);
			facilityLabel->setText(format("%s%%", usage));
			break;
		}
		case BaseGraphics::FacilityHighlight::Stores:
		{
			auto facilityPic = form->findControlTyped<Graphic>("FACILITY_SECOND_PIC");
			facilityPic->setVisible(true);
			facilityPic->setImage(state->facility_types["FACILITYTYPE_STORES"]->sprite);
			form->findControlTyped<Graphic>("FACILITY_SECOND_BAR")->setVisible(true);
			int usage = second_base->getUsage(*state, FacilityType::Capacity::Stores, cargo2Delta);
			fillBaseBar(false, usage);
			auto facilityLabel = form->findControlTyped<Label>("FACILITY_SECOND_TEXT");
			facilityLabel->setVisible(true);
			facilityLabel->setText(format("%s%%", usage));
			break;
		}
		case BaseGraphics::FacilityHighlight::Aliens:
		{
			auto facilityPic = form->findControlTyped<Graphic>("FACILITY_SECOND_PIC");
			facilityPic->setVisible(true);
			facilityPic->setImage(state->facility_types["FACILITYTYPE_ALIEN_CONTAINMENT"]->sprite);
			form->findControlTyped<Graphic>("FACILITY_SECOND_BAR")->setVisible(true);
			int usage = second_base->getUsage(*state, FacilityType::Capacity::Aliens, bio2Delta);
			fillBaseBar(false, usage);
			auto facilityLabel = form->findControlTyped<Label>("FACILITY_SECOND_TEXT");
			facilityLabel->setVisible(true);
			facilityLabel->setText(format("%s%%", usage));
			break;
		}
		default:
		{
			form->findControlTyped<Graphic>("FACILITY_SECOND_PIC")->setVisible(false);
			form->findControlTyped<Graphic>("FACILITY_SECOND_BAR")->setVisible(false);
			form->findControlTyped<Graphic>("FACILITY_SECOND_FILL")->setVisible(false);
			form->findControlTyped<Label>("FACILITY_SECOND_TEXT")->setVisible(false);
			break;
		}
	}
}

void TransactionTransfer::closeScreen(bool forced)
{
	// Forced means we already asked player to confirm some secondary thing
	// (like there being no free ferries right now)
	if (forced)
	{
		executeOrders();
		fw().stageQueueCommand({StageCmd::Command::POP});
		return;
	}

	// Step 02: Check accomodation of different sorts
	{
		// FIXME: CHECK LQ SPACE
		std::vector<int> vecCargoDelta;
		std::vector<int> vecBioDelta;
		std::vector<bool> vecChanged;
		vecCargoDelta.resize(8);
		vecBioDelta.resize(8);
		vecChanged.resize(8);

		// Find all delta and mark all that have any changes
		std::set<sp<TransactionControl>> linkedControls;
		for (auto &l : transactionControls)
		{
			for (auto &c : l.second)
			{
				if (linkedControls.find(c) != linkedControls.end())
				{
					continue;
				}
				for (int i = 0; i < 8; i++)
				{
					vecCargoDelta[i] += c->getCargoDelta(i);
					vecBioDelta[i] += c->getBioDelta(i);
					if (c->initialStock[i] != c->currentStock[i])
					{
						vecChanged[i] = true;
					}
				}
				for (auto &l : c->getLinked())
				{
					linkedControls.insert(l);
				}
			}
		}

		// Check every base, find first bad one
		int bindex = 0;
		StateRef<Base> bad_base;
		bool cargoOverLimit = false;
		bool alienOverLimit = false;
		bool crewOverLimit = false; //  only if mode == Mode::Transfer
		for (auto &b : state->player_bases)
		{
			if (vecChanged[bindex] || forceLimits)
			{
				if (b.second->getUsage(*state, FacilityType::Capacity::Stores,
				                       vecCargoDelta[bindex]) > 100)
				{
					bad_base = b.second->building->base;
					cargoOverLimit = true;
					break;
				}
			}
			bindex++;
		}

		// Found bad base
		if (bad_base)
		{
			UString title;
			UString message;
			if (crewOverLimit)
			{
				title = tr("Accomodation exceeded");
				message = tr("Transfer limited by available accommodation.");
				type = Type::Soldier;
			}
			else if (cargoOverLimit)
			{
				title = tr("Storage space exceeded");
				{
					message = tr("Transfer limited by available storage space.");
				}
				type = Type::AgentEquipment;
			}
			else if (alienOverLimit)
			{
				title = tr("Alien Containment space exceeded");
				{
					message = tr("Transfer limited by available Alien Containment space.");
				}
				type = Type::Aliens;
			}
			fw().stageQueueCommand(
			    {StageCmd::Command::PUSH,
			     mksp<MessageBox>(title, message, MessageBox::ButtonOptions::Ok)});
			if (bad_base != state->current_base && bad_base != second_base)
			{
				for (auto &view : miniViews)
				{
					if (bad_base == view->getData<Base>())
					{
						currentView = view;
						changeBase(bad_base);
						break;
					}
				}
			}
			return;
		}
	}

	// Step 03: Check transportation

	// Step 03.02: Check transportation for transfers
	// or for purchase-transfers
	//	if (mode == Mode::Transfer || purchaseTransferFound)
	{
		bool transportationHostile = false;
		bool transportationBusy = false;
		std::list<StateRef<Organisation>> badOrgs;

		// Find out who provides transportation services
		std::list<StateRef<Organisation>> ferryCompanies;
		for (auto &o : state->organisations)
		{
			if (o.second->providesTransportationServices)
			{
				ferryCompanies.emplace_back(state.get(), o.first);
			}
		}
		// Check if a ferry provider exists that likes us
		// Clear those that don't
		for (auto it = ferryCompanies.begin(); it != ferryCompanies.end();)
		{
			if ((*it)->isRelatedTo(state->getPlayer()) == Organisation::Relation::Hostile)
			{
				badOrgs.push_back(*it);
				it = ferryCompanies.erase(it);
			}
			else
			{
				it++;
			}
		}
		if (ferryCompanies.empty())
		{
			transportationHostile = true;
		}
		else
		{
			// Check if ferry provider has free ferries
			if (config().getBool("OpenApoc.NewFeature.CallExistingFerry"))
			{
				bool ferryFound = false;
				for (auto &o : ferryCompanies)
				{
					for (auto &v : state->vehicles)
					{
						if (v.second->owner != o || (!v.second->type->provideFreightCargo &&
						                             !v.second->type->provideFreightBio) ||
						    !v.second->missions.empty())
						{
							continue;
						}
						ferryFound = true;
						break;
					}
				}
				if (!ferryFound)
				{
					transportationBusy = true;
					for (auto &o : ferryCompanies)
					{
						badOrgs.push_back(o);
					}
				}
			}
		}

		if (transportationBusy || transportationHostile)
		{
			UString title =
			    format("%s%s", badOrgs.front()->name, badOrgs.size() > 1 ? " & others" : "");

			// If player can ferry themselves then give option
			if (config().getBool("OpenApoc.NewFeature.AllowManualCargoFerry"))
			{
				UString message =
				    transportationHostile
				        ? format("%s %s", tr("This hostile organization refuses to carry out the "
				                             "requested transfer."),
				                 tr("Proceed?"))
				        : format("%s %s", tr("No free transport to carry out the requested "
				                             "transportation detected in the city."),
				                 tr("Proceed?"));
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH,
				     mksp<MessageBox>(title, message, MessageBox::ButtonOptions::YesNo,
				                      [this] { this->closeScreen(true); })});
				return;
			}
			// Otherwise if transportation is only busy give option
			else if (!transportationHostile)
			{
				// FIXME: Different message maybe? Same for now
				UString message = format("%s %s", tr("No free transport to carry out the requested "
				                                     "transportation detected in the city."),
				                         tr("Proceed?"));
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH,
				     mksp<MessageBox>(title, message, MessageBox::ButtonOptions::YesNo,
				                      [this] { this->closeScreen(true); })});
				return;
			}
			// Otherwise deny
			else
			{
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH,
				     mksp<MessageBox>(title, tr("This hostile organization refuses to carry out "
				                                "the requested transfer."),
				                      MessageBox::ButtonOptions::Ok)});
				return;
			}
		}
	}

	// Step 04: If we reached this then go!
	executeOrders();
	fw().stageQueueCommand({StageCmd::Command::POP});
	return;
}

void TransactionTransfer::executeOrders()
{
	std::vector<StateRef<Base>> bases;
	for (auto &b : state->player_bases)
	{
		bases.push_back(b.second->building->base);
	}
	bases.resize(8);

	// Step 01: Re-direct all vehicles and agents if transferring
	//	if (mode == Mode::Transfer)
	{
		// FIXME: IMPLEMENT AGENTS
		for (auto &c : transactionControls[Type::Vehicle])
		{
			StateRef<Base> newBase;
			for (int i = 0; i < 8; i++)
			{
				if (c->currentStock[i] == 1)
				{
					newBase = bases[i];
					break;
				}
			}
			if (!newBase)
			{
				LogError("WTF? VEHICLE VANISHED IN TRANSACTION!?");
				break;
			}
			auto vehicle = StateRef<Vehicle>{state.get(), c->itemId};
			if (vehicle->homeBuilding != newBase->building)
			{
				auto wasInBase = vehicle->currentBuilding == vehicle->homeBuilding;
				vehicle->homeBuilding = newBase->building;
				if (wasInBase)
				{
					vehicle->setMission(*state, VehicleMission::gotoBuilding(*state, *vehicle));
				}
			}
		}
	}

	// Step 02: Gather data about differences in stocks
	// Map is base id to number bought/sold
	std::map<StateRef<AEquipmentType>, std::map<int, int>> aeMap;
	std::map<StateRef<AEquipmentType>, std::map<int, int>> bioMap;
	std::map<StateRef<VEquipmentType>, std::map<int, int>> veMap;
	std::map<StateRef<VAmmoType>, std::map<int, int>> vaMap;
	std::map<StateRef<VehicleType>, std::map<int, int>> vtMap;
	std::list<std::pair<StateRef<Vehicle>, int>> soldVehicles;

	std::set<sp<TransactionControl>> linkedControls;
	for (auto &l : transactionControls)
	{
		for (auto &c : l.second)
		{
			if (linkedControls.find(c) != linkedControls.end())
			{
				continue;
			}
			bool vehicleSold = c->itemType == TransactionControl::Type::Vehicle;
			for (int i = 0; i < 8; i++)
			{
				switch (c->itemType)
				{
					case TransactionControl::Type::Vehicle:
						if (c->currentStock[i] != 0)
						{
							vehicleSold = false;
						}
						break;
					case TransactionControl::Type::AgentEquipmentBio:
						bioMap[{state.get(), c->itemId}][i] =
						    c->currentStock[i] - c->initialStock[i];
						break;
					case TransactionControl::Type::AgentEquipmentCargo:
						aeMap[{state.get(), c->itemId}][i] =
						    c->currentStock[i] - c->initialStock[i];
						break;
					case TransactionControl::Type::VehicleAmmo:
						vaMap[{state.get(), c->itemId}][i] =
						    c->currentStock[i] - c->initialStock[i];
						break;
					case TransactionControl::Type::VehicleEquipment:
						veMap[{state.get(), c->itemId}][i] =
						    c->currentStock[i] - c->initialStock[i];
						break;
					case TransactionControl::Type::VehicleType:
						vtMap[{state.get(), c->itemId}][i] =
						    c->currentStock[i] - c->initialStock[i];
						break;
				}
			}
			if (vehicleSold)
			{
				soldVehicles.emplace_back(StateRef<Vehicle>{state.get(), c->itemId}, c->price);
			}
			for (auto &l : c->getLinked())
			{
				linkedControls.insert(l);
			}
		}
	}

	// Step 03: Act according to mode
	auto player = state->getPlayer();

	// Step 03.02: If transfer then move stuff from negative to positive
	//	if (mode == Mode::Transfer || needTransfer)
	{
		// Agent items
		for (auto &e : aeMap)
		{
			std::list<std::pair<int, int>> source;
			std::list<std::pair<int, int>> destination;
			for (auto &ae : e.second)
			{
				if (ae.second < 0)
				{
					source.emplace_back(ae.first, -ae.second);
				}
				if (ae.second > 0)
				{
					destination.emplace_back(ae.first, ae.second);
				}
			}
			for (auto &d : destination)
			{
				int remaining = d.second;
				for (auto &s : source)
				{
					if (s.second == 0)
					{
						continue;
					}
					int count = std::min(remaining, s.second);
					s.second -= count;
					remaining -= count;
					int realCount =
					    (e.first->type == AEquipmentType::Type::Ammo ? e.first->max_ammo : 1) *
					    count;
					// We may be transferring last bullets in a clip here so adjust accordingly
					realCount = std::min(realCount,
					                     (int)bases[s.first]->inventoryAgentEquipment[e.first.id]);
					bases[s.first]->building->cargo.emplace_back(*state, e.first, count, 0, player,
					                                             bases[d.first]->building);
					bases[s.first]->inventoryAgentEquipment[e.first.id] -= realCount;
					if (remaining == 0)
					{
						break;
					}
				}
				if (remaining != 0)
				{
					LogError("General screw up in transactions, have remaining demand of %d for %s",
					         remaining, e.first.id);
				}
			}
		}
		// Bio items (Aliens)
		for (auto &e : bioMap)
		{
			std::list<std::pair<int, int>> source;
			std::list<std::pair<int, int>> destination;
			for (auto &be : e.second)
			{
				if (be.second < 0)
				{
					source.emplace_back(be.first, be.second);
				}
				if (be.second > 0)
				{
					destination.emplace_back(be.first, be.second);
				}
			}
			for (auto &d : destination)
			{
				int remaining = d.second;
				for (auto &s : source)
				{
					if (s.second == 0)
					{
						continue;
					}
					int count = std::min(remaining, s.second);
					s.second -= count;
					remaining -= count;
					bases[s.first]->building->cargo.emplace_back(*state, e.first, count, 0, player,
					                                             bases[d.first]->building);
					bases[s.first]->inventoryBioEquipment[e.first.id] -= count;
					if (remaining == 0)
					{
						break;
					}
				}
				if (remaining != 0)
				{
					LogError("General screw up in transactions, have remaining demand of %d for %s",
					         remaining, e.first.id);
				}
			}
		}
		// Transfer Vehicle Equipment
		for (auto &e : veMap)
		{
			std::list<std::pair<int, int>> source;
			std::list<std::pair<int, int>> destination;
			for (auto &ve : e.second)
			{
				if (ve.second < 0)
				{
					source.emplace_back(ve.first, ve.second);
				}
				if (ve.second > 0)
				{
					destination.emplace_back(ve.first, ve.second);
				}
			}
			for (auto &d : destination)
			{
				int remaining = d.second;
				for (auto &s : source)
				{
					if (s.second == 0)
					{
						continue;
					}
					int count = std::min(remaining, s.second);
					s.second -= count;
					remaining -= count;
					bases[s.first]->building->cargo.emplace_back(*state, e.first, count, 0, player,
					                                             bases[d.first]->building);
					bases[s.first]->inventoryVehicleEquipment[e.first.id] -= count;
					if (remaining == 0)
					{
						break;
					}
				}
				if (remaining != 0)
				{
					LogError("General screw up in transactions, have remaining demand of %d for %s",
					         remaining, e.first.id);
				}
			}
		}
		// Transfer Vehicle Ammo
		for (auto &e : vaMap)
		{
			std::list<std::pair<int, int>> source;
			std::list<std::pair<int, int>> destination;
			for (auto &va : e.second)
			{
				if (va.second < 0)
				{
					source.emplace_back(va.first, va.second);
				}
				if (va.second > 0)
				{
					destination.emplace_back(va.first, va.second);
				}
			}
			for (auto &d : destination)
			{
				int remaining = d.second;
				for (auto &s : source)
				{
					if (s.second == 0)
					{
						continue;
					}
					int count = std::min(remaining, s.second);
					s.second -= count;
					remaining -= count;
					bases[s.first]->building->cargo.emplace_back(*state, e.first, count, 0, player,
					                                             bases[d.first]->building);
					bases[s.first]->inventoryVehicleAmmo[e.first.id] -= count;
					if (remaining == 0)
					{
						break;
					}
				}
				if (remaining != 0)
				{
					LogError("General screw up in transactions, have remaining demand of %d for %s",
					         remaining, e.first.id);
				}
			}
		}
		// Vehicles and Agents already processed above
		return;
	}
}

void TransactionTransfer::initViewSecondBase()
{
	int b = 0;
	for (auto &pair : state->player_bases)
	{
		auto &viewBase = pair.second;
		auto viewName = format("BUTTON_SECOND_BASE_%d", ++b);
		auto view = form->findControlTyped<GraphicButton>(viewName);
		view->setVisible(true);
		if (second_base == viewBase)
		{
			currentSecondView = view;
		}
		view->setData(viewBase);
		auto viewImage = drawMiniBase(viewBase, viewHighlight, viewFacility);
		view->setImage(viewImage);
		view->setDepressedImage(viewImage);
		wp<GraphicButton> weakView(view);
		view->addCallback(FormEventType::ButtonClick, [this, weakView](FormsEvent *e) {
			auto base = e->forms().RaisedBy->getData<Base>();
			if (this->second_base != base)
			{
				this->changeSecondBase(base);
				this->currentSecondView = weakView.lock();
			}
		});
		view->addCallback(FormEventType::MouseEnter, [this](FormsEvent *e) {
			auto base = e->forms().RaisedBy->getData<Base>();
			this->textViewSecondBase->setText(base->name);
			this->textViewSecondBase->setVisible(true);
		});
		view->addCallback(FormEventType::MouseLeave, [this](FormsEvent *) {
			// this->textViewSecondBase->setText("");
			this->textViewSecondBase->setVisible(false);
		});
	}
	textViewSecondBase = form->findControlTyped<Label>("TEXT_BUTTON_SECOND_BASE");
	textViewSecondBase->setVisible(false);
}

void TransactionTransfer::render()
{
	TransactionScreen::render();

	textViewSecondBaseStatic->setVisible(!textViewSecondBase || !textViewSecondBase->isVisible());

	// Highlight selected base
	if (currentSecondView != nullptr)
	{
		auto viewBase = currentSecondView->getData<Base>();
		if (second_base == viewBase)
		{
			Vec2<int> pos = form->Location + currentSecondView->Location - 2;
			Vec2<int> size = currentSecondView->Size + 4;
			fw().renderer->drawRect(pos, size, COLOUR_RED);
		}
	}
}
}