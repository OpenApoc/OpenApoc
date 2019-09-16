#include "game/ui/general/transactioncontrol.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/listbox.h"
#include "forms/scrollbar.h"
#include "forms/ui.h"
#include "framework/data.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/renderer.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/research.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vequipment.h"
#include "game/state/gamestate.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/city/vammotype.h"
#include "game/state/shared/organisation.h"
#include "game/ui/general/messagebox.h"

namespace OpenApoc
{

sp<Image> TransactionControl::bgLeft;
sp<Image> TransactionControl::bgRight;
sp<Image> TransactionControl::purchaseBoxIcon;
sp<Image> TransactionControl::purchaseXComIcon;
sp<Image> TransactionControl::purchaseArrow;
sp<Image> TransactionControl::alienContainedDetain;
sp<Image> TransactionControl::alienContainedKill;
sp<Image> TransactionControl::scrollLeft;
sp<Image> TransactionControl::scrollRight;
sp<Image> TransactionControl::transactionShade;
sp<BitmapFont> TransactionControl::labelFont;
bool TransactionControl::resourcesInitialised = false;

void TransactionControl::initResources()
{
	bgLeft = fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx", 45));
	bgRight = fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx", 46));
	purchaseBoxIcon = fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx", 47));
	purchaseXComIcon = fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx", 48));
	purchaseArrow = fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx", 52));
	alienContainedDetain = fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx", 75));
	alienContainedKill = fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx", 76));
	scrollLeft = fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx", 53));
	scrollRight = fw().data->loadImage(format(
	    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:%d:xcom3/ufodata/research.pcx", 54));
	transactionShade = fw().data->loadImage("city/transaction-shade.png");
	labelFont = ui().getFont("smalfont");

	resourcesInitialised = true;
}

void TransactionControl::setScrollbarValues()
{
	if (tradeState.getLeftIndex() == tradeState.getRightIndex())
	{
		scrollBar->setMinimum(0);
		scrollBar->setMaximum(0);
		scrollBar->setValue(0);
	}
	else
	{
		scrollBar->setMinimum(0);
		scrollBar->setMaximum(tradeState.getLeftStock() + tradeState.getRightStock());
		scrollBar->setValue(tradeState.getBalance());
	}
	updateValues();
}

void TransactionControl::setIndexLeft(int index)
{
	tradeState.setLeftIndex(index);
	setScrollbarValues();
}

void TransactionControl::setIndexRight(int index)
{
	tradeState.setRightIndex(index);
	setScrollbarValues();
}

void TransactionControl::updateValues()
{
	if (scrollBar->getMaximum() != 0)
	{
		if (manufacturerHostile || manufacturerUnavailable)
		{
			int defaultRightStock = tradeState.getRightStock();
			if ((tradeState.getLeftIndex() == ECONOMY_IDX &&
			     scrollBar->getValue() > defaultRightStock) ||
			    (tradeState.getRightIndex() == ECONOMY_IDX &&
			     scrollBar->getValue() < defaultRightStock))
			{
				tradeState.cancelOrder();
				scrollBar->setValue(tradeState.getBalance());

				auto message_box = mksp<MessageBox>(
				    manufacturerName,
				    manufacturerHostile ? tr("Order canceled by the hostile manufacturer.")
				                        : tr("Manufacturer has no intact buildings in this city to "
				                             "deliver goods from."),
				    MessageBox::ButtonOptions::Ok);
				fw().stageQueueCommand({StageCmd::Command::PUSH, message_box});
				return;
			}
		}

		// TODO: remove linked
		if (tradeState.getBalance() != scrollBar->getValue())
		{
			tradeState.setBalance(scrollBar->getValue());
			if (linked)
			{
				for (auto &c : *linked)
				{
					if (auto c_sp = c.lock())
					{
						c_sp->suspendUpdates = true;
						c_sp->scrollBar->setValue(scrollBar->getValue());
						c_sp->updateValues();
						c_sp->suspendUpdates = false;
					}
				}
			}
			if (!suspendUpdates)
			{
				this->pushFormEvent(FormEventType::ScrollBarChange, nullptr);
			}
		}
	}

	int curDeltaRight = tradeState.getLROrder();
	int curDeltaLeft = -curDeltaRight;

	stockLeft->setText(format("%d", tradeState.getLeftStock(true)));
	stockRight->setText(format("%d", tradeState.getRightStock(true)));
	deltaLeft->setText(format("%s%d", curDeltaLeft > 0 ? "+" : "", curDeltaLeft));
	deltaRight->setText(format("%s%d", curDeltaRight > 0 ? "+" : "", curDeltaRight));
	deltaLeft->setVisible(tradeState.getLeftIndex() != ECONOMY_IDX && curDeltaLeft != 0);
	deltaRight->setVisible(tradeState.getRightIndex() != ECONOMY_IDX && curDeltaRight != 0);
	setDirty();
}

void TransactionControl::link(sp<TransactionControl> c1, sp<TransactionControl> c2)
{
	if (c1->linked && c2->linked)
	{
		LogError("Cannot link two already linked transaction controls!");
		return;
	}
	if (!c2->linked)
	{
		if (!c1->linked)
		{
			c1->linked = mksp<std::list<wp<TransactionControl>>>();
			c1->linked->emplace_back(c1);
		}
		c1->linked->emplace_back(c2);
		c2->linked = c1->linked;
	}
	if (!c1->linked && c2->linked)
	{
		c2->linked->emplace_back(c1);
		c1->linked = c2->linked;
	}
	// we assume c1 is older than c2, so we update c2 to match c1
	c2->scrollBar->setValue(c1->scrollBar->getValue());
	c2->updateValues();
}

const sp<std::list<wp<TransactionControl>>> &TransactionControl::getLinked() const
{
	return linked;
}

sp<TransactionControl> TransactionControl::createControl(GameState &state, StateRef<Agent> agent,
                                                         int indexLeft, int indexRight)
{
	// The agent or agent's vehicle should be on a base
	auto currentBuilding =
	    agent->currentVehicle ? agent->currentVehicle->currentBuilding : agent->currentBuilding;
	if (!currentBuilding || !currentBuilding->base)
	{
		return nullptr;
	}

	std::vector<int> initialStock;
	// Fill out stock
	{
		initialStock.resize(9);
		// Stock of agents always zero on all bases except where it belongs
		int baseIndex = 0;
		for (auto &b : state.player_bases)
		{
			if (b.first == agent->homeBuilding->base.id)
			{
				initialStock[baseIndex] = 1;
				break;
			}
			baseIndex++;
		}
	}

	Type type;
	switch (agent->type->role)
	{
		case AgentType::Role::BioChemist:
			type = Type::BioChemist;
			break;
		case AgentType::Role::Engineer:
			type = Type::Engineer;
			break;
		case AgentType::Role::Physicist:
			type = Type::Physicist;
			break;
		case AgentType::Role::Soldier:
			type = Type::Soldier;
			break;
		default:
			LogError("Unknown type of agent %s.", agent.id);
			return nullptr;
	}

	int price = 0;
	int storeSpace = 0;
	bool isAmmo = false;
	bool isBio = false;
	bool isPerson = true;
	bool researched = true;
	auto manufacturer = agent->owner;
	bool manufacturerHostile = false;
	bool manufacturerUnavailable = false;

	return createControl(agent.id, type, agent->name, manufacturer, isAmmo, isBio, isPerson,
	                     researched, manufacturerHostile, manufacturerUnavailable, price,
	                     storeSpace, initialStock, indexLeft, indexRight);
}

sp<TransactionControl>
TransactionControl::createControl(GameState &state, StateRef<AEquipmentType> agentEquipmentType,
                                  int indexLeft, int indexRight)
{
	bool isBio = agentEquipmentType->bioStorage;
	int price = 0;
	int storeSpace = agentEquipmentType->store_space;
	bool researched = isBio ? true : state.research.isComplete(agentEquipmentType);

	std::vector<int> initialStock;
	bool hasStock = false;
	// Fill out stock
	{
		initialStock.resize(9);
		int baseIndex = 0;
		for (auto &b : state.player_bases)
		{
			int divisor = (agentEquipmentType->type == AEquipmentType::Type::Ammo && !isBio)
			                  ? agentEquipmentType->max_ammo
			                  : 1;
			initialStock[baseIndex] =
			    isBio ? b.second->inventoryBioEquipment[agentEquipmentType.id]
			          : b.second->inventoryAgentEquipment[agentEquipmentType.id];
			initialStock[baseIndex] = (initialStock[baseIndex] + divisor - 1) / divisor;
			if (initialStock[baseIndex] > 0)
			{
				hasStock = true;
			}
			baseIndex++;
		}
	}
	// Fill out economy data
	if (!agentEquipmentType->bioStorage)
	{
		bool economyUnavailable = true;
		if (state.economy.find(agentEquipmentType.id) != state.economy.end())
		{
			auto &economy = state.economy[agentEquipmentType.id];
			int week = state.gameTime.getWeek();
			initialStock[ECONOMY_IDX] = economy.currentStock;
			price = economy.currentPrice;
			economyUnavailable =
			    economy.weekAvailable == 0 || economy.weekAvailable > week || !researched;
		}
		if (!hasStock && economyUnavailable)
		{
			return nullptr;
		}
	}
	else if (!hasStock)
	{
		return nullptr;
	}

	auto manufacturer = agentEquipmentType->manufacturer;
	bool isAmmo = agentEquipmentType->type == AEquipmentType::Type::Ammo;
	bool isPerson = false;
	auto canBuy = isBio ? Organisation::PurchaseResult::OK
	                    : agentEquipmentType->manufacturer->canPurchaseFrom(
	                          state, state.current_base->building, false);
	bool manufacturerHostile = canBuy == Organisation::PurchaseResult::OrgHostile;
	bool manufacturerUnavailable = manufacturer != state.getPlayer() &&
	                               canBuy == Organisation::PurchaseResult::OrgHasNoBuildings;

	return createControl(agentEquipmentType.id,
	                     isBio ? Type::AgentEquipmentBio : Type::AgentEquipmentCargo,
	                     agentEquipmentType->name, manufacturer, isAmmo, isBio, isPerson,
	                     researched, manufacturerHostile, manufacturerUnavailable, price,
	                     storeSpace, initialStock, indexLeft, indexRight);
}

sp<TransactionControl>
TransactionControl::createControl(GameState &state, StateRef<VEquipmentType> vehicleEquipmentType,
                                  int indexLeft, int indexRight)
{
	int price = 0;
	int storeSpace = vehicleEquipmentType->store_space;
	bool researched = state.research.isComplete(vehicleEquipmentType);

	std::vector<int> initialStock;
	bool hasStock = false;
	// Fill out stock
	{
		initialStock.resize(9);
		int baseIndex = 0;
		for (auto &b : state.player_bases)
		{
			initialStock[baseIndex] = b.second->inventoryVehicleEquipment[vehicleEquipmentType.id];
			if (initialStock[baseIndex] > 0)
			{
				hasStock = true;
			}
			baseIndex++;
		}
	}
	// Fill out economy data
	{
		bool economyUnavailable = true;
		if (state.economy.find(vehicleEquipmentType.id) != state.economy.end())
		{
			auto &economy = state.economy[vehicleEquipmentType.id];
			int week = state.gameTime.getWeek();
			initialStock[ECONOMY_IDX] = economy.currentStock;
			price = economy.currentPrice;
			economyUnavailable =
			    economy.weekAvailable == 0 || economy.weekAvailable > week || !researched;
		}
		if (!hasStock && economyUnavailable)
		{
			return nullptr;
		}
	}

	auto manufacturer = vehicleEquipmentType->manufacturer;
	bool isAmmo = false;
	bool isBio = false;
	bool isPerson = false;
	// Expecting all bases to be in one city
	auto canBuy = vehicleEquipmentType->manufacturer->canPurchaseFrom(
	    state, state.current_base->building, false);
	bool manufacturerHostile = canBuy == Organisation::PurchaseResult::OrgHostile;
	bool manufacturerUnavailable = manufacturer != state.getPlayer() &&
	                               canBuy == Organisation::PurchaseResult::OrgHasNoBuildings;

	return createControl(vehicleEquipmentType.id, Type::VehicleEquipment,
	                     vehicleEquipmentType->name, manufacturer, isAmmo, isBio, isPerson,
	                     researched, manufacturerHostile, manufacturerUnavailable, price,
	                     storeSpace, initialStock, indexLeft, indexRight);
}

sp<TransactionControl> TransactionControl::createControl(GameState &state,
                                                         StateRef<VAmmoType> vehicleAmmoType,
                                                         int indexLeft, int indexRight)
{
	int price = 0;
	int storeSpace = vehicleAmmoType->store_space;
	std::vector<int> initialStock;
	bool hasStock = false;
	// Fill out stock
	{
		initialStock.resize(9);
		int baseIndex = 0;
		for (auto &b : state.player_bases)
		{
			initialStock[baseIndex] = b.second->inventoryVehicleAmmo[vehicleAmmoType.id];
			if (initialStock[baseIndex] > 0)
			{
				hasStock = true;
			}
			baseIndex++;
		}
	}
	// Fill out economy data
	{
		bool economyUnavailable = true;
		if (state.economy.find(vehicleAmmoType.id) != state.economy.end())
		{
			auto &economy = state.economy[vehicleAmmoType.id];
			int week = state.gameTime.getWeek();
			initialStock[ECONOMY_IDX] = economy.currentStock;
			price = economy.currentPrice;
			economyUnavailable = economy.weekAvailable == 0 || economy.weekAvailable > week;
		}
		if (!hasStock && economyUnavailable)
		{
			return nullptr;
		}
	}

	auto manufacturer = vehicleAmmoType->manufacturer;
	bool isAmmo = true;
	bool isBio = false;
	bool isPerson = false;
	bool researched = true;
	// Expecting all bases to be in one city
	auto canBuy =
	    vehicleAmmoType->manufacturer->canPurchaseFrom(state, state.current_base->building, false);
	bool manufacturerHostile = canBuy == Organisation::PurchaseResult::OrgHostile;
	bool manufacturerUnavailable = manufacturer != state.getPlayer() &&
	                               canBuy == Organisation::PurchaseResult::OrgHasNoBuildings;

	return createControl(vehicleAmmoType.id, Type::VehicleAmmo, vehicleAmmoType->name, manufacturer,
	                     isAmmo, isBio, isPerson, researched, manufacturerHostile,
	                     manufacturerUnavailable, price, storeSpace, initialStock, indexLeft,
	                     indexRight);
}

sp<TransactionControl> TransactionControl::createControl(GameState &state,
                                                         StateRef<VehicleType> vehicleType,
                                                         int indexLeft, int indexRight)
{
	// No sense in transfer
	if (indexLeft != ECONOMY_IDX && indexRight != ECONOMY_IDX)
	{
		return nullptr;
	}
	int price = 0;
	int storeSpace = 0;
	std::vector<int> initialStock;
	// Fill out stock
	{
		initialStock.resize(9);
		// Stock of vehicle types always zero
	}
	// Fill out economy data
	{
		bool economyUnavailable = true;
		if (state.economy.find(vehicleType.id) != state.economy.end())
		{
			auto &economy = state.economy[vehicleType.id];
			int week = state.gameTime.getWeek();
			initialStock[ECONOMY_IDX] = economy.currentStock;
			price = economy.currentPrice;
			economyUnavailable = economy.weekAvailable == 0 || economy.weekAvailable > week;
		}
		if (economyUnavailable)
		{
			return nullptr;
		}
	}

	auto manufacturer = vehicleType->manufacturer;
	bool isAmmo = false;
	bool isBio = false;
	bool isPerson = false;
	bool researched = true;
	// Expecting all bases to be in one city
	auto canBuy =
	    vehicleType->manufacturer->canPurchaseFrom(state, state.current_base->building, true);
	bool manufacturerHostile = canBuy == Organisation::PurchaseResult::OrgHostile;
	bool manufacturerUnavailable = manufacturer != state.getPlayer() &&
	                               canBuy == Organisation::PurchaseResult::OrgHasNoBuildings;

	return createControl(vehicleType.id, Type::VehicleType, vehicleType->name, manufacturer, isAmmo,
	                     isBio, isPerson, researched, manufacturerHostile, manufacturerUnavailable,
	                     price, storeSpace, initialStock, indexLeft, indexRight);
}

sp<TransactionControl> TransactionControl::createControl(GameState &state,
                                                         StateRef<Vehicle> vehicle, int indexLeft,
                                                         int indexRight)
{
	// Only parked on base vehicles can be sold
	if (!vehicle->currentBuilding || !vehicle->currentBuilding->base)
	{
		return nullptr;
	}
	int price = 0;
	int storeSpace = 0;
	std::vector<int> initialStock;
	// Fill out stock
	{
		initialStock.resize(9);
		// Stock of vehicle types always zero on all bases except where it belongs
		int baseIndex = 0;
		for (auto &b : state.player_bases)
		{
			if (b.first == vehicle->homeBuilding->base.id)
			{
				initialStock[baseIndex] = 1;
				break;
			}
			baseIndex++;
		}
	}
	// Fill out economy data
	{
		bool economyUnavailable = true;
		if (state.economy.find(vehicle->type.id) != state.economy.end())
		{
			auto &economy = state.economy[vehicle->type.id];
			int week = state.gameTime.getWeek();
			price = economy.currentPrice;
			economyUnavailable = economy.weekAvailable == 0 || economy.weekAvailable > week;
		}
		if (economyUnavailable)
		{
			// Nothing, we can still sell it for parts or transfer!
		}
	}
	LogInfo("Vehicle type %s starting price %d", vehicle->type.id, price);
	// Add price of ammo and equipment
	for (auto &e : vehicle->equipment)
	{
		if (state.economy.find(e->type.id) != state.economy.end())
		{
			price += state.economy[e->type.id].currentPrice;
			if (e->ammo > 0 && state.economy.find(e->type->ammo_type.id) != state.economy.end())
			{
				price += e->ammo * state.economy[e->type->ammo_type.id].currentPrice;
			}
			LogInfo("Vehicle type %s price increased to %d after counting %s", vehicle->type.id,
			        price, e->type.id);
		}
	}
	// Subtract price of default equipment
	for (auto &e : vehicle->type->initial_equipment_list)
	{
		if (state.economy.find(e.second.id) != state.economy.end())
		{
			price -= state.economy[e.second.id].currentPrice;
			LogInfo("Vehicle type %s price decreased to %d after counting %s", vehicle->type.id,
			        price, e.second.id);
		}
	}
	LogInfo("Vehicle type %s final price %d", vehicle->type.id, price);

	auto manufacturer = vehicle->type->manufacturer;
	bool isAmmo = false;
	bool isBio = false;
	bool isPerson = false;
	bool researched = true;
	bool manufacturerHostile = false;
	bool manufacturerUnavailable = false;

	return createControl(vehicle.id, Type::Vehicle, vehicle->name, manufacturer, isAmmo, isBio,
	                     isPerson, researched, manufacturerHostile, manufacturerUnavailable, price,
	                     storeSpace, initialStock, indexLeft, indexRight);
}

sp<TransactionControl>
TransactionControl::createControl(const UString &id, Type type, const UString &name,
                                  StateRef<Organisation> manufacturer, bool isAmmo, bool isBio,
                                  bool isPerson, bool researched, bool manufacturerHostile,
                                  bool manufacturerUnavailable, int price, int storeSpace,
                                  std::vector<int> &initialStock, int indexLeft, int indexRight)
{
	auto control = mksp<TransactionControl>();
	control->itemId = id;
	control->itemType = type;
	control->manufacturer = manufacturer;
	control->isAmmo = isAmmo;
	control->isBio = isBio;
	control->isPerson = isPerson;
	control->researched = researched;
	control->manufacturerHostile = manufacturerHostile;
	control->manufacturerUnavailable = manufacturerUnavailable;
	control->storeSpace = storeSpace;
	control->tradeState.setInitialStock(std::forward<std::vector<int>>(initialStock));
	control->tradeState.setLeftIndex(indexLeft);
	control->tradeState.setRightIndex(indexRight);
	// If we create a non-purchase control we never become one so clear the values
	if (isBio || !researched || (indexLeft != ECONOMY_IDX && indexRight != ECONOMY_IDX))
	{
		control->manufacturerName = "";
		control->price = 0;
	}
	else
	{
		control->manufacturerName = manufacturer->name;
		control->price = price;
	}

	// Setup vars
	control->Size = Vec2<int>{173 + 178 - 2, 47};

	// Setup resources
	if (!resourcesInitialised)
	{
		initResources();
	}

	// Add controls

	// Name
	const UString &labelName = researched ? tr(name) : tr("Alien Artifact");
	if (labelName.length() > 0)
	{
		auto label = control->createChild<Label>(labelName, labelFont);
		label->Location = {isAmmo ? 32 : 11, 3};
		label->Size = {256, 16};
		label->TextHAlign = HorizontalAlignment::Left;
		label->TextVAlign = VerticalAlignment::Centre;
	}
	// Manufacturer
	if (control->manufacturerName.length() > 0)
	{
		auto label = control->createChild<Label>(tr(control->manufacturerName), labelFont);
		if (manufacturerHostile || manufacturerUnavailable)
		{
			label->Tint = {255, 50, 25};
		}
		label->Location = {34, 3};
		label->Size = {256, 16};
		label->TextHAlign = HorizontalAlignment::Right;
		label->TextVAlign = VerticalAlignment::Centre;
	}
	// Price
	if (price != 0 && (indexLeft == ECONOMY_IDX || indexRight == ECONOMY_IDX))
	{
		auto label = control->createChild<Label>(format("$%d", control->price), labelFont);
		label->Location = {290, 3};
		label->Size = {47, 16};
		label->TextHAlign = HorizontalAlignment::Right;
		label->TextVAlign = VerticalAlignment::Centre;
	}
	// Stock (values set in updateValues)
	control->stockLeft = control->createChild<Label>("", labelFont);
	control->stockLeft->Location = {11, 26};
	control->stockLeft->Size = {32, 14};
	control->stockLeft->TextHAlign = HorizontalAlignment::Right;
	control->stockLeft->TextVAlign = VerticalAlignment::Centre;
	control->stockRight = control->createChild<Label>("", labelFont);
	control->stockRight->Location = {303, 26};
	control->stockRight->Size = {32, 14};
	control->stockRight->TextHAlign = HorizontalAlignment::Right;
	control->stockRight->TextVAlign = VerticalAlignment::Centre;
	// Delta (values set in updateValues)
	control->deltaLeft = control->createChild<Label>("", labelFont);
	control->deltaLeft->Location = {50, 26};
	control->deltaLeft->Size = {32, 14};
	control->deltaLeft->TextHAlign = HorizontalAlignment::Right;
	control->deltaLeft->TextVAlign = VerticalAlignment::Centre;
	control->deltaRight = control->createChild<Label>("", labelFont);
	control->deltaRight->Location = {264, 26};
	control->deltaRight->Size = {30, 14};
	control->deltaRight->TextHAlign = HorizontalAlignment::Right;
	control->deltaRight->TextVAlign = VerticalAlignment::Centre;
	// ScrollBar
	control->scrollBar = control->createChild<ScrollBar>();
	control->scrollBar->Location = {102, 24};
	control->scrollBar->Size = {147, 20};
	control->scrollBar->setMinimum(0);
	control->scrollBar->setMaximum(0);
	// ScrollBar buttons
	auto buttonScrollLeft = control->createChild<GraphicButton>(nullptr, scrollLeft);
	buttonScrollLeft->Size = scrollLeft->size;
	buttonScrollLeft->Location = {87, 24};
	buttonScrollLeft->ScrollBarPrev = control->scrollBar;
	auto buttonScrollRight = control->createChild<GraphicButton>(nullptr, scrollRight);
	buttonScrollRight->Size = scrollRight->size;
	buttonScrollRight->Location = {247, 24};
	buttonScrollRight->ScrollBarNext = control->scrollBar;
	// Callback
	control->setupCallbacks();
	// Finally set the values
	control->setScrollbarValues();

	return control;
}

void TransactionControl::setupCallbacks()
{
	std::function<void(FormsEvent * e)> onScrollChange = [this](FormsEvent *) {
		if (!this->suspendUpdates)
		{
			this->updateValues();
		}
	};
	scrollBar->addCallback(FormEventType::ScrollBarChange, onScrollChange);
}

int TransactionControl::getCrewDelta(int index) const
{
	return isPerson ? -tradeState.shipmentsTotal(index) : 0;
}

int TransactionControl::getCargoDelta(int index) const
{
	return !isBio && !isPerson ? -tradeState.shipmentsTotal(index) * storeSpace : 0;
}

int TransactionControl::getBioDelta(int index) const
{
	return isBio ? -tradeState.shipmentsTotal(index) * storeSpace : 0;
}

int TransactionControl::getPriceDelta() const
{
	int delta = 0;
	for (int i = 0; i < 8; i++)
	{
		delta += tradeState.shipmentsTotal(i) * price;
	}
	return delta;
}

void TransactionControl::onRender()
{
	Control::onRender();

	static Vec2<int> bgLeftPos = {0, 2};
	static Vec2<int> bgRightPos = {172, 2};
	static Vec2<int> ammoPos = {4, 2};
	static Vec2<int> iconLeftPos = {58, 24};
	static Vec2<int> iconRightPos = {270, 24};
	static Vec2<int> iconSize = {22, 20};

	// Draw BG
	fw().renderer->draw(bgLeft, bgLeftPos);
	fw().renderer->draw(bgRight, bgRightPos);
	// Draw Ammo Arrow
	if (isAmmo)
	{
		fw().renderer->draw(purchaseArrow, ammoPos);
	}
	// Draw Icons
	if (!deltaLeft->isVisible())
	{
		sp<Image> icon;
		if (isBio)
		{
			icon = tradeState.getLeftIndex() == ECONOMY_IDX ? alienContainedKill
			                                                : alienContainedDetain;
		}
		else
		{
			icon = tradeState.getLeftIndex() == ECONOMY_IDX ? purchaseBoxIcon : purchaseXComIcon;
		}
		auto iconPos = iconLeftPos + (iconSize - (Vec2<int>)icon->size) / 2;
		fw().renderer->draw(icon, iconPos);
	}
	if (!deltaRight->isVisible())
	{
		sp<Image> icon;
		if (isBio)
		{
			icon = tradeState.getRightIndex() == ECONOMY_IDX ? alienContainedKill
			                                                 : alienContainedDetain;
		}
		else
		{
			icon = tradeState.getRightIndex() == ECONOMY_IDX ? purchaseBoxIcon : purchaseXComIcon;
		}
		auto iconPos = iconRightPos + (iconSize - (Vec2<int>)icon->size) / 2;
		fw().renderer->draw(icon, iconPos);
	}
}

void TransactionControl::postRender()
{
	Control::postRender();

	// Draw shade if inactive
	static Vec2<int> shadePos = {0, 0};
	if (tradeState.getLeftIndex() == tradeState.getRightIndex() ||
	    (tradeState.getLeftStock() == 0 && tradeState.getRightStock() == 0))
	{
		fw().renderer->draw(transactionShade, shadePos);
	}
}

void TransactionControl::unloadResources()
{
	bgLeft.reset();
	bgRight.reset();

	purchaseBoxIcon.reset();
	purchaseXComIcon.reset();
	purchaseArrow.reset();

	alienContainedDetain.reset();
	alienContainedKill.reset();

	scrollLeft.reset();
	scrollRight.reset();

	transactionShade.reset();

	Control::unloadResources();
}

/**
 * Get the sum of shipment orders from the base (economy).
 * @param from - 0-7 for bases, 8 for economy
 * @param exclude - 0-7 for bases, 8 for economy, -1 don't exclude (by default)
 * @return - sum of shipment orders
 */
int TransactionControl::Trade::shipmentsFrom(const int from, const int exclude) const
{
	int total = 0;
	if (shipments.find(from) != shipments.end())
	{
		for (auto &s : shipments.at(from))
		{
			if (s.first != exclude && s.second > 0)
			{
				total += s.second;
			}
		}
	}
	return total;
}

/**
 * Get total shipment orders from(+) and to(-) the base (economy).
 * @param baseIdx - 0-7 for bases, 8 for economy
 * @return - total sum of shipment orders
 */
int TransactionControl::Trade::shipmentsTotal(const int baseIdx) const
{
	int total = 0;
	if (shipments.find(baseIdx) != shipments.end())
	{
		for (auto &s : shipments.at(baseIdx))
		{
			total += s.second;
		}
	}
	return total;
}

/**
 * Get shipment order.
 * @param from - 0-7 for bases, 8 for economy
 * @param to - 0-7 for bases, 8 for economy
 * @return - the shipment order
 */
int TransactionControl::Trade::getOrder(const int from, const int to) const
{
	if (shipments.find(from) != shipments.end())
	{
		auto &order = shipments.at(from);
		if (order.find(to) != order.end())
		{
			return order.at(to);
		}
	}
	return 0;
}

/**
 * Cancel shipment order.
 * @param from - 0-7 for bases, 8 for economy
 * @param to - 0-7 for bases, 8 for economy
 */
void TransactionControl::Trade::cancelOrder(const int from, const int to)
{
	if (shipments.find(from) != shipments.end())
	{
		shipments.at(from).erase(to);
		if (shipments.at(from).empty())
			shipments.erase(from);
	}
	if (shipments.find(to) != shipments.end())
	{
		shipments.at(to).erase(from);
		if (shipments.at(to).empty())
			shipments.erase(to);
	}
}

/**
 * Get current stock.
 * @param baseIdx - index of the base (economy)
 * @param oppositeIdx - index of the opposite base (economy)
 * @param currentStock - true for current, false for default (by default)
 * @return - the stock
 */
int TransactionControl::Trade::getStock(const int baseIdx, const int oppositeIdx,
                                        bool currentStock) const
{
	return initialStock[baseIdx] - shipmentsFrom(baseIdx, oppositeIdx) -
	       (currentStock ? getOrder(baseIdx, oppositeIdx) : 0);
}

/**
 * ScrollBar support. Set current value.
 * @param balance - scrollBar->getValue()
 * @return - order from left to right side
 */
int TransactionControl::Trade::setBalance(const int balance)
{
	int orderLR = balance - getRightStock();
	if (orderLR == 0)
	{
		cancelOrder();
	}
	else
	{
		shipments[leftIdx][rightIdx] = orderLR;
		shipments[rightIdx][leftIdx] = -orderLR;
	}
	return orderLR;
}

}; // namespace OpenApoc
