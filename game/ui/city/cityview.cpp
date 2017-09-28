#include "game/ui/city/cityview.h"
#include "forms/checkbox.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/list.h"
#include "forms/radiobutton.h"
#include "game/state/city/baselayout.h"
#include "forms/ticker.h"
#include "forms/ui.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "framework/keycodes.h"
#include "framework/palette.h"
#include "framework/renderer.h"
#include "framework/sound.h"
#include "framework/trace.h"
#include "game/state/base/base.h"
#include "game/state/base/facility.h"
#include "game/state/battle/battle.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/citycommonsamplelist.h"
#include "game/state/city/projectile.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/gameevent.h"
#include "game/state/gamestate.h"
#include "game/state/message.h"
#include "game/state/organisation.h"
#include "game/state/research.h"
#include "game/state/rules/aequipment_type.h"
#include "game/state/rules/vammo_type.h"
#include "game/state/rules/vehicle_type.h"
#include "game/state/tileview/collision.h"
#include "game/state/tileview/tileobject_projectile.h"
#include "game/state/tileview/tileobject_scenery.h"
#include "game/state/tileview/tileobject_vehicle.h"
#include "game/state/ufopaedia.h"
#include "game/ui/base/basegraphics.h"
#include "game/ui/base/basescreen.h"
#include "game/ui/city/basebuyscreen.h"
#include "game/ui/city/locationscreen.h"
#include "game/ui/base/researchscreen.h"
#include "game/ui/base/vequipscreen.h"
#include "game/ui/battle/battlebriefing.h"
#include "game/ui/city/alertscreen.h"
#include "game/ui/city/baseselectscreen.h"
#include "game/ui/city/buildingscreen.h"
#include "game/ui/city/infiltrationscreen.h"
#include "game/ui/city/scorescreen.h"
#include "game/ui/general/ingameoptions.h"
#include "game/ui/general/messagebox.h"
#include "game/ui/general/messagelogscreen.h"
#include "game/ui/general/notificationscreen.h"
#include "game/ui/ufopaedia/ufopaediacategoryview.h"
#include "game/ui/ufopaedia/ufopaediaview.h"
#include "game/ui/controlgenerator.h"
#include "library/sp.h"
#include "library/strings_format.h"
#include <glm/glm.hpp>

