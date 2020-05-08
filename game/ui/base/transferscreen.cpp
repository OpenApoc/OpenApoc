#include "game/ui/base/transferscreen.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/radiobutton.h"
#include "forms/scrollbar.h"
#include "forms/ui.h"
#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/renderer.h"
#include "game/state/city/agentmission.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/gamestate.h"
#include "game/state/rules/city/vammotype.h"
#include "game/state/shared/organisation.h"
#include "game/ui/base/recruitscreen.h"
#include "game/ui/general/agentsheet.h"
#include "game/ui/general/messagebox.h"
#include "game/ui/general/transactioncontrol.h"
#include <array>

namespace OpenApoc
{

TransferScreen::TransferScreen(sp<GameState> state, bool forceLimits)
    : TransactionScreen(state, forceLimits), bigUnitRanks(RecruitScreen::getBigUnitRanks())
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

	textViewSecondBaseStatic = form->findControlTyped<Label>("TEXT_BUTTON_SECOND_BASE_STATIC");
	textViewSecondBaseStatic->setVisible(true);

	// Adding callbacks after checking the button because we don't need to
	// have the callback be called since changeBase() will update display anyways

	form->findControlTyped<RadioButton>("BUTTON_SOLDIERS")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->setDisplayType(Type::Soldier); });
	form->findControlTyped<RadioButton>("BUTTON_BIOSCIS")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->setDisplayType(Type::BioChemist); });
	form->findControlTyped<RadioButton>("BUTTON_PHYSCIS")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->setDisplayType(Type::Physicist); });
	form->findControlTyped<RadioButton>("BUTTON_ENGINRS")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->setDisplayType(Type::Engineer); });
	form->findControlTyped<RadioButton>("BUTTON_ALIENS")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->setDisplayType(Type::Aliens); });

	form->findControlTyped<RadioButton>("BUTTON_VEHICLES")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->setDisplayType(Type::Vehicle); });
	form->findControlTyped<RadioButton>("BUTTON_AGENTS")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->setDisplayType(Type::AgentEquipment); });
	form->findControlTyped<RadioButton>("BUTTON_FLYING")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->setDisplayType(Type::FlyingEquipment); });
	form->findControlTyped<RadioButton>("BUTTON_GROUND")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->setDisplayType(Type::GroundEquipment); });

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

	confirmClosureText = tr("Confirm Transfers");

	type = Type::Soldier;
	form->findControlTyped<RadioButton>("BUTTON_SOLDIERS")->setChecked(true);
}

void TransferScreen::changeSecondBase(sp<Base> newBase)
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

int TransferScreen::getRightIndex()
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
	LogError("The right side base wasn't found.");
	return -1; // should not be reached
}

