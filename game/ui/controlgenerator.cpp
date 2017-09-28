#include "game/ui/controlgenerator.h"
#include "framework/logger.h"
#include "forms/graphic.h"
#include "forms/label.h"
#include "framework/framework.h"
#include "game/state/city/city.h"
#include "game/state/city/building.h"
#include "framework/font.h"
#include "game/state/city/vehicle.h"
#include "game/state/gamestate.h"
#include "framework/data.h"

namespace OpenApoc
{
ControlGenerator ControlGenerator::singleton;

void ControlGenerator::init(GameState & state)
{
	LogWarning("Implement properly, without requiring an overlaying unitselects that hover over controls, then we will be using stock stuff");
	unitSelect.push_back(fw().data->loadImage(
		"PCK:xcom3/ufodata/vs_icon.pck:xcom3/ufodata/vs_icon.tab:37:xcom3/ufodata/pal_01.dat"));
	unitSelect.push_back(fw().data->loadImage("battle/battle-icon-38.png"));
	unitSelect.push_back(fw().data->loadImage("battle/battle-icon-39.png"));

	auto img = mksp<RGBImage>(Vec2<int>{1, 2});
	{
		RGBImageLock l(img);
		l.set({ 0, 0 }, Colour{ 255, 255, 219 });
		l.set({ 0, 1 }, Colour{ 215, 0, 0 });
	}
	healthImage = img;
	img = mksp<RGBImage>(Vec2<int>{1, 2});
	{
		RGBImageLock l(img);
		l.set({ 0, 0 }, Colour{ 160, 236, 252 });
		l.set({ 0, 1 }, Colour{ 4, 100, 252 });
	}
	shieldImage = img;
	img = mksp<RGBImage>(Vec2<int>{1, 2});
	{
		RGBImageLock l(img);
		l.set({ 0, 0 }, Colour{ 150, 150, 150 });
		l.set({ 0, 1 }, Colour{ 97, 101, 105 });
	}
	stunImage = img;
	iconShade = fw().data->loadImage("battle/battle-icon-shade.png");

	for (int i = 47; i <= 50; i++)
	{
		icons.push_back(fw().data->loadImage(
			format("PCK:xcom3/ufodata/vs_icon.pck:xcom3/ufodata/vs_icon.tab:%s:xcom3/ufodata/pal_01.dat", i)));
	}

	for (int i = 51; i <= 63; i++)
	{
		vehiclePassengerCountIcons.push_back(fw().data->loadImage(
			format("PCK:xcom3/ufodata/vs_icon.pck:xcom3/ufodata/vs_icon.tab:%s:xcom3/ufodata/pal_01.dat", i)));
	}

	initialised = true;
}

VehicleTileInfo ControlGenerator::createVehicleInfo(GameState &state, sp<Vehicle> v)
{
	VehicleTileInfo t;
	t.vehicle = v;
	t.selected = 0;

	for (auto &veh : state.current_city->cityViewSelectedVehicles)
	{
		if (veh == v)
		{
			t.selected =
				(v->name == state.current_city->cityViewSelectedVehicles.front()->name) ? 2 : 1;
			break;
		}
	}

	float maxHealth;
	float currentHealth;
	if (v->getShield() != 0)
	{
		maxHealth = v->getMaxShield();
		currentHealth = v->getShield();
		t.shield = true;
	}
	else
	{
		maxHealth = v->getMaxHealth();
		currentHealth = v->getHealth();
		t.shield = false;
	}

	t.healthProportion = currentHealth / maxHealth;
	// Clamp passengers to 13 as anything beyond that gets the same icon
	t.passengers = std::min(13, v->getPassengers());
	// FIXME Fade out vehicles that are on the way to/back from the alien dimension
	t.faded = false;

	auto b = v->currentBuilding;
	if (b)
	{
		if (b == v->homeBuilding)
		{
			t.state = CityUnitState::InBase;
		}
		else
		{
			t.state = CityUnitState::InBuilding;
		}
	}
	else
	{
		t.state = CityUnitState::InMotion;
	}
	return t;
}

sp<Control> ControlGenerator::createVehicleControl(GameState & state, const VehicleTileInfo &info)
{
	if (!singleton.initialised)
	{
		singleton.init(state);
	}
	LogInfo("Creating city info control for vehicle \"%s\"", info.vehicle->name);

	auto frame = singleton.unitSelect[info.selected];
	auto baseControl = mksp<Graphic>(frame);
	baseControl->Size = frame->size;
	// FIXME: There's an extra 1 pixel here that's annoying
	baseControl->Size.x -= 1;
	baseControl->Name = "OWNED_VEHICLE_FRAME_" + info.vehicle->name;
	baseControl->setData(info.vehicle);

	auto vehicleIcon = baseControl->createChild<Graphic>(info.vehicle->type->icon);
	vehicleIcon->AutoSize = true;
	vehicleIcon->Location = { 1, 1 };
	vehicleIcon->Name = "OWNED_VEHICLE_ICON_" + info.vehicle->name;

	// FIXME: Put these somewhere slightly less magic?
	Vec2<int> healthBarOffset = { 27, 2 };
	Vec2<int> healthBarSize = { 3, 20 };

	auto healthImg = info.shield ? singleton.shieldImage : singleton.healthImage;

	auto healthGraphic = baseControl->createChild<Graphic>(healthImg);
	// This is a bit annoying as the health bar starts at the bottom, but the coord origin is
	// top-left, so fix that up a bit
	int healthBarHeight = (int)((float)healthBarSize.y * info.healthProportion);
	healthBarOffset.y = healthBarOffset.y + (healthBarSize.y - healthBarHeight);
	healthBarSize.y = healthBarHeight;
	healthGraphic->Location = healthBarOffset;
	healthGraphic->Size = healthBarSize;
	healthGraphic->ImagePosition = FillMethod::Stretch;

	sp<Graphic> stateGraphic;

	stateGraphic = baseControl->createChild<Graphic>(singleton.icons[(int)info.state]);
	stateGraphic->AutoSize = true;
	stateGraphic->Location = { 0, 0 };
	stateGraphic->Name = "OWNED_VEHICLE_STATE_" + info.vehicle->name;

	if (info.passengers)
	{
		auto passengerGraphic =
			vehicleIcon->createChild<Graphic>(singleton.vehiclePassengerCountIcons[info.passengers]);
		passengerGraphic->AutoSize = true;
		passengerGraphic->Location = { 0, 0 };
		passengerGraphic->Name = "OWNED_VEHICLE_PASSENGERS_" + info.vehicle->name;
	}

	return baseControl;
}

sp<Control> ControlGenerator::createVehicleControl(GameState & state, sp<Vehicle> v)
{
	auto info = createVehicleInfo(state, v);
	return createVehicleControl(state, info);
}




bool VehicleTileInfo::operator==(const VehicleTileInfo &other) const
{
	return (this->vehicle == other.vehicle && this->selected == other.selected &&
		this->healthProportion == other.healthProportion && this->shield == other.shield &&
		this->passengers == other.passengers && this->state == other.state);
}

}