namespace OpenApoc
{
namespace
{

static const std::vector<UString> TAB_FORM_NAMES = {
    "city/tab1", "city/tab2", "city/tab3", "city/tab4",
    "city/tab5", "city/tab6", "city/tab7", "city/tab8",
};

} // anonymous namespace

void CityView::orderGoToBase()
{
	for (auto &v : this->state->current_city->cityViewSelectedVehicles)
	{
		if (v && v->owner == this->state->getPlayer())
		{
			LogWarning("Goto base for vehicle \"%s\"", v->name);
			auto bld = v->homeBuilding;
			if (!bld)
			{
				LogError("Vehicle \"%s\" has no building", v->name);
			}
			LogWarning("Vehicle \"%s\" goto building \"%s\"", v->name, bld->name);
			// FIXME: Don't clear missions if not replacing current mission
			v->setMission(*this->state, VehicleMission::gotoBuilding(*this->state, *v, bld));
		}
	}
}

void CityView::orderMove(Vec3<float> position, bool useTeleporter)
{
	state->current_city->groupMove(*state, state->current_city->cityViewSelectedVehicles, position,
	                               useTeleporter);
}

void CityView::orderMove(StateRef<Building> building, bool useTeleporter)
{
	for (auto &v : this->state->current_city->cityViewSelectedVehicles)
	{
		if (v && v->owner == this->state->getPlayer())
		{
			LogWarning("Vehicle \"%s\" goto building \"%s\"", v->name, building->name);
			// FIXME: Don't clear missions if not replacing current mission
			v->setMission(*state,
			              VehicleMission::gotoBuilding(*state, *v, building, useTeleporter));
		}
	}
}

void CityView::orderSelect(StateRef<Vehicle> vehicle, bool inverse, bool additive)
{
	auto pos = std::find(state->current_city->cityViewSelectedVehicles.begin(),
	                     state->current_city->cityViewSelectedVehicles.end(), vehicle);
	if (inverse)
	{
		// Vehicle in selection => remove
		if (pos != state->current_city->cityViewSelectedVehicles.end())
		{
			state->current_city->cityViewSelectedVehicles.erase(pos);
		}
	}
	else
	{
		// Vehicle not selected
		if (pos == state->current_city->cityViewSelectedVehicles.end())
		{
			// Selecting non-owned vehicles is always additive to current selection
			if (additive || vehicle->owner != state->getPlayer())
			{
				// Whenever adding clear any non-player vehicles from selection
				if (!state->current_city->cityViewSelectedVehicles.empty() &&
				    state->current_city->cityViewSelectedVehicles.front()->owner !=
				        state->getPlayer())
				{
					state->current_city->cityViewSelectedVehicles.pop_front();
				}
				state->current_city->cityViewSelectedVehicles.push_front(vehicle);
			}
			else
			{
				// Vehicle not in selection => replace selection with vehicle
				state->current_city->cityViewSelectedVehicles.clear();
				state->current_city->cityViewSelectedVehicles.push_back(vehicle);
			}
		}
		// Vehicle is selected
		else
		{
			// First move vehicle to front
			state->current_city->cityViewSelectedVehicles.erase(pos);
			// If moving vehicle to front, deselect any non-owned vehicle, unless it's that one
			if (!state->current_city->cityViewSelectedVehicles.empty() &&
			    state->current_city->cityViewSelectedVehicles.front()->owner != state->getPlayer())
			{
				state->current_city->cityViewSelectedVehicles.pop_front();
			}
			state->current_city->cityViewSelectedVehicles.push_front(vehicle);
			// Then if not additive then zoom to vehicle
			if (!additive)
			{
				this->setScreenCenterTile(vehicle->position);
			}
		}
	}
}

void CityView::orderAttack(StateRef<Vehicle> vehicle)
{
	for (auto &v : this->state->current_city->cityViewSelectedVehicles)
	{
		if (v && v->owner == this->state->getPlayer() && v != vehicle)
		{
			// FIXME: Don't clear missions if not replacing current mission
			v->setMission(*state, VehicleMission::attackVehicle(*this->state, *v, vehicle));
		}
	}
}

void CityView::orderAttack(StateRef<Building> building)
{
	for (auto &v : this->state->current_city->cityViewSelectedVehicles)
	{
		if (v && v->owner == this->state->getPlayer())
		{
			// TODO: Attack building mission
			LogWarning("IMPLEMENT: Vehicle \"%s\" attack building \"%s\"", v->name, building->name);
		}
	}
}

CityView::CityView(sp<GameState> state)
    : CityTileView(*state->current_city->map, Vec3<int>{TILE_X_CITY, TILE_Y_CITY, TILE_Z_CITY},
                   Vec2<int>{STRAT_TILE_X, STRAT_TILE_Y}, TileViewMode::Isometric,
                   state->current_city->cityViewScreenCenter, *state),
      baseForm(ui().getForm("city/city")), overlayTab(ui().getForm("city/overlay")),
      updateSpeed(UpdateSpeed::Speed1), lastSpeed(UpdateSpeed::Pause), state(state),
      followVehicle(false), selectionState(SelectionState::Normal),
      day_palette(fw().data->loadPalette("xcom3/ufodata/pal_01.dat")),
      twilight_palette(fw().data->loadPalette("xcom3/ufodata/pal_02.dat")),
      night_palette(fw().data->loadPalette("xcom3/ufodata/pal_03.dat"))
{
	std::vector<sp<Palette>> newPal;
	newPal.resize(3);
	for (int j = 0; j <= 15; j++)
	{
		colorCurrent = j;
		newPal[0] = mksp<Palette>();
		newPal[1] = mksp<Palette>();
		newPal[2] = mksp<Palette>();

		for (int i = 0; i < 255 - 4; i++)
		{
			newPal[0]->setColour(i, day_palette->getColour(i));
			newPal[1]->setColour(i, twilight_palette->getColour(i));
			newPal[2]->setColour(i, night_palette->getColour(i));
		}
		for (int i = 0; i < 3; i++)
		{
			// Yellow color, for owned indicators, pulsates from (3/8r 3/8g 0b) to (8/8r 8/8g 0b)
			newPal[i]->setColour(255 - 3, Colour((colorCurrent * 16 * 5 + 255 * 3) / 8,
			                                     (colorCurrent * 16 * 5 + 255 * 3) / 8, 0));
			// Red color, for enemy indicators, pulsates from (3/8r 0g 0b) to (8/8r 0g 0b)
			newPal[i]->setColour(255 - 2, Colour((colorCurrent * 16 * 5 + 255 * 3) / 8, 0, 0));
			// Pink color, for neutral indicators, pulsates from (3/8r 0g 3/8b) to (8/8r 0g 8/8b)
			newPal[i]->setColour(255 - 1, Colour((colorCurrent * 16 * 5 + 255 * 3) / 8, 0,
			                                     (colorCurrent * 16 * 5 + 255 * 3) / 8));
			// Blue color, for misc. indicators, pulsates from (0r 3/8g 3/8b) to (0r 8/8g 8/8b)
			newPal[i]->setColour(255 - 0, Colour(0, (colorCurrent * 16 * 5 + 255 * 3) / 8,
			                                     (colorCurrent * 16 * 5 + 255 * 3) / 8));
		}

		mod_day_palette.push_back(newPal[0]);
		mod_twilight_palette.push_back(newPal[1]);
		mod_night_palette.push_back(newPal[2]);
	}

	overlayTab->setVisible(false);
	overlayTab->findControlTyped<GraphicButton>("BUTTON_CLOSE")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *) { setSelectionState(SelectionState::Normal); });
	baseForm->findControlTyped<RadioButton>("BUTTON_SPEED1")->setChecked(true);
	for (auto &formName : TAB_FORM_NAMES)
	{
		sp<Form> f(ui().getForm(formName));
		if (!f)
		{
			LogError("Failed to load form \"%s\"", formName);
			return;
		}
		f->takesFocus = false;
		this->uiTabs.push_back(f);
	}
	this->activeTab = this->uiTabs[this->state->current_city->cityViewPageIndex];

	// Refresh base views
	resume();

	if (state->newGame)
	{
		auto bld = state->current_base->building;
		if (!bld)
		{
			LogError("Base with invalid bld");
		}
		auto bldBounds = bld->bounds;

		Vec2<int> buildingCenter = (bldBounds.p0 + bldBounds.p1) / 2;
		this->setScreenCenterTile(buildingCenter);
	}

	this->baseForm->findControl("BUTTON_TAB_1")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->state->current_city->cityViewPageIndex = 0;
		    this->activeTab = this->uiTabs[this->state->current_city->cityViewPageIndex];
		});
	this->baseForm->findControl("BUTTON_TAB_2")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->state->current_city->cityViewPageIndex = 1;
		    this->activeTab = this->uiTabs[this->state->current_city->cityViewPageIndex];
		});
	this->baseForm->findControl("BUTTON_TAB_3")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->state->current_city->cityViewPageIndex = 2;
		    this->activeTab = this->uiTabs[this->state->current_city->cityViewPageIndex];
		});
	this->baseForm->findControl("BUTTON_TAB_4")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->state->current_city->cityViewPageIndex = 3;
		    this->activeTab = this->uiTabs[this->state->current_city->cityViewPageIndex];
		});
	this->baseForm->findControl("BUTTON_TAB_5")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->state->current_city->cityViewPageIndex = 4;
		    this->activeTab = this->uiTabs[this->state->current_city->cityViewPageIndex];
		});
	this->baseForm->findControl("BUTTON_TAB_6")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->state->current_city->cityViewPageIndex = 5;
		    this->activeTab = this->uiTabs[this->state->current_city->cityViewPageIndex];
		});
	this->baseForm->findControl("BUTTON_TAB_7")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->state->current_city->cityViewPageIndex = 6;
		    this->activeTab = this->uiTabs[this->state->current_city->cityViewPageIndex];
		});
	this->baseForm->findControl("BUTTON_TAB_8")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->state->current_city->cityViewPageIndex = 7;
		    this->activeTab = this->uiTabs[this->state->current_city->cityViewPageIndex];
		});
	this->baseForm->findControl("BUTTON_FOLLOW_VEHICLE")
	    ->addCallback(FormEventType::CheckBoxChange, [this](FormsEvent *e) {
		    this->followVehicle =
		        std::dynamic_pointer_cast<CheckBox>(e->forms().RaisedBy)->isChecked();
		});
	this->baseForm->findControl("BUTTON_TOGGLE_STRATMAP")
	    ->addCallback(FormEventType::CheckBoxChange, [this](FormsEvent *e) {
		    bool strategy = std::dynamic_pointer_cast<CheckBox>(e->forms().RaisedBy)->isChecked();
		    this->setViewMode(strategy ? TileViewMode::Strategy : TileViewMode::Isometric);
		});
	this->baseForm->findControl("BUTTON_SPEED0")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->updateSpeed = UpdateSpeed::Pause; });
	this->baseForm->findControl("BUTTON_SPEED1")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->updateSpeed = UpdateSpeed::Speed1; });
	this->baseForm->findControl("BUTTON_SPEED2")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->updateSpeed = UpdateSpeed::Speed2; });
	this->baseForm->findControl("BUTTON_SPEED3")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->updateSpeed = UpdateSpeed::Speed3; });
	this->baseForm->findControl("BUTTON_SPEED4")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->updateSpeed = UpdateSpeed::Speed4; });
	this->baseForm->findControl("BUTTON_SPEED5")
	    ->addCallback(FormEventType::CheckBoxSelected,
	                  [this](Event *) { this->updateSpeed = UpdateSpeed::Speed5; });
	this->baseForm->findControl("BUTTON_SHOW_ALIEN_INFILTRATION")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    fw().stageQueueCommand(
		        {StageCmd::Command::PUSH, mksp<InfiltrationScreen>(this->state)});
		});
	this->baseForm->findControl("BUTTON_SHOW_SCORE")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<ScoreScreen>(this->state)});
		});
	this->baseForm->findControl("BUTTON_SHOW_UFOPAEDIA")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<UfopaediaView>(this->state)});
		});
	this->baseForm->findControl("BUTTON_SHOW_OPTIONS")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<InGameOptions>(this->state)});
		});
	this->baseForm->findControl("BUTTON_SHOW_LOG")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    fw().stageQueueCommand(
		        {StageCmd::Command::PUSH, mksp<MessageLogScreen>(this->state, *this)});
		});
	this->baseForm->findControl("BUTTON_ZOOM_EVENT")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    if (baseForm->findControlTyped<Ticker>("NEWS_TICKER")->hasMessages())
		    {
			    this->zoomLastEvent();
		    }
		});

	auto baseManagementForm = this->uiTabs[0];
	baseManagementForm->findControl("BUTTON_SHOW_BASE")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<BaseScreen>(this->state)});
		});
	baseManagementForm->findControl("BUTTON_BUILD_BASE")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    fw().stageQueueCommand(
		        {StageCmd::Command::PUSH, mksp<BaseSelectScreen>(this->state, this->centerPos)});
		});
	auto vehicleForm = this->uiTabs[1];
	vehicleForm->findControl("BUTTON_EQUIP_VEHICLE")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    auto equipScreen = mksp<VEquipScreen>(this->state);
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
				    equipScreen->setSelectedVehicle(v);
				    fw().stageQueueCommand({StageCmd::Command::PUSH, equipScreen});
				    break;
			    }
		    }
		});
	vehicleForm->findControl("BUTTON_VEHICLE_BUILDING")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
					fw().stageQueueCommand(
					    {StageCmd::Command::PUSH, mksp<LocationScreen>(this->state, v)});
					break;
			    }
		    }
		});
	vehicleForm->findControl("BUTTON_GOTO_BUILDING")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
				    setSelectionState(SelectionState::VehicleGotoBuilding);
				    break;
			    }
		    }
		});
	vehicleForm->findControl("BUTTON_GOTO_LOCATION")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
				    setSelectionState(SelectionState::VehicleGotoLocation);
				    break;
			    }
		    }
		});
	vehicleForm->findControl("BUTTON_VEHICLE_ATTACK")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
				    setSelectionState(SelectionState::VehicleAttackVehicle);
				    break;
			    }
		    }
		});
	vehicleForm->findControl("BUTTON_VEHICLE_ATTACK_BUILDING")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
				    setSelectionState(SelectionState::VehicleGotoLocation);
				    break;
			    }
		    }
		});
	vehicleForm->findControl("BUTTON_GOTO_BASE")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) { orderGoToBase(); });

	vehicleForm->findControl("BUTTON_ATTACK_MODE_AGGRESSIVE")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
				    v->attackMode = Vehicle::AttackMode::Aggressive;
			    }
		    }
		});
	vehicleForm->findControl("BUTTON_ATTACK_MODE_STANDARD")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
				    v->attackMode = Vehicle::AttackMode::Standard;
			    }
		    }
		});
	vehicleForm->findControl("BUTTON_ATTACK_MODE_DEFENSIVE")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
				    v->attackMode = Vehicle::AttackMode::Defensive;
			    }
		    }
		});
	vehicleForm->findControl("BUTTON_ATTACK_MODE_EVASIVE")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
				    v->attackMode = Vehicle::AttackMode::Evasive;
			    }
		    }
		});
	vehicleForm->findControl("BUTTON_ALTITUDE_HIGHEST")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
				    v->altitude = Vehicle::Altitude::Highest;
			    }
		    }
		});
	vehicleForm->findControl("BUTTON_ALTITUDE_HIGH")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
				    v->altitude = Vehicle::Altitude::High;
			    }
		    }
		});
	vehicleForm->findControl("BUTTON_ALTITUDE_STANDARD")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
				    v->altitude = Vehicle::Altitude::Standard;
			    }
		    }
		});
	vehicleForm->findControl("BUTTON_ALTITUDE_LOW")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		    {
			    if (v && v->owner == this->state->getPlayer())
			    {
				    v->altitude = Vehicle::Altitude::Low;
			    }
		    }
		});
	auto agentForm = this->uiTabs[2];
	agentForm->findControl("BUTTON_AGENT_BUILDING")
		->addCallback(FormEventType::ButtonClick, [this](Event *) {
		for (auto &v : this->state->current_city->cityViewSelectedVehicles)
		{
			if (v && v->owner == this->state->getPlayer())
			{
				// FIXME: Open agent's location screen here
				LogWarning("Implement agent location scren opening");
				fw().stageQueueCommand(
				{ StageCmd::Command::PUSH, mksp<LocationScreen>(this->state, v) });
				break;
			}
		}
	});
}