void TransferScreen::updateBaseHighlight()
{
	if (viewHighlightPrevious != viewHighlight)
	{
		int i = 0;
		for (auto &b : state->player_bases)
		{
			auto viewName = format("BUTTON_SECOND_BASE_%d", ++i);
			auto view = form->findControlTyped<GraphicButton>(viewName);
			auto viewImage = drawMiniBase(*b.second, viewHighlight, viewFacility);
			view->setImage(viewImage);
			view->setDepressedImage(viewImage);
		}
	}

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

void TransferScreen::displayItem(sp<TransactionControl> control)
{
	TransactionScreen::displayItem(control);

	switch (control->itemType)
	{
		case TransactionControl::Type::BioChemist:
		case TransactionControl::Type::Engineer:
		case TransactionControl::Type::Physicist:
		{
			RecruitScreen::personnelSheet(*state->agents[control->itemId], formPersonnelStats);
			formPersonnelStats->setVisible(true);
			break;
		}
		case TransactionControl::Type::Soldier:
		{
			AgentSheet(formAgentStats)
			    .display(*state->agents[control->itemId], bigUnitRanks, false);
			formAgentStats->setVisible(true);
			break;
		}
		default:
			break;
	}
}

void TransferScreen::closeScreen()
{
	auto player = state->getPlayer();

	// Step 01: Check accommodation of different sorts
	{
		std::array<int, MAX_BASES> vecCrewDelta;
		std::array<int, MAX_BASES> vecCargoDelta;
		std::array<int, MAX_BASES> vecBioDelta;
		std::array<bool, MAX_BASES> vecChanged;
		vecCrewDelta.fill(0);
		vecCargoDelta.fill(0);
		vecBioDelta.fill(0);
		vecChanged.fill(false);

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
				int i = 0;
				for ([[maybe_unused]] auto &b : state->player_bases)
				{
					int crewDelta = c->getCrewDelta(i);
					int cargoDelta = c->getCargoDelta(i);
					int bioDelta = c->getBioDelta(i);
					if (cargoDelta || bioDelta || crewDelta)
					{
						vecCrewDelta[i] += crewDelta;
						vecCargoDelta[i] += cargoDelta;
						vecBioDelta[i] += bioDelta;
						vecChanged[i] = true;
					}
					i++;
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

		// Check every base, find first bad one
		int i = 0;
		StateRef<Base> bad_base;
		bool cargoOverLimit = false;
		bool alienOverLimit = false;
		bool crewOverLimit = false;
		for (auto &b : state->player_bases)
		{
			if (vecChanged[i] || forceLimits)
			{
				crewOverLimit = vecCrewDelta[i] > 0 &&
				                b.second->getUsage(*state, FacilityType::Capacity::Quarters,
				                                   vecCrewDelta[i]) > 100;
				cargoOverLimit = vecCargoDelta[i] > 0 &&
				                 b.second->getUsage(*state, FacilityType::Capacity::Stores,
				                                    vecCargoDelta[i]) > 100;
				alienOverLimit =
				    vecBioDelta[i] > 0 && b.second->getUsage(*state, FacilityType::Capacity::Aliens,
				                                             vecBioDelta[i]) > 100;
				if (crewOverLimit || cargoOverLimit || alienOverLimit)
				{
					bad_base = b.second->building->base;
					break;
				}
			}
			i++;
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
				message = tr("Transfer limited by available storage space.");
				type = Type::AgentEquipment;
			}
			else if (alienOverLimit)
			{
				title = tr("Alien Containment space exceeded");
				message = tr("Transfer limited by available Alien Containment space.");
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

	// Step 02: Check transportation
	{
		// Find out who provides transportation services
		std::list<StateRef<Organisation>> badOrgs;
		std::list<StateRef<Organisation>> ferryCompanies;
		for (auto &o : state->organisations)
		{
			if (o.second->providesTransportationServices)
			{
				StateRef<Organisation> org{state.get(), o.first};
				if (o.second->isRelatedTo(player) == Organisation::Relation::Hostile)
				{
					badOrgs.push_back(org);
				}
				else
				{
					ferryCompanies.push_back(org);
				}
			}
		}

		bool transportationBusy = false;
		bool transportationHostile = ferryCompanies.empty();
		if (!transportationHostile)
		{
			// Check if ferry provider has free ferries
			// TODO: rewrite
			if (config().getBool("OpenApoc.NewFeature.CallExistingFerry"))
			{
				bool ferryFound = false;
				for (auto &o : ferryCompanies)
				{
					for (auto &v : state->vehicles)
					{
						if (v.second->owner != o ||
						    (!v.second->type->provideFreightCargo &&
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
				        ? format("%s %s",
				                 tr("This hostile organization refuses to carry out the "
				                    "requested transfer."),
				                 tr("Proceed?"))
				        : format("%s %s",
				                 tr("No free transport to carry out the requested "
				                    "transportation detected in the city."),
				                 tr("Proceed?"));
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH,
				     mksp<MessageBox>(title, message, MessageBox::ButtonOptions::YesNo,
				                      [this] { this->forcedCloseScreen(); })});
				return;
			}
			// Otherwise if transportation is only busy give option
			else if (!transportationHostile)
			{
				// FIXME: Different message maybe? Same for now
				UString message = format("%s %s",
				                         tr("No free transport to carry out the requested "
				                            "transportation detected in the city."),
				                         tr("Proceed?"));
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH,
				     mksp<MessageBox>(title, message, MessageBox::ButtonOptions::YesNo,
				                      [this] { this->forcedCloseScreen(); })});
				return;
			}
			// Otherwise deny
			else
			{
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH,
				     mksp<MessageBox>(title,
				                      tr("This hostile organization refuses to carry out "
				                         "the requested transfer."),
				                      MessageBox::ButtonOptions::Ok)});
				return;
			}
		}
	}

	// Step 03: If we reached this then go!
	executeOrders();
	fw().stageQueueCommand({StageCmd::Command::POP});
	return;
}

void TransferScreen::executeOrders()
{
	auto player = state->getPlayer();

	// Step 01: Re-direct all vehicles and agents if transferring
	std::set<sp<TransactionControl>> linkedControls;
	for (auto t : std::set<Type>{Type::BioChemist, Type::Engineer, Type::Physicist, Type::Soldier,
	                             Type::Vehicle})
	{
		for (auto &c : transactionControls[t])
		{
			if (linkedControls.find(c) != linkedControls.end())
			{
				continue;
			}

			StateRef<Base> newBase;
			int i = 0;
			for (auto &b : state->player_bases)
			{
				if (c->tradeState.shipmentsTotal(i++) == -1)
				{
					newBase = b.second->building->base;
					break;
				}
			}
			if (newBase)
			{
				switch (c->itemType)
				{
					case TransactionControl::Type::BioChemist:
					case TransactionControl::Type::Engineer:
					case TransactionControl::Type::Physicist:
					{
						StateRef<Agent> agent{state.get(), c->itemId};
						if (agent->lab_assigned)
						{
							StateRef<Lab> lab{state.get(), agent->lab_assigned};
							agent->lab_assigned->removeAgent(lab, agent);
						}
						agent->transfer(*state, newBase->building);
						break;
					}
					case TransactionControl::Type::Soldier:
					{
						StateRef<Agent> agent{state.get(), c->itemId};
						if (agent->homeBuilding != newBase->building)
						{
							agent->homeBuilding = newBase->building;
							if (agent->currentBuilding != agent->homeBuilding ||
							    (agent->currentVehicle &&
							     agent->currentVehicle->currentBuilding != agent->homeBuilding))
							{
								if (agent->currentVehicle)
								{
									agent->enterBuilding(*state,
									                     agent->currentVehicle->currentBuilding);
								}
								agent->setMission(*state,
								                  AgentMission::gotoBuilding(*state, *agent));
							}
						}
						break;
					}
					case TransactionControl::Type::Vehicle:
					{
						StateRef<Vehicle> vehicle{state.get(), c->itemId};
						if (vehicle->homeBuilding != newBase->building)
						{
							bool wasInBase = vehicle->currentBuilding == vehicle->homeBuilding;
							vehicle->homeBuilding = newBase->building;
							if (wasInBase)
							{
								vehicle->setMission(*state,
								                    VehicleMission::gotoBuilding(*state, *vehicle));
							}
						}
						break;
					}
					default:
						break;
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

	// Step 03.02: Move stuff
	linkedControls.clear();
	for (auto t : std::set<Type>{Type::AgentEquipment, Type::Aliens, Type::FlyingEquipment,
	                             Type::GroundEquipment})
	{
		for (auto &c : transactionControls[t])
		{
			if (linkedControls.find(c) != linkedControls.end())
			{
				continue;
			}

			int i = 0;
			for (auto &b : state->player_bases)
			{
				int src = i++;
				if (!c->tradeState.shipmentsFrom(src))
				{
					continue;
				}

				int i2 = 0;
				for (auto &b2 : state->player_bases)
				{
					int dest = i2++;
					int order = c->tradeState.getOrder(src, dest);
					if (src == dest || order <= 0)
					{
						continue;
					}

					switch (c->itemType)
					{
						case TransactionControl::Type::AgentEquipmentBio:
						{
							StateRef<AEquipmentType> bio{state.get(), c->itemId};
							b.second->building->cargo.emplace_back(*state, bio, order, 0, player,
							                                       b2.second->building);
							b.second->inventoryBioEquipment[c->itemId] -= order;
							break;
						}
						case TransactionControl::Type::AgentEquipmentCargo:
						{
							StateRef<AEquipmentType> equipment{state.get(), c->itemId};
							int quantity = order * (equipment->type == AEquipmentType::Type::Ammo
							                            ? equipment->max_ammo
							                            : 1);
							// We may be transferring last bullets in a clip here so adjust
							// accordingly
							quantity = std::min(quantity,
							                    (int)b.second->inventoryAgentEquipment[c->itemId]);
							b.second->building->cargo.emplace_back(*state, equipment, quantity, 0,
							                                       player, b2.second->building);
							b.second->inventoryAgentEquipment[c->itemId] -= quantity;
							break;
						}
						case TransactionControl::Type::VehicleAmmo:
						{
							StateRef<VAmmoType> ammo{state.get(), c->itemId};
							b.second->building->cargo.emplace_back(*state, ammo, order, 0, player,
							                                       b2.second->building);
							b.second->inventoryVehicleAmmo[c->itemId] -= order;
							break;
						}
						case TransactionControl::Type::VehicleEquipment:
						{
							StateRef<VEquipmentType> equipment{state.get(), c->itemId};
							b.second->building->cargo.emplace_back(*state, equipment, order, 0,
							                                       player, b2.second->building);
							b.second->inventoryVehicleEquipment[c->itemId] -= order;
							break;
						}
						default:
						{
							LogError("Unhandled TransactionControl::Type %d",
							         static_cast<int>(c->itemType));
							break;
						}
					}
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
}

void TransferScreen::initViewSecondBase()
{
	int i = 0;
	for (auto &b : state->player_bases)
	{
		auto viewName = format("BUTTON_SECOND_BASE_%d", ++i);
		auto view = form->findControlTyped<GraphicButton>(viewName);
		view->setVisible(true);
		if (second_base == b.second)
		{
			currentSecondView = view;
		}
		view->setData(b.second);
		auto viewImage = drawMiniBase(*b.second, viewHighlight, viewFacility);
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
			this->textViewSecondBaseStatic->setVisible(false);
		});
		view->addCallback(FormEventType::MouseLeave, [this](FormsEvent *) {
			// this->textViewSecondBase->setText("");
			this->textViewSecondBase->setVisible(false);
			this->textViewSecondBaseStatic->setVisible(true);
		});
	}
	textViewSecondBase = form->findControlTyped<Label>("TEXT_BUTTON_SECOND_BASE");
	textViewSecondBase->setVisible(false);
}

void TransferScreen::render()
{
	TransactionScreen::render();

	// Highlight second selected base
	auto viewBase = currentSecondView->getData<Base>();
	if (second_base == viewBase)
	{
		Vec2<int> pos = currentSecondView->getLocationOnScreen() - 2;
		Vec2<int> size = currentSecondView->Size + 4;
		fw().renderer->drawRect(pos, size, COLOUR_RED);
	}
}

}; // namespace OpenApoc