CityView::~CityView() = default;

void CityView::begin()
{
	if (state->newGame)
	{
		state->newGame = false;
		baseForm->findControlTyped<Ticker>("NEWS_TICKER")
		    ->addMessage(tr("Welcome to X-COM Apocalypse"));
	}
}

void CityView::resume()
{
	modifierLAlt = false;
	modifierLCtrl = false;
	modifierLShift = false;
	modifierRAlt = false;
	modifierRCtrl = false;
	modifierRShift = false;

	this->uiTabs[0]->findControlTyped<Label>("TEXT_BASE_NAME")->setText(state->current_base->name);
	miniViews.clear();
	int b = 0;
	for (auto &pair : state->player_bases)
	{
		auto &viewBase = pair.second;
		auto viewName = format("BUTTON_BASE_%d", ++b);
		auto view = this->uiTabs[0]->findControlTyped<GraphicButton>(viewName);
		if (!view)
		{
			LogError("Failed to find UI control matching \"%s\"", viewName);
		}
		view->setData(viewBase);
		auto viewImage = BaseGraphics::drawMiniBase(viewBase);
		view->setImage(viewImage);
		view->setDepressedImage(viewImage);
		view->addCallback(FormEventType::ButtonClick, [this](FormsEvent *e) {
			this->state->current_base = {this->state.get(), e->forms().RaisedBy->getData<Base>()};
			this->uiTabs[0]
			    ->findControlTyped<Label>("TEXT_BASE_NAME")
			    ->setText(this->state->current_base->name);
		});
		miniViews.push_back(view);
	}
}

void CityView::render()
{
	TRACE_FN;

	if (!this->surface)
	{
		this->drawCity = true;
		this->surface = mksp<Surface>(fw().displayGetSize());
	}

	if (drawCity)
	{
		this->drawCity = false;
		RendererSurfaceBinding b(*fw().renderer, this->surface);

		CityTileView::render();
		if (state->showVehiclePath)
		{
			for (auto &pair : state->vehicles)
			{
				auto v = pair.second;
				if (v->city != state->current_city)
					continue;
				auto vTile = v->tileObject;
				if (!vTile)
					continue;
				auto &path = v->missions.front()->currentPlannedPath;
				Vec3<float> prevPos = vTile->getPosition();
				for (auto &pos : path)
				{
					Vec2<float> screenPosA = this->tileToOffsetScreenCoords(prevPos);
					Vec2<float> screenPosB = this->tileToOffsetScreenCoords(pos);

					fw().renderer->drawLine(screenPosA, screenPosB, Colour{255, 0, 0, 128});

					prevPos = pos;
				}
			}
		}
		activeTab->render();
		baseForm->render();
		overlayTab->render();
		if (activeTab == uiTabs[0])
		{
			// Highlight selected base
			for (auto &view : miniViews)
			{
				auto viewBase = view->getData<Base>();
				if (state->current_base == viewBase)
				{
					Vec2<int> pos = uiTabs[0]->Location + view->Location - 1;
					Vec2<int> size = view->Size + 2;
					fw().renderer->drawRect(pos, size, Colour{255, 0, 0});
					break;
				}
			}
		}
	}

	// If there's a modal dialog, darken the screen
	if (fw().stageGetCurrent() != this->shared_from_this())
	{
		fw().renderer->drawTinted(this->surface, {0, 0}, {128, 128, 128, 255});
	}
	else
	{
		fw().renderer->draw(this->surface, {0, 0});
	}
}

void CityView::update()
{
	this->drawCity = true;
	CityTileView::update();

	unsigned int ticks = 0;
	bool turbo = false;
	switch (this->updateSpeed)
	{
		case UpdateSpeed::Pause:
			ticks = 0;
			break;
		/* POSSIBLE FIXME: 'vanilla' apoc appears to implement Speed1 as 1/2 speed - that is
		    * only
		    * every other call calls the update loop, meaning that the later update tick counts are
		    * halved as well.
		    * This effectively means that all openapoc tick counts count for 1/2 the value of
		    * vanilla
		    * apoc ticks */
		case UpdateSpeed::Speed1:
			ticks = 1;
			break;
		case UpdateSpeed::Speed2:
			ticks = 2;
			break;
		case UpdateSpeed::Speed3:
			ticks = 4;
			break;
		case UpdateSpeed::Speed4:
			ticks = 6;
			break;
		case UpdateSpeed::Speed5:
			if (!this->state->canTurbo())
			{
				setUpdateSpeed(UpdateSpeed::Speed1);
				ticks = 1;
			}
			else
			{
				turbo = true;
			}
			break;
	}
	baseForm->findControl("BUTTON_SPEED5")->Enabled = this->state->canTurbo();

	if (turbo)
	{
		this->state->updateTurbo();
		if (!this->state->canTurbo())
		{
			setUpdateSpeed(UpdateSpeed::Speed1);
		}
	}
	else
	{
		while (ticks > 0)
		{
			int ticksPerUpdate = UPDATE_EVERY_TICK ? 1 : ticks;
			state->update(ticksPerUpdate);
			ticks -= ticksPerUpdate;
		}
	}

	updateSelectedUnits();

	// Update time display
	auto clockControl = baseForm->findControlTyped<Label>("CLOCK");
	clockControl->setText(state->gameTime.getLongTimeString());

	// Pulsate palette colors
	colorCurrent += (colorForward ? 1 : -1);
	if (colorCurrent <= 0 || colorCurrent >= 15)
	{
		colorCurrent = clamp(colorCurrent, 0, 15);
		colorForward = !colorForward;
	}

	// The palette fades from pal_03 at 3am to pal_02 at 6am then pal_01 at 9am
	// The reverse for 3pm, 6pm & 9pm

	auto hour = state->gameTime.getHours();
	sp<Palette> interpolated_palette;
	if (hour < 3 || hour >= 21)
	{
		interpolated_palette = this->mod_night_palette[colorCurrent];
	}
	else if (hour >= 9 && hour < 15)
	{
		interpolated_palette = this->mod_day_palette[colorCurrent];
	}
	else
	{
		sp<Palette> palette1;
		sp<Palette> palette2;
		float factor = 0;

		float hours_float = hour + (float)state->gameTime.getMinutes() / 60.0f;

		if (hour >= 3 && hour < 6)
		{
			palette1 = this->mod_night_palette[colorCurrent];
			palette2 = this->mod_twilight_palette[colorCurrent];
			factor = clamp((hours_float - 3.0f) / 3.0f, 0.0f, 1.0f);
		}
		else if (hour >= 6 && hour < 9)
		{
			palette1 = this->mod_twilight_palette[colorCurrent];
			palette2 = this->mod_day_palette[colorCurrent];
			factor = clamp((hours_float - 6.0f) / 3.0f, 0.0f, 1.0f);
		}
		else if (hour >= 15 && hour < 18)
		{
			palette1 = this->mod_day_palette[colorCurrent];
			palette2 = this->mod_twilight_palette[colorCurrent];
			factor = clamp((hours_float - 15.0f) / 3.0f, 0.0f, 1.0f);
		}
		else if (hour >= 18 && hour < 21)
		{
			palette1 = this->mod_twilight_palette[colorCurrent];
			palette2 = this->mod_night_palette[colorCurrent];
			factor = clamp((hours_float - 18.0f) / 3.0f, 0.0f, 1.0f);
		}
		else
		{
			LogError("Unhandled hoursClamped %d", hour);
		}

		interpolated_palette = mksp<Palette>();
		for (int i = 0; i < 256; i++)
		{
			auto &colour1 = palette1->getColour(i);
			auto &colour2 = palette2->getColour(i);
			Colour interpolated_colour;

			interpolated_colour.r = (int)mix((float)colour1.r, (float)colour2.r, factor);
			interpolated_colour.g = (int)mix((float)colour1.g, (float)colour2.g, factor);
			interpolated_colour.b = (int)mix((float)colour1.b, (float)colour2.b, factor);
			interpolated_colour.a = (int)mix((float)colour1.a, (float)colour2.a, factor);
			interpolated_palette->setColour(i, interpolated_colour);
		}
	}

	this->pal = interpolated_palette;

	// FIXME: Possibly more efficient ways than re-generating all controls every frame?

	// Setup owned vehicle list controls
	auto ownedVehicleList = uiTabs[1]->findControlTyped<ListBox>("OWNED_VEHICLE_LIST");
	if (!ownedVehicleList)
	{
		LogError("Failed to find \"OWNED_VEHICLE_LIST\" control on city tab \"%s\"",
		         TAB_FORM_NAMES[1]);
	}

	ownedVehicleList->ItemSpacing = 0;

	std::map<sp<Vehicle>, std::pair<VehicleTileInfo, sp<Control>>> newVehicleListControls;

	ownedVehicleList->clear();

	if (activeTab == uiTabs[1])
	{
		for (auto &v : state->vehicles)
		{
			auto vehicle = v.second;
			if (vehicle->owner != state->getPlayer())
			{
				continue;
			}
			auto info = ControlGenerator::createVehicleInfo(*state, vehicle);
			auto it = this->vehicleListControls.find(vehicle);
			sp<Control> control;
			if (it != this->vehicleListControls.end() && it->second.first == info)
			{
				// Control unchanged, don't regenerate
				control = it->second.second;
			}
			else
			{
				control = ControlGenerator::createVehicleControl(*state, info);
				control->addCallback(FormEventType::MouseDown, [this, vehicle](FormsEvent *e) {
					orderSelect(
					    StateRef<Vehicle>{state.get(), Vehicle::getId(*state, vehicle)},
					    Event::isPressed(e->forms().MouseInfo.Button, Event::MouseButton::Right),
					    modifierLCtrl || modifierRCtrl);
					auto vehicleForm = this->uiTabs[1];

					switch (vehicle->altitude)
					{
						case Vehicle::Altitude::Highest:
							vehicleForm->findControlTyped<RadioButton>("BUTTON_ALTITUDE_HIGHEST")
							    ->setChecked(true);
							break;
						case Vehicle::Altitude::High:
							vehicleForm->findControlTyped<RadioButton>("BUTTON_ALTITUDE_HIGH")
							    ->setChecked(true);
							break;
						case Vehicle::Altitude::Standard:
							vehicleForm->findControlTyped<RadioButton>("BUTTON_ALTITUDE_STANDARD")
							    ->setChecked(true);
							break;
						case Vehicle::Altitude::Low:
							vehicleForm->findControlTyped<RadioButton>("BUTTON_ALTITUDE_LOW")
							    ->setChecked(true);
							break;
					}

					switch (vehicle->attackMode)
					{
						case Vehicle::AttackMode::Aggressive:
							vehicleForm
							    ->findControlTyped<RadioButton>("BUTTON_ATTACK_MODE_AGGRESSIVE")
							    ->setChecked(true);
							break;
						case Vehicle::AttackMode::Standard:
							vehicleForm
							    ->findControlTyped<RadioButton>("BUTTON_ATTACK_MODE_STANDARD")
							    ->setChecked(true);
							break;
						case Vehicle::AttackMode::Defensive:
							vehicleForm
							    ->findControlTyped<RadioButton>("BUTTON_ATTACK_MODE_DEFENSIVE")
							    ->setChecked(true);
							break;
						case Vehicle::AttackMode::Evasive:
							vehicleForm->findControlTyped<RadioButton>("BUTTON_ATTACK_MODE_EVASIVE")
							    ->setChecked(true);
							break;
					}
				});
			}
			newVehicleListControls[vehicle] = std::make_pair(info, control);
			ownedVehicleList->addItem(control);
		}
	}

	// Clear the old list and reset to the new one (May be empty is not on a vehicle-displaying tab
	this->vehicleListControls.clear();
	this->vehicleListControls = std::move(newVehicleListControls);

	activeTab->update();
	baseForm->update();
	overlayTab->update();

	// If we have 'follow vehicle' enabled we clobber any other movement that may have occurred in
	// this frame
	if (this->followVehicle && !state->current_city->cityViewSelectedVehicles.empty())
	{
		auto v = state->current_city->cityViewSelectedVehicles.front();
		// The selected vehicle may not have a tile object if it's not on the map
		if (v->tileObject)
		{
			this->setScreenCenterTile(v->tileObject->getCenter());
		}
	}
	// Store screen center for serialisation
	state->current_city->cityViewScreenCenter = centerPos;
}

std::shared_future<void> loadBattleBase(sp<GameState> state, StateRef<Base> base,
                                        StateRef<Organisation> attacker)
{
	auto loadTask = fw().threadPoolEnqueue([base, state, attacker]() -> void {

		std::list<StateRef<Agent>> agents;
		StateRef<Vehicle> veh = {};

		Battle::beginBattle(*state, false, attacker, agents, nullptr, nullptr, nullptr, veh,
		                    base->building);
	});

	return loadTask;
}

void CityView::initiateDefenseMission(StateRef<Base> base, StateRef<Organisation> attacker)
{
	fw().stageQueueCommand({StageCmd::Command::REPLACEALL,
	                        mksp<BattleBriefing>(state, attacker, base->building.id, false, false,
	                                             loadBattleBase(state, base, attacker))});
}

void CityView::eventOccurred(Event *e)
{
	this->drawCity = true;
	activeTab->eventOccured(e);
	baseForm->eventOccured(e);
	overlayTab->eventOccured(e);
	// Exclude mouse down events that are over the form
	if (activeTab->eventIsWithin(e) || baseForm->eventIsWithin(e) || overlayTab->eventIsWithin(e))
	{
		return;
	}

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (handleKeyDown(e))
		{
			return;
		}
	}
	if (e->type() == EVENT_KEY_UP)
	{
		if (handleKeyUp(e))
		{
			return;
		}
	}
	if (e->type() == EVENT_MOUSE_DOWN)
	{
		if (handleMouseDown(e))
		{
			return;
		}
	}
	if (e->type() == EVENT_GAME_STATE)
	{
		if (handleGameStateEvent(e))
		{
			return;
		}
	}
	CityTileView::eventOccurred(e);
}

bool CityView::handleKeyDown(Event *e)
{
	// Common keys active in both debug and normal mode
	switch (e->keyboard().KeyCode)
	{
		case SDLK_RSHIFT:
			modifierRShift = true;
			return true;
		case SDLK_LSHIFT:
			modifierLShift = true;
			return true;
		case SDLK_RALT:
			modifierRAlt = true;
			return true;
		case SDLK_LALT:
			modifierLAlt = true;
			return true;
		case SDLK_RCTRL:
			modifierRCtrl = true;
			return true;
		case SDLK_LCTRL:
			modifierLCtrl = true;
			return true;
	}
	// Cheat codes
	if (e->type() == EVENT_KEY_DOWN)
	{
		switch (e->keyboard().KeyCode)
		{
			case SDLK_r:
			{
				LogInfo("Repairing...");
				std::set<sp<Scenery>> stuffToRepair;
				for (auto &s : state->current_city->scenery)
				{
					if (s->canRepair())
					{
						stuffToRepair.insert(s);
					}
				}
				LogInfo("Repairing %u tiles out of %u", static_cast<unsigned>(stuffToRepair.size()),
				        static_cast<unsigned>(state->current_city->scenery.size()));

				for (auto &s : stuffToRepair)
				{
					s->repair(*state);
				}
				return true;
			}
		}
	}
	// Keyboard commands
	if (e->type() == EVENT_KEY_DOWN)
	{
		switch (e->keyboard().KeyCode)
		{
			case SDLK_ESCAPE:
				if (selectionState == SelectionState::Normal)
				{
					fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<InGameOptions>(state)});
				}
				else
				{
					setSelectionState(SelectionState::Normal);
				}
				return true;
			case SDLK_TAB:
				this->baseForm->findControlTyped<CheckBox>("BUTTON_TOGGLE_STRATMAP")
				    ->setChecked(
				        !this->baseForm->findControlTyped<CheckBox>("BUTTON_TOGGLE_STRATMAP")
				             ->isChecked());
				return true;
			case SDLK_SPACE:
				if (this->updateSpeed != UpdateSpeed::Pause)
					setUpdateSpeed(UpdateSpeed::Pause);
				else
					setUpdateSpeed(this->lastSpeed);
				return true;
		}
	}
	return false;
}

bool CityView::handleKeyUp(Event *e)
{
	switch (e->keyboard().KeyCode)
	{
		case SDLK_RSHIFT:
			modifierRShift = false;
			return true;
		case SDLK_LSHIFT:
			modifierLShift = false;
			return true;
		case SDLK_RALT:
			modifierRAlt = false;
			return true;
		case SDLK_LALT:
			modifierLAlt = false;
			return true;
		case SDLK_RCTRL:
			modifierRCtrl = false;
			return true;
		case SDLK_LCTRL:
			modifierLCtrl = false;
			return true;
	}
	return false;
}

bool CityView::handleMouseDown(Event *e)
{
	if (this->getViewMode() == TileViewMode::Strategy &&
	    Event::isPressed(e->mouse().Button, Event::MouseButton::Middle))
	{
		Vec2<float> screenOffset = {this->getScreenOffset().x, this->getScreenOffset().y};
		auto clickTile =
		    this->screenToTileCoords(Vec2<float>{e->mouse().X, e->mouse().Y} - screenOffset, 0.0f);
		this->setScreenCenterTile(Vec2<float>{clickTile.x, clickTile.y});
		return true;
	}
	if (Event::isPressed(e->mouse().Button, Event::MouseButton::Left) ||
	    Event::isPressed(e->mouse().Button, Event::MouseButton::Right))
	{
		auto buttonPressed = Event::isPressed(e->mouse().Button, Event::MouseButton::Left)
		                         ? Event::MouseButton::Left
		                         : Event::MouseButton::Right;

		// If a click has not been handled by a form it's in the map. See if we intersect with
		// anything
		Vec2<float> screenOffset = {this->getScreenOffset().x, this->getScreenOffset().y};
		auto clickTop = this->screenToTileCoords(
		    Vec2<float>{e->mouse().X, e->mouse().Y} - screenOffset, 12.99f);
		auto clickBottom =
		    this->screenToTileCoords(Vec2<float>{e->mouse().X, e->mouse().Y} - screenOffset, 0.0f);
		auto collision =
		    state->current_city->map->findCollision(clickTop, clickBottom, {}, nullptr, true);
		if (collision)
		{
			auto position = collision.position;
			sp<Scenery> scenery;
			StateRef<Building> building;
			sp<Vehicle> vehicle;
			sp<Projectile> projectile;
			switch (collision.obj->getType())
			{
				case TileObject::Type::Scenery:
				{
					scenery =
					    std::dynamic_pointer_cast<TileObjectScenery>(collision.obj)->getOwner();
					building = scenery->building;
					LogInfo("CLICKED SCENERY %s at %s BUILDING %s", scenery->type.id,
					        scenery->currentPosition, building.id);
					break;
				}
				case TileObject::Type::Vehicle:
				{
					vehicle =
					    std::dynamic_pointer_cast<TileObjectVehicle>(collision.obj)->getVehicle();
					LogInfo("CLICKED VEHICLE %s at %s", vehicle->name, vehicle->position);
					break;
				}
				case TileObject::Type::Projectile:
				{
					projectile = std::dynamic_pointer_cast<TileObjectProjectile>(collision.obj)
					                 ->getProjectile();
					LogInfo("CLICKED PROJECTILE %s at %s", projectile->damage,
					        projectile->position);
					break;
				}
				default:
				{
					LogError("Clicked on some object we didn't care to process?");
					break;
				}
			}
			// Alt opens info screens
			if (modifierLAlt || modifierRAlt)
			{
				StateRef<UfopaediaEntry> ufopaediaEntry;
				// Left =  Object's ufopaedia info
				if (buttonPressed == Event::MouseButton::Left)
				{
					if (vehicle)
					{
						ufopaediaEntry = vehicle->type->ufopaedia_entry;
					}
					if (building)
					{
						ufopaediaEntry = building->function->ufopaedia_entry;
					}
				}
				// Right = Object owner's ufopaedia info
				else
				{
					StateRef<Organisation> owner;
					if (vehicle)
					{
						owner = vehicle->owner;
					}
					if (building)
					{
						owner = building->owner;
					}
					if (projectile && projectile->firerVehicle)
					{
						owner = projectile->firerVehicle->owner;
					}
					if (owner)
					{
						ufopaediaEntry = owner->ufopaedia_entry;
					}
				}
				// Open ufopaedia entry
				if (ufopaediaEntry && ufopaediaEntry->dependency.satisfied())
				{
					sp<UfopaediaCategory> ufopaedia_category;
					for (auto &cat : this->state->ufopaedia)
					{
						for (auto &entry : cat.second->entries)
						{
							if (ufopaediaEntry == entry.second)
							{
								ufopaedia_category = cat.second;
								break;
							}
						}
						if (ufopaedia_category)
							break;
					}
					if (!ufopaedia_category)
					{
						LogError("No UFOPaedia category found for entry %s", ufopaediaEntry->title);
					}
					fw().stageQueueCommand(
					    {StageCmd::Command::PUSH,
					     mksp<UfopaediaCategoryView>(state, ufopaedia_category, ufopaediaEntry)});
				}
				return true;
			}
			// If shift then we show object's info screen
			if (modifierLShift || modifierRShift)
			{
				// Left = Normal (Equip/Manage)
				if (buttonPressed == Event::MouseButton::Left)
				{
					if (vehicle && vehicle->owner == state->player)
					{
						// Equipscreen for owner vehicles
						auto equipScreen = mksp<VEquipScreen>(this->state);
						equipScreen->setSelectedVehicle(vehicle);
						fw().stageQueueCommand({StageCmd::Command::PUSH, equipScreen});
					}
					if (building)
					{
						if (building->base)
						{
							// Base screen
							state->current_base = building->base;
							this->uiTabs[0]
								->findControlTyped<Label>("TEXT_BASE_NAME")
								->setText(this->state->current_base->name);
							fw().stageQueueCommand(
							{ StageCmd::Command::PUSH, mksp<BaseScreen>(this->state) });
						}
						else if (building->base_layout && building->owner.id == "ORG_GOVERNMENT")
						{
							fw().stageQueueCommand(
							{ StageCmd::Command::PUSH, mksp<BaseBuyScreen>(state, building) });
						}
						else
						{
							// Building screen
							fw().stageQueueCommand(
							{ StageCmd::Command::PUSH, mksp<BuildingScreen>(this->state, building) });
						}
					}
				}
				// Right = Alternative (Location)
				else
				{
					if (vehicle && vehicle->owner == state->player)
					{
						// Location screen
						fw().stageQueueCommand(
						{StageCmd::Command::PUSH,  mksp<LocationScreen>(this->state, vehicle) });
					}
					if (building)
					{
						// Building screen
						fw().stageQueueCommand(
						{ StageCmd::Command::PUSH, mksp<BuildingScreen>(this->state, building) });
					}
				}
				return true;
			}
			// Otherwise proceed as normal
			switch (selectionState)
			{
				case SelectionState::Normal:
				{
					if (building)
					{
						// Building screen for any building
						fw().stageQueueCommand(
						    {StageCmd::Command::PUSH, mksp<BuildingScreen>(this->state, building)});
					}
					if (vehicle)
					{
						orderSelect(StateRef<Vehicle>{state.get(), Vehicle::getId(*state, vehicle)},
						            buttonPressed == Event::MouseButton::Right,
						            modifierLCtrl || modifierRCtrl);
					}
					break;
				}
				case SelectionState::VehicleAttackBuilding:
				{
					if (building)
					{
						orderAttack(
						    StateRef<Building>{state.get(), Building::getId(*state, building)});
						setSelectionState(SelectionState::Normal);
					}
					break;
				}
				case SelectionState::VehicleAttackVehicle:
				{
					if (vehicle)
					{
						orderAttack(
						    StateRef<Vehicle>{state.get(), Vehicle::getId(*state, vehicle)});
						setSelectionState(SelectionState::Normal);
					}
					break;
				}
				case SelectionState::VehicleGotoBuilding:
				{
					if (building)
					{
						orderMove(building, modifierRCtrl || modifierLCtrl);
						setSelectionState(SelectionState::Normal);
					}
					break;
				}
				case SelectionState::VehicleGotoLocation:
				{
					// We always have a position if we came this far
					{
						orderMove(position, modifierRCtrl || modifierLCtrl);
						setSelectionState(SelectionState::Normal);
					}
					break;
				}
			}
		}
		return true;
	}
	return true;
}

bool CityView::handleGameStateEvent(Event *e)
{
	auto gameEvent = dynamic_cast<GameEvent *>(e);
	if (!gameEvent)
	{
		LogError("Invalid game state event");
		return true;
	}
	if (!gameEvent->message().empty())
	{
		state->logEvent(gameEvent);
		baseForm->findControlTyped<Ticker>("NEWS_TICKER")->addMessage(gameEvent->message());
		if (gameEvent->type != GameEventType::AlienSpotted)
		{
			fw().stageQueueCommand({StageCmd::Command::PUSH,
			                        mksp<NotificationScreen>(state, *this, gameEvent->message())});
		}
	}
	switch (gameEvent->type)
	{
		case GameEventType::AlienTakeover:
		{
			// FIXME: Proper takeover message
			auto gameOrgEvent = dynamic_cast<GameOrganisationEvent *>(e);
			fw().stageQueueCommand(
			    {StageCmd::Command::PUSH,
			     mksp<NotificationScreen>(state, *this, format("Aliens have taken over %s",
			                                                   gameOrgEvent->organisation->name))});
		}
		break;
		case GameEventType::DefendTheBase:
		{
			auto gameDefenseEvent = dynamic_cast<GameDefenseEvent *>(e);
			initiateDefenseMission(gameDefenseEvent->base, gameDefenseEvent->organisation);
			break;
		}
		case GameEventType::AlienSpotted:
		{
			auto ev = dynamic_cast<GameBuildingEvent *>(e);
			if (!ev)
			{
				LogError("Invalid spotted event");
			}
			fw().soundBackend->playSample(
			    listRandomiser(state->rng, state->city_common_sample_list->alertSounds));
			zoomLastEvent();
			setUpdateSpeed(UpdateSpeed::Speed1);
			fw().stageQueueCommand(
			    {StageCmd::Command::PUSH, mksp<AlertScreen>(state, ev->building)});
			break;
		}
		case GameEventType::ResearchCompleted:
		{
			auto ev = dynamic_cast<GameResearchEvent *>(e);
			if (!ev)
			{
				LogError("Invalid research event");
			}
			this->state->research.resortTopicList();
			sp<Facility> lab_facility;
			for (auto &base : state->player_bases)
			{
				for (auto &facility : base.second->facilities)
				{
					if (ev->lab == facility->lab)
					{
						lab_facility = facility;
						break;
					}
				}
				if (lab_facility)
					break;
			}
			if (!lab_facility)
			{
				LogError("No facilities matching lab");
			}
			auto game_state = this->state;
			auto ufopaedia_entry = ev->topic->ufopaedia_entry;
			sp<UfopaediaCategory> ufopaedia_category;
			if (ufopaedia_entry)
			{
				for (auto &cat : this->state->ufopaedia)
				{
					for (auto &entry : cat.second->entries)
					{
						if (ufopaedia_entry == entry.second)
						{
							ufopaedia_category = cat.second;
							break;
						}
					}
					if (ufopaedia_category)
						break;
				}
				if (!ufopaedia_category)
				{
					LogError("No UFOPaedia category found for entry %s", ufopaedia_entry->title);
				}
			}
			auto message_box = mksp<MessageBox>(
			    tr("RESEARCH COMPLETE"),
			    format("%s\n%s\n%s", tr("Research project completed:"), ev->topic->name,
			           tr("Do you wish to view the UFOpaedia report?")),
			    MessageBox::ButtonOptions::YesNo,
			    // Yes callback
			    [game_state, lab_facility, ufopaedia_category, ufopaedia_entry]() {
				    fw().stageQueueCommand(
				        {StageCmd::Command::PUSH, mksp<ResearchScreen>(game_state, lab_facility)});
				    if (ufopaedia_entry)
				    {
					    fw().stageQueueCommand(
					        {StageCmd::Command::PUSH,
					         mksp<UfopaediaCategoryView>(game_state, ufopaedia_category,
					                                     ufopaedia_entry)});
				    }
				},
			    // No callback
			    [game_state, lab_facility]() {
				    fw().stageQueueCommand(
				        {StageCmd::Command::PUSH, mksp<ResearchScreen>(game_state, lab_facility)});
				});
			fw().stageQueueCommand({StageCmd::Command::PUSH, message_box});
		}
		break;
		case GameEventType::ManufactureCompleted:
		{
			auto ev = dynamic_cast<GameManufactureEvent *>(e);
			if (!ev)
			{
				LogError("Invalid manufacture event");
			}
			sp<Facility> lab_facility;
			sp<Base> lab_base;
			for (auto &base : state->player_bases)
			{
				for (auto &facility : base.second->facilities)
				{
					if (ev->lab == facility->lab)
					{
						lab_facility = facility;
						lab_base = base.second;
						break;
					}
				}
				if (lab_facility)
					break;
			}
			if (!lab_facility)
			{
				LogError("No facilities matching lab");
			}
			auto game_state = this->state;

			UString item_name;
			switch (ev->topic->item_type)
			{
				case ResearchTopic::ItemType::VehicleEquipment:
					item_name = game_state->vehicle_equipment[ev->topic->item_produced]->name;
					break;
				case ResearchTopic::ItemType::VehicleEquipmentAmmo:
					item_name = game_state->vehicle_ammo[ev->topic->item_produced]->name;
					break;
				case ResearchTopic::ItemType::AgentEquipment:
					item_name = game_state->agent_equipment[ev->topic->item_produced]->name;
					break;
				case ResearchTopic::ItemType::Craft:
					item_name = game_state->vehicle_types[ev->topic->item_produced]->name;
					break;
			}
			auto message_box = mksp<MessageBox>(
			    tr("MANUFACTURE COMPLETED"),
			    format("%s\n%s\n%s %d\n%d", lab_base->name, tr(item_name), tr("Quantity:"),
			           ev->goal, tr("Do you wish to reasign the Workshop?")),
			    MessageBox::ButtonOptions::YesNo,
			    // Yes callback
			    [game_state, lab_facility]() {
				    fw().stageQueueCommand(
				        {StageCmd::Command::PUSH, mksp<ResearchScreen>(game_state, lab_facility)});
				});
			fw().stageQueueCommand({StageCmd::Command::PUSH, message_box});
		}
		break;
		case GameEventType::ManufactureHalted:
		{
			auto ev = dynamic_cast<GameManufactureEvent *>(e);
			if (!ev)
			{
				LogError("Invalid manufacture event");
			}
			sp<Facility> lab_facility;
			sp<Base> lab_base;
			for (auto &base : state->player_bases)
			{
				for (auto &facility : base.second->facilities)
				{
					if (ev->lab == facility->lab)
					{
						lab_facility = facility;
						lab_base = base.second;
						break;
					}
				}
				if (lab_facility)
					break;
			}
			if (!lab_facility)
			{
				LogError("No facilities matching lab");
			}
			auto game_state = this->state;

			UString item_name;
			switch (ev->topic->item_type)
			{
				case ResearchTopic::ItemType::VehicleEquipment:
					item_name = game_state->vehicle_equipment[ev->topic->item_produced]->name;
					break;
				case ResearchTopic::ItemType::VehicleEquipmentAmmo:
					item_name = game_state->vehicle_ammo[ev->topic->item_produced]->name;
					break;
				case ResearchTopic::ItemType::AgentEquipment:
					item_name = game_state->agent_equipment[ev->topic->item_produced]->name;
					break;
				case ResearchTopic::ItemType::Craft:
					item_name = game_state->vehicles[ev->topic->item_produced]->name;
					break;
			}
			auto message_box =
			    mksp<MessageBox>(tr("MANUFACTURING HALTED"),
			                     format("%s\n%s\n%s %d/%d\n%d", lab_base->name, tr(item_name),
			                            tr("Completion status:"), ev->done, ev->goal,
			                            tr("Production costs exceed your available funds.")),
			                     MessageBox::ButtonOptions::Ok);
			fw().stageQueueCommand({StageCmd::Command::PUSH, message_box});
		}
		break;
		case GameEventType::FacilityCompleted:
		{
			auto ev = dynamic_cast<GameFacilityEvent *>(e);
			if (!ev)
			{
				LogError("Invalid facility event");
				return true;
			}
			auto message_box =
			    mksp<MessageBox>(tr("FACILITY COMPLETED"),
			                     format("%s\n%s", ev->base->name, tr(ev->facility->type->name)),
			                     MessageBox::ButtonOptions::Ok);
			fw().stageQueueCommand({StageCmd::Command::PUSH, message_box});
		}
		break;
		default:
			break;
	}
	return true;
}

void CityView::updateSelectedUnits()
{
	auto o = state->getPlayer();
	auto it = state->current_city->cityViewSelectedVehicles.begin();
	bool foundOwned = false;
	while (it != state->current_city->cityViewSelectedVehicles.end())
	{
		auto v = *it;
		if (!v || v->health <= 0)
		{
			it = state->current_city->cityViewSelectedVehicles.erase(it);
		}
		else
		{
			if (v->owner == o)
			{
				foundOwned = true;
			}
			it++;
		}
	}
	if (!foundOwned && selectionState != SelectionState::Normal)
	{
		setSelectionState(SelectionState::Normal);
	}
}

void CityView::setUpdateSpeed(UpdateSpeed updateSpeed)
{
	if (this->updateSpeed == updateSpeed)
	{
		return;
	}
	this->lastSpeed = this->updateSpeed;
	switch (updateSpeed)
	{
		case UpdateSpeed::Pause:
			baseForm->findControlTyped<RadioButton>("BUTTON_SPEED0")->setChecked(true);
			break;
		case UpdateSpeed::Speed1:
			baseForm->findControlTyped<RadioButton>("BUTTON_SPEED1")->setChecked(true);
			break;
		case UpdateSpeed::Speed2:
			baseForm->findControlTyped<RadioButton>("BUTTON_SPEED2")->setChecked(true);
			break;
		case UpdateSpeed::Speed3:
			baseForm->findControlTyped<RadioButton>("BUTTON_SPEED3")->setChecked(true);
			break;
		case UpdateSpeed::Speed4:
			baseForm->findControlTyped<RadioButton>("BUTTON_SPEED4")->setChecked(true);
			break;
		case UpdateSpeed::Speed5:
			baseForm->findControlTyped<RadioButton>("BUTTON_SPEED5")->setChecked(true);
			break;
	}
}

void CityView::zoomLastEvent()
{
	if (!state->messages.empty())
	{
		auto message = state->messages.back();
		if (message.location != EventMessage::NO_LOCATION)
		{
			setScreenCenterTile(message.location);
		}
	}
}
void CityView::setSelectionState(SelectionState selectionState)
{
	this->selectionState = selectionState;
	switch (selectionState)
	{
		case SelectionState::Normal:
		{
			overlayTab->setVisible(false);
			break;
		}
		case SelectionState::VehicleAttackBuilding:
		{
			overlayTab->findControlTyped<Label>("TEXT")->setText(
			    tr("Click on building for selected vehicle to attack"));
			overlayTab->setVisible(true);
			break;
		}
		case SelectionState::VehicleAttackVehicle:
		{
			overlayTab->findControlTyped<Label>("TEXT")->setText(
			    tr("Click on target vehicle for selected vehicle"));
			overlayTab->setVisible(true);
			break;
		}
		case SelectionState::VehicleGotoBuilding:
		{
			overlayTab->findControlTyped<Label>("TEXT")->setText(
			    tr("Click on destination building for selected vehicle"));
			overlayTab->setVisible(true);
			break;
		}
		case SelectionState::VehicleGotoLocation:
		{
			overlayTab->findControlTyped<Label>("TEXT")->setText(
			    tr("Click on destination map point for selected vehicle"));
			overlayTab->setVisible(true);
			break;
		}
	}
}
}; // namespace OpenApoc
