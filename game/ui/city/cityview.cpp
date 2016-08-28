#include "game/ui/city/cityview.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/base/facility.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/gameevent.h"
#include "game/state/research.h"
#include "game/state/tileview/tileobject_scenery.h"
#include "game/state/tileview/tileobject_vehicle.h"
#include "game/state/tileview/voxel.h"
#include "game/ui/base/basegraphics.h"
#include "game/ui/base/basescreen.h"
#include "game/ui/base/researchscreen.h"
#include "game/ui/base/vequipscreen.h"
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
#include "library/sp.h"

namespace OpenApoc
{
namespace
{

static const std::map<CityIcon, UString> CITY_ICON_RESOURCES = {
    // FIXME: Put this in the rules somewhere?
    {CityIcon::UnselectedFrame,
     "PCK:xcom3/UFODATA/VS_ICON.PCK:xcom3/UFODATA/VS_ICON.TAB:37:xcom3/UFODATA/PAL_01.DAT"},
    {CityIcon::SelectedFrame,
     "PCK:xcom3/UFODATA/VS_ICON.PCK:xcom3/UFODATA/VS_ICON.TAB:39:xcom3/UFODATA/PAL_01.DAT"},
    {CityIcon::SelectedSecondaryFrame,
     "PCK:xcom3/UFODATA/VS_ICON.PCK:xcom3/UFODATA/VS_ICON.TAB:38:xcom3/UFODATA/PAL_01.DAT"},

    {CityIcon::InBase,
     "PCK:xcom3/UFODATA/VS_ICON.PCK:xcom3/UFODATA/VS_ICON.TAB:47:xcom3/UFODATA/PAL_01.DAT"},
    {CityIcon::InVehicle,
     "PCK:xcom3/UFODATA/VS_ICON.PCK:xcom3/UFODATA/VS_ICON.TAB:48:xcom3/UFODATA/PAL_01.DAT"},
    {CityIcon::InBuilding,
     "PCK:xcom3/UFODATA/VS_ICON.PCK:xcom3/UFODATA/VS_ICON.TAB:49:xcom3/UFODATA/PAL_01.DAT"},
    {CityIcon::InMotion,
     "PCK:xcom3/UFODATA/VS_ICON.PCK:xcom3/UFODATA/VS_ICON.TAB:50:xcom3/UFODATA/PAL_01.DAT"},
};
static const std::vector<UString> TAB_FORM_NAMES = {
    "FORM_CITY_UI_1", "FORM_CITY_UI_2", "FORM_CITY_UI_3", "FORM_CITY_UI_4",
    "FORM_CITY_UI_5", "FORM_CITY_UI_6", "FORM_CITY_UI_7", "FORM_CITY_UI_8",
};

static const std::vector<UString> CITY_ICON_VEHICLE_PASSENGER_COUNT_RESOURCES = {
    {""}, // 0 has no image
    {"PCK:xcom3/UFODATA/VS_ICON.PCK:xcom3/UFODATA/VS_ICON.TAB:51:xcom3/UFODATA/PAL_01.DAT"}, // 1
    {"PCK:xcom3/UFODATA/VS_ICON.PCK:xcom3/UFODATA/VS_ICON.TAB:52:xcom3/UFODATA/PAL_01.DAT"}, // 2
    {"PCK:xcom3/UFODATA/VS_ICON.PCK:xcom3/UFODATA/VS_ICON.TAB:53:xcom3/UFODATA/PAL_01.DAT"}, // 3
    {"PCK:xcom3/UFODATA/VS_ICON.PCK:xcom3/UFODATA/VS_ICON.TAB:54:xcom3/UFODATA/PAL_01.DAT"}, // 4
    {"PCK:xcom3/UFODATA/VS_ICON.PCK:xcom3/UFODATA/VS_ICON.TAB:55:xcom3/UFODATA/PAL_01.DAT"}, // 5
    {"PCK:xcom3/UFODATA/VS_ICON.PCK:xcom3/UFODATA/VS_ICON.TAB:56:xcom3/UFODATA/PAL_01.DAT"}, // 6
    {"PCK:xcom3/UFODATA/VS_ICON.PCK:xcom3/UFODATA/VS_ICON.TAB:57:xcom3/UFODATA/PAL_01.DAT"}, // 7
    {"PCK:xcom3/UFODATA/VS_ICON.PCK:xcom3/UFODATA/VS_ICON.TAB:58:xcom3/UFODATA/PAL_01.DAT"}, // 8
    {"PCK:xcom3/UFODATA/VS_ICON.PCK:xcom3/UFODATA/VS_ICON.TAB:59:xcom3/UFODATA/PAL_01.DAT"}, // 9
    {"PCK:xcom3/UFODATA/VS_ICON.PCK:xcom3/UFODATA/VS_ICON.TAB:60:xcom3/UFODATA/PAL_01.DAT"}, // 10
    {"PCK:xcom3/UFODATA/VS_ICON.PCK:xcom3/UFODATA/VS_ICON.TAB:61:xcom3/UFODATA/PAL_01.DAT"}, // 11
    {"PCK:xcom3/UFODATA/VS_ICON.PCK:xcom3/UFODATA/VS_ICON.TAB:62:xcom3/UFODATA/PAL_01.DAT"}, // 12
    {"PCK:xcom3/UFODATA/VS_ICON.PCK:xcom3/UFODATA/VS_ICON.TAB:63:xcom3/UFODATA/PAL_01.DAT"}, // 13+
};

} // anonymous namespace

CityView::CityView(sp<GameState> state)
    : TileView(*state->current_city->map, Vec3<int>{CITY_TILE_X, CITY_TILE_Y, CITY_TILE_Z},
               Vec2<int>{CITY_STRAT_TILE_X, CITY_STRAT_TILE_Y}, TileViewMode::Isometric),
      baseForm(ui().getForm("FORM_CITY_UI")), updateSpeed(UpdateSpeed::Speed1), state(state),
      followVehicle(false), selectionState(SelectionState::Normal),
      day_palette(fw().data->loadPalette("xcom3/ufodata/PAL_01.DAT")),
      twilight_palette(fw().data->loadPalette("xcom3/ufodata/PAL_02.DAT")),
      night_palette(fw().data->loadPalette("xcom3/ufodata/PAL_03.DAT"))
{
	baseForm->findControlTyped<RadioButton>("BUTTON_SPEED1")->setChecked(true);
	for (auto &formName : TAB_FORM_NAMES)
	{
		sp<Form> f(ui().getForm(formName));
		if (!f)
		{
			LogError("Failed to load form \"%s\"", formName.cStr());
			return;
		}
		f->takesFocus = false;
		this->uiTabs.push_back(f);
	}
	this->activeTab = this->uiTabs[0];

	// Refresh base views
	resume();

	auto bld = state->current_base->building;
	if (!bld)
	{
		LogError("Base with invalid bld");
	}
	auto bldBounds = bld->bounds;

	Vec2<int> buildingCenter = (bldBounds.p0 + bldBounds.p1) / 2;
	this->setScreenCenterTile(buildingCenter);

	for (auto &iconResource : CITY_ICON_RESOURCES)
	{
		auto &type = iconResource.first;
		auto &path = iconResource.second;
		auto image = fw().data->loadImage(path);
		if (!image)
		{
			LogError("Failed to open city icon resource \"%s\"", path.cStr());
		}
		this->icons[type] = image;
	}

	for (auto &passengerResource : CITY_ICON_VEHICLE_PASSENGER_COUNT_RESOURCES)
	{
		auto image = fw().data->loadImage(passengerResource);
		if (!image && passengerResource != "")
		{
			LogError("Failed to open city vehicle passenger icon resource \"%s\"",
			         passengerResource.cStr());
		}
		this->vehiclePassengerCountIcons.push_back(image);
	}

	auto img = mksp<RGBImage>(Vec2<int>{1, 2});
	{
		RGBImageLock l(img);
		l.set({0, 0}, Colour{255, 255, 219});
		l.set({0, 1}, Colour{215, 0, 0});
	}
	this->healthImage = img;
	img = mksp<RGBImage>(Vec2<int>{1, 2});
	{
		RGBImageLock l(img);
		l.set({0, 0}, Colour{160, 236, 252});
		l.set({0, 1}, Colour{4, 100, 252});
	}
	this->shieldImage = img;

	this->baseForm->findControl("BUTTON_TAB_1")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *) { this->activeTab = this->uiTabs[0]; });
	this->baseForm->findControl("BUTTON_TAB_2")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *) { this->activeTab = this->uiTabs[1]; });
	this->baseForm->findControl("BUTTON_TAB_3")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *) { this->activeTab = this->uiTabs[2]; });
	this->baseForm->findControl("BUTTON_TAB_4")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *) { this->activeTab = this->uiTabs[3]; });
	this->baseForm->findControl("BUTTON_TAB_5")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *) { this->activeTab = this->uiTabs[4]; });
	this->baseForm->findControl("BUTTON_TAB_6")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *) { this->activeTab = this->uiTabs[5]; });
	this->baseForm->findControl("BUTTON_TAB_7")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *) { this->activeTab = this->uiTabs[6]; });
	this->baseForm->findControl("BUTTON_TAB_8")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *) { this->activeTab = this->uiTabs[7]; });
	this->baseForm->findControl("BUTTON_FOLLOW_VEHICLE")
	    ->addCallback(FormEventType::CheckBoxChange, [this](Event *e) {
		    this->followVehicle =
		        std::dynamic_pointer_cast<CheckBox>(e->forms().RaisedBy)->isChecked();
		});
	this->baseForm->findControl("BUTTON_TOGGLE_STRATMAP")
	    ->addCallback(FormEventType::CheckBoxChange, [this](Event *e) {
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
		    this->stageCmd.cmd = StageCmd::Command::PUSH;
		    this->stageCmd.nextStage = mksp<InfiltrationScreen>(this->state);
		});
	this->baseForm->findControl("BUTTON_SHOW_SCORE")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->stageCmd.cmd = StageCmd::Command::PUSH;
		    this->stageCmd.nextStage = mksp<ScoreScreen>(this->state);
		});
	this->baseForm->findControl("BUTTON_SHOW_UFOPAEDIA")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->stageCmd.cmd = StageCmd::Command::PUSH;
		    this->stageCmd.nextStage = mksp<UfopaediaView>(this->state);
		});
	this->baseForm->findControl("BUTTON_SHOW_OPTIONS")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->stageCmd.cmd = StageCmd::Command::PUSH;
		    this->stageCmd.nextStage = mksp<InGameOptions>(this->state);
		});
	this->baseForm->findControl("BUTTON_SHOW_LOG")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->stageCmd.cmd = StageCmd::Command::PUSH;
		    this->stageCmd.nextStage = mksp<MessageLogScreen>(this->state, *this);
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
		    this->stageCmd.cmd = StageCmd::Command::PUSH;
		    this->stageCmd.nextStage = mksp<BaseScreen>(this->state);
		});
	baseManagementForm->findControl("BUTTON_BUILD_BASE")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->stageCmd.cmd = StageCmd::Command::PUSH;
		    this->stageCmd.nextStage = mksp<BaseSelectScreen>(this->state, this->centerPos);
		});
	auto vehicleForm = this->uiTabs[1];
	vehicleForm->findControl("BUTTON_EQUIP_VEHICLE")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->stageCmd.cmd = StageCmd::Command::PUSH;
		    auto equipScreen = mksp<VEquipScreen>(this->state);
		    auto v = this->selectedVehicle.lock();
		    if (v && v->owner == this->state->getPlayer())
		    {
			    equipScreen->setSelectedVehicle(v);
		    }
		    this->stageCmd.nextStage = equipScreen;
		});
	vehicleForm->findControl("BUTTON_VEHICLE_BUILDING")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    auto v = this->selectedVehicle.lock();
		    if (v && v->owner == this->state->getPlayer())
		    {
			    auto b = v->currentlyLandedBuilding;
			    if (b)
			    {
				    this->stageCmd.cmd = StageCmd::Command::PUSH;
				    this->stageCmd.nextStage = mksp<BuildingScreen>(this->state, b);
			    }
		    }
		});
	vehicleForm->findControl("BUTTON_GOTO_BUILDING")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    auto v = this->selectedVehicle.lock();
		    if (v && v->owner == this->state->getPlayer())
		    {
			    LogInfo("Select building for vehicle \"%s\"", v->name.cStr());
			    this->selectionState = SelectionState::VehicleGotoBuilding;
		    }

		});
	vehicleForm->findControl("BUTTON_GOTO_LOCATION")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    auto v = this->selectedVehicle.lock();
		    if (v && v->owner == this->state->getPlayer())
		    {
			    LogInfo("Select building for vehicle \"%s\"", v->name.cStr());
			    this->selectionState = SelectionState::VehicleGotoLocation;
		    }

		});
	vehicleForm->findControl("BUTTON_GOTO_BASE")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    auto v = this->selectedVehicle.lock();
		    if (v && v->owner == this->state->getPlayer())
		    {
			    LogWarning("Goto base for vehicle \"%s\"", v->name.cStr());
			    auto bld = v->homeBuilding;
			    if (!bld)
			    {
				    LogError("Vehicle \"%s\" has no building", v->name.cStr());
			    }
			    LogWarning("Vehicle \"%s\" goto building \"%s\"", v->name.cStr(), bld->name.cStr());
			    // FIXME: Don't clear missions if not replacing current mission
			    v->missions.clear();
			    v->missions.emplace_back(VehicleMission::gotoBuilding(*v, bld));
			    v->missions.front()->start(*this->state, *v);
		    }
		});
	vehicleForm->findControl("BUTTON_VEHICLE_ATTACK")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    auto v = this->selectedVehicle.lock();
		    if (v && v->owner == this->state->getPlayer())
		    {
			    LogInfo("Select target for vehicle \"%s\"", v->name.cStr());
			    this->selectionState = SelectionState::VehicleAttackVehicle;
		    }

		});
	vehicleForm->findControl("BUTTON_VEHICLE_ATTACK_BUILDING")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    auto v = this->selectedVehicle.lock();
		    if (v && v->owner == this->state->getPlayer())
		    {
			    LogInfo("Select target building for vehicle \"%s\"", v->name.cStr());
			    this->selectionState = SelectionState::VehicleGotoLocation;
		    }

		});

	vehicleForm->findControl("BUTTON_ATTACK_MODE_AGGRESSIVE")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    auto v = this->selectedVehicle.lock();
		    if (v && v->owner == this->state->getPlayer())
		    {
			    v->attackMode = Vehicle::AttackMode::Aggressive;
		    }
		});
	vehicleForm->findControl("BUTTON_ATTACK_MODE_STANDARD")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    auto v = this->selectedVehicle.lock();
		    if (v && v->owner == this->state->getPlayer())
		    {
			    v->attackMode = Vehicle::AttackMode::Standard;
		    }
		});
	vehicleForm->findControl("BUTTON_ATTACK_MODE_DEFENSIVE")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    auto v = this->selectedVehicle.lock();
		    if (v && v->owner == this->state->getPlayer())
		    {
			    v->attackMode = Vehicle::AttackMode::Defensive;
		    }
		});
	vehicleForm->findControl("BUTTON_ATTACK_MODE_EVASIVE")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    auto v = this->selectedVehicle.lock();
		    if (v && v->owner == this->state->getPlayer())
		    {
			    v->attackMode = Vehicle::AttackMode::Evasive;
		    }
		});
	vehicleForm->findControl("BUTTON_ALTITUDE_HIGHEST")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    auto v = this->selectedVehicle.lock();
		    if (v && v->owner == this->state->getPlayer())
		    {
			    v->altitude = Vehicle::Altitude::Highest;
		    }
		});
	vehicleForm->findControl("BUTTON_ALTITUDE_HIGH")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    auto v = this->selectedVehicle.lock();
		    if (v && v->owner == this->state->getPlayer())
		    {
			    v->altitude = Vehicle::Altitude::High;
		    }
		});
	vehicleForm->findControl("BUTTON_ALTITUDE_STANDARD")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    auto v = this->selectedVehicle.lock();
		    if (v && v->owner == this->state->getPlayer())
		    {
			    v->altitude = Vehicle::Altitude::Standard;
		    }
		});
	vehicleForm->findControl("BUTTON_ALTITUDE_LOW")
	    ->addCallback(FormEventType::CheckBoxSelected, [this](Event *) {
		    auto v = this->selectedVehicle.lock();
		    if (v && v->owner == this->state->getPlayer())
		    {
			    v->altitude = Vehicle::Altitude::Low;
		    }
		});
}

CityView::~CityView() = default;

void CityView::begin()
{
	baseForm->findControlTyped<Ticker>("NEWS_TICKER")
	    ->addMessage(tr("Welcome to X-COM Apocalypse"));
}

void CityView::resume()
{
	this->uiTabs[0]->findControlTyped<Label>("TEXT_BASE_NAME")->setText(state->current_base->name);
	miniViews.clear();
	int b = 0;
	for (auto &pair : state->player_bases)
	{
		auto &viewBase = pair.second;
		auto viewName = UString::format("BUTTON_BASE_%d", ++b);
		auto view = this->uiTabs[0]->findControlTyped<GraphicButton>(viewName);
		if (!view)
		{
			LogError("Failed to find UI control matching \"%s\"", viewName.cStr());
		}
		view->setData(viewBase);
		auto viewImage = BaseGraphics::drawMiniBase(viewBase);
		view->setImage(viewImage);
		view->setDepressedImage(viewImage);
		view->addCallback(FormEventType::ButtonClick, [this](Event *e) {
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

	TileView::render();
	if (state->showVehiclePath)
	{
		for (auto pair : state->vehicles)
		{
			auto v = pair.second;
			if (v->city != state->current_city)
				continue;
			auto vTile = v->tileObject;
			if (!vTile)
				continue;
			auto &path = v->missions.front()->currentPlannedPath;
			Vec3<float> prevPos = vTile->getPosition();
			for (auto pos : path)
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

void CityView::update(StageCmd *const cmd)
{
	unsigned int ticks = 0;
	bool turbo = false;
	switch (this->updateSpeed)
	{
		case UpdateSpeed::Pause:
			ticks = 0;
			break;
		/* POSSIBLE FIXME: 'vanilla' apoc appears to implement Speed1 as 1/2 speed - that is only
		 * every other call calls the update loop, meaning that the later update tick counts are
		 * halved as well.
		 * This effectively means that all openapoc tick counts count for 1/2 the value of vanilla
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
			this->state->update();
			ticks--;
		}
	}
	auto clockControl = baseForm->findControlTyped<Label>("CLOCK");

	clockControl->setText(state->gameTime.getTimeString());

	// The palette fades from pal_03 at 3am to pal_02 at 6am then pal_01 at 9am
	// The reverse for 3pm, 6pm & 9pm

	auto hour = state->gameTime.getHours();
	sp<Palette> interpolated_palette;
	if (hour < 3 || hour >= 21)
	{
		interpolated_palette = this->night_palette;
	}
	else if (hour >= 9 && hour < 15)
	{
		interpolated_palette = this->day_palette;
	}
	else
	{
		sp<Palette> palette1;
		sp<Palette> palette2;
		float factor = 0;

		float hours_float = hour + (float)state->gameTime.getMinutes() / 60.0f;

		if (hour >= 3 && hour < 6)
		{
			palette1 = this->night_palette;
			palette2 = this->twilight_palette;
			factor = clamp((hours_float - 3.0f) / 3.0f, 0.0f, 1.0f);
		}
		else if (hour >= 6 && hour < 9)
		{
			palette1 = this->twilight_palette;
			palette2 = this->day_palette;
			factor = clamp((hours_float - 6.0f) / 3.0f, 0.0f, 1.0f);
		}
		else if (hour >= 15 && hour < 18)
		{
			palette1 = this->day_palette;
			palette2 = this->twilight_palette;
			factor = clamp((hours_float - 15.0f) / 3.0f, 0.0f, 1.0f);
		}
		else if (hour >= 18 && hour < 21)
		{
			palette1 = this->twilight_palette;
			palette2 = this->night_palette;
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
		         TAB_FORM_NAMES[1].cStr());
	}

	ownedVehicleList->ItemSpacing = 0;

	auto selected = this->selectedVehicle.lock();
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
			auto info = this->createVehicleInfo(vehicle);
			auto it = this->vehicleListControls.find(vehicle);
			sp<Control> control;
			if (it != this->vehicleListControls.end() && it->second.first == info)
			{
				// Control unchanged, don't regenerate
				control = it->second.second;
			}
			else
			{
				control = createVehicleInfoControl(info);
			}
			newVehicleListControls[vehicle] = std::make_pair(info, control);
			ownedVehicleList->addItem(control);

			control->addCallback(FormEventType::MouseDown, [this, vehicle](Event *) {
				this->selectedVehicle = vehicle;
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
						vehicleForm->findControlTyped<RadioButton>("BUTTON_ATTACK_MODE_AGGRESSIVE")
						    ->setChecked(true);
						break;
					case Vehicle::AttackMode::Standard:
						vehicleForm->findControlTyped<RadioButton>("BUTTON_ATTACK_MODE_STANDARD")
						    ->setChecked(true);
						break;
					case Vehicle::AttackMode::Defensive:
						vehicleForm->findControlTyped<RadioButton>("BUTTON_ATTACK_MODE_DEFENSIVE")
						    ->setChecked(true);
						break;
					case Vehicle::AttackMode::Evasive:
						vehicleForm->findControlTyped<RadioButton>("BUTTON_ATTACK_MODE_EVASIVE")
						    ->setChecked(true);
						break;
				}
			});
		}
	}

	// Clear the old list and reset to the new one (May be empty is not on a vehicle-displaying tab
	this->vehicleListControls.clear();
	this->vehicleListControls = std::move(newVehicleListControls);

	activeTab->update();
	baseForm->update();

	// If we have 'follow vehicle' enabled we clobber any other movement that may have occurred in
	// this frame
	if (this->followVehicle)
	{
		auto v = this->selectedVehicle.lock();
		if (v)
		{
			// The selected vehicle may not have a tile object if it's not on the map
			if (v->tileObject)
			{
				this->setScreenCenterTile(v->tileObject->getPosition());
			}
		}
	}
	*cmd = stageCmd;
	stageCmd = StageCmd();
}

void CityView::eventOccurred(Event *e)
{
	activeTab->eventOccured(e);
	baseForm->eventOccured(e);

	if (activeTab->eventIsWithin(e) || baseForm->eventIsWithin(e))
	{
		return;
	}

	if (e->type() == EVENT_KEY_DOWN && e->keyboard().KeyCode == SDLK_ESCAPE)
	{
		stageCmd.cmd = StageCmd::Command::PUSH;
		stageCmd.nextStage = mksp<InGameOptions>(state);
		return;
	}
	// FIXME: Check if scancode is better/worse
	else if (e->type() == EVENT_KEY_DOWN &&
	         SDL_GetScancodeFromKey(e->keyboard().KeyCode) == SDL_SCANCODE_R)
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
	}
	// Exclude mouse down events that are over the form
	else if (e->type() == EVENT_MOUSE_DOWN)
	{
		if (this->getViewMode() == TileViewMode::Strategy && e->type() == EVENT_MOUSE_DOWN &&
		    e->mouse().Button == 2)
		{
			Vec2<float> screenOffset = {this->getScreenOffset().x, this->getScreenOffset().y};
			auto clickTile = this->screenToTileCoords(
			    Vec2<float>{e->mouse().X, e->mouse().Y} - screenOffset, 0.0f);
			this->setScreenCenterTile({clickTile.x, clickTile.y});
		}
		else if (e->type() == EVENT_MOUSE_DOWN && e->mouse().Button == 1)
		{
			// If a click has not been handled by a form it's in the map. See if we intersect with
			// anything
			Vec2<float> screenOffset = {this->getScreenOffset().x, this->getScreenOffset().y};
			auto clickTop = this->screenToTileCoords(
			    Vec2<float>{e->mouse().X, e->mouse().Y} - screenOffset, 9.99f);
			auto clickBottom = this->screenToTileCoords(
			    Vec2<float>{e->mouse().X, e->mouse().Y} - screenOffset, 0.0f);
			auto collision = state->current_city->map->findCollision(clickTop, clickBottom);
			if (collision)
			{
				if (collision.obj->getType() == TileObject::Type::Scenery)
				{
					auto scenery =
					    std::dynamic_pointer_cast<TileObjectScenery>(collision.obj)->getOwner();
					LogInfo("Clicked on scenery at {%f,%f,%f}", scenery->currentPosition.x,
					        scenery->currentPosition.y, scenery->currentPosition.z);

					auto building = scenery->building;
					if (this->selectionState == SelectionState::VehicleGotoLocation)
					{

						auto v = this->selectedVehicle.lock();
						if (v && v->owner == state->getPlayer())
						{
							// Use vehicle altitude preference to select target height, clamp by map
							// size
							int altitude =
							    glm::min((int)v->altitude, state->current_city->map->size.z - 1);

							Vec3<int> targetPos{scenery->currentPosition.x,
							                    scenery->currentPosition.y, altitude};
							// FIXME: Don't clear missions if not replacing current mission
							v->missions.clear();
							v->missions.emplace_back(VehicleMission::gotoLocation(*v, targetPos));
							v->missions.front()->start(*this->state, *v);
							LogWarning("Vehicle \"%s\" going to location {%d,%d,%d}",
							           v->name.cStr(), targetPos.x, targetPos.y, targetPos.z);
						}
						this->selectionState = SelectionState::Normal;
					}
					else if (building)
					{
						LogInfo("Scenery owned by building \"%s\"", building->name.cStr());
						if (this->selectionState == SelectionState::VehicleGotoBuilding)
						{
							auto v = this->selectedVehicle.lock();
							if (v && v->owner == state->getPlayer())
							{
								LogWarning("Vehicle \"%s\" goto building \"%s\"", v->name.cStr(),
								           building->name.cStr());
								// FIXME: Don't clear missions if not replacing current mission
								v->missions.clear();
								v->missions.emplace_back(
								    VehicleMission::gotoBuilding(*v, building));
								v->missions.front()->start(*this->state, *v);
							}
							this->selectionState = SelectionState::Normal;
						}
						else if (this->selectionState == SelectionState::VehicleAttackBuilding)
						{
							auto v = this->selectedVehicle.lock();
							if (v)
							{
								// TODO: Attack building mission
								LogWarning("Vehicle \"%s\" attack building \"%s\"", v->name.cStr(),
								           building->name.cStr());
							}
							this->selectionState = SelectionState::Normal;
						}
						else if (this->selectionState == SelectionState::Normal)
						{
							stageCmd.cmd = StageCmd::Command::PUSH;
							stageCmd.nextStage = mksp<BuildingScreen>(this->state, building);
						}

						return;
					}
				}
				else if (collision.obj->getType() == TileObject::Type::Vehicle)
				{
					auto vehicle =
					    std::dynamic_pointer_cast<TileObjectVehicle>(collision.obj)->getVehicle();
					LogWarning("Clicked on vehicle \"%s\"", vehicle->name.cStr());

					if (this->selectionState == SelectionState::VehicleAttackVehicle)
					{
						auto v = this->selectedVehicle.lock();
						StateRef<Vehicle> vehicleRef(state.get(), vehicle);

						if (v && v->owner == state->getPlayer() && v != vehicle)
						{
							// FIXME: Don't clear missions if not replacing current mission
							v->missions.clear();
							v->missions.emplace_back(VehicleMission::attackVehicle(*v, vehicleRef));
							v->missions.front()->start(*this->state, *v);
						}
						this->selectionState = SelectionState::Normal;
					}
				}
			}
		}
	}
	else if (e->type() == EVENT_GAME_STATE)
	{
		auto gameEvent = dynamic_cast<GameEvent *>(e);
		if (!gameEvent)
		{
			LogError("Invalid game state event");
		}
		if (!gameEvent->message().empty())
		{
			state->logEvent(gameEvent);
			baseForm->findControlTyped<Ticker>("NEWS_TICKER")->addMessage(gameEvent->message());
			auto notification = mksp<NotificationScreen>(state, *this, gameEvent->message());
			stageCmd.cmd = StageCmd::Command::PUSH;
			stageCmd.nextStage = notification;
		}
		switch (gameEvent->type)
		{
			default:
				break;
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
						LogError("No UFOPaedia category found for entry %s",
						         ufopaedia_entry->title.cStr());
					}
				}
				auto message_box = mksp<MessageBox>(
				    tr("RESEARCH COMPLETE"),
				    UString::format("%s\n%s\n%s", tr("Research project completed:"),
				                    ev->topic->name,
				                    tr("Do you wish to view the UFOpaedia report?")),
				    MessageBox::ButtonOptions::YesNo,
				    // Yes callback
				    [game_state, lab_facility, ufopaedia_category, ufopaedia_entry]() {
					    fw().stagePush(mksp<ResearchScreen>(game_state, lab_facility));
					    if (ufopaedia_entry)
						    fw().stagePush(mksp<UfopaediaCategoryView>(
						        game_state, ufopaedia_category, ufopaedia_entry));
					},
				    // No callback
				    [game_state, lab_facility]() {
					    fw().stagePush(mksp<ResearchScreen>(game_state, lab_facility));
					});
				stageCmd.cmd = StageCmd::Command::PUSH;
				stageCmd.nextStage = message_box;
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
				    UString::format("%s\n%s\n%s %d\n%d", lab_base->name, tr(item_name.cStr()),
				                    tr("Quantity:"), ev->goal,
				                    tr("Do you wish to reasign the Workshop?")),
				    MessageBox::ButtonOptions::YesNo,
				    // Yes callback
				    [game_state, lab_facility]() {
					    fw().stagePush(mksp<ResearchScreen>(game_state, lab_facility));
					});
				stageCmd.cmd = StageCmd::Command::PUSH;
				stageCmd.nextStage = message_box;
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

				auto message_box = mksp<MessageBox>(
				    tr("MANUFACTURING HALTED"),
				    UString::format("%s\n%s\n%s %d/%d\n%d", lab_base->name, tr(item_name.cStr()),
				                    tr("Completion status:"), ev->done, ev->goal,
				                    tr("Production costs exceed your available funds.")),
				    MessageBox::ButtonOptions::Ok);
				stageCmd.cmd = StageCmd::Command::PUSH;
				stageCmd.nextStage = message_box;
			}
			break;
			case GameEventType::FacilityCompleted:
			{
				auto ev = dynamic_cast<GameFacilityEvent *>(e);
				if (!ev)
				{
					LogError("Invalid facility event");
					return;
				}
				auto message_box = mksp<MessageBox>(
				    tr("FACILITY COMPLETED"),
				    UString::format("%s\n%s", ev->base->name, tr(ev->facility->type->name)),
				    MessageBox::ButtonOptions::Ok);
				stageCmd.cmd = StageCmd::Command::PUSH;
				stageCmd.nextStage = message_box;
			}
			break;
		}
	}
	else
	{
		TileView::eventOccurred(e);
	}
}

VehicleTileInfo CityView::createVehicleInfo(sp<Vehicle> v)
{
	VehicleTileInfo t;
	t.vehicle = v;
	t.selected = (v == this->selectedVehicle.lock());

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

	auto b = v->currentlyLandedBuilding;
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

sp<Control> CityView::createVehicleInfoControl(const VehicleTileInfo &info)
{
	LogInfo("Creating city info control for vehicle \"%s\"", info.vehicle->name.cStr());

	auto frame = info.selected ? this->icons[CityIcon::SelectedFrame]
	                           : this->icons[CityIcon::UnselectedFrame];
	auto baseControl = mksp<GraphicButton>(frame, frame);
	baseControl->Size = frame->size;
	// FIXME: There's an extra 1 pixel here that's annoying
	baseControl->Size.x -= 1;
	baseControl->Name = "OWNED_VEHICLE_FRAME_" + info.vehicle->name;

	auto vehicleIcon = baseControl->createChild<Graphic>(info.vehicle->type->icon);
	vehicleIcon->AutoSize = true;
	vehicleIcon->Location = {1, 1};
	vehicleIcon->Name = "OWNED_VEHICLE_ICON_" + info.vehicle->name;

	// FIXME: Put these somewhere slightly less magic?
	Vec2<int> healthBarOffset = {27, 2};
	Vec2<int> healthBarSize = {3, 20};

	auto healthImg = info.shield ? this->shieldImage : this->healthImage;

	auto healthGraphic = baseControl->createChild<Graphic>(healthImg);
	// This is a bit annoying as the health bar starts at the bottom, but the coord origin is
	// top-left, so fix that up a bit
	int healthBarHeight = (float)healthBarSize.y * info.healthProportion;
	healthBarOffset.y = healthBarOffset.y + (healthBarSize.y - healthBarHeight);
	healthBarSize.y = healthBarHeight;
	healthGraphic->Location = healthBarOffset;
	healthGraphic->Size = healthBarSize;
	healthGraphic->ImagePosition = FillMethod::Stretch;

	sp<Graphic> stateGraphic;

	switch (info.state)
	{
		case CityUnitState::InBase:
			stateGraphic = vehicleIcon->createChild<Graphic>(this->icons[CityIcon::InBase]);
			break;
		case CityUnitState::InVehicle:
			stateGraphic = vehicleIcon->createChild<Graphic>(this->icons[CityIcon::InVehicle]);
			break;
		case CityUnitState::InBuilding:
			stateGraphic = vehicleIcon->createChild<Graphic>(this->icons[CityIcon::InBuilding]);
			break;
		case CityUnitState::InMotion:
			stateGraphic = vehicleIcon->createChild<Graphic>(this->icons[CityIcon::InMotion]);
			break;
	}

	stateGraphic->AutoSize = true;
	stateGraphic->Location = {0, 0};
	stateGraphic->Name = "OWNED_VEHICLE_STATE_" + info.vehicle->name;

	if (info.passengers)
	{
		auto passengerGraphic =
		    vehicleIcon->createChild<Graphic>(this->vehiclePassengerCountIcons[info.passengers]);
		passengerGraphic->AutoSize = true;
		passengerGraphic->Location = {0, 0};
		passengerGraphic->Name = "OWNED_VEHICLE_PASSENGERS_" + info.vehicle->name;
	}

	return baseControl;
}

bool VehicleTileInfo::operator==(const VehicleTileInfo &other) const
{
	return (this->vehicle == other.vehicle && this->selected == other.selected &&
	        this->healthProportion == other.healthProportion && this->shield == other.shield &&
	        this->passengers == other.passengers && this->state == other.state);
}

void CityView::setUpdateSpeed(UpdateSpeed updateSpeed)
{
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
		if (message.getMapLocation(*state) != EventMessage::NO_LOCATION)
		{
			setScreenCenterTile(message.getMapLocation(*state));
		}
	}
}
}; // namespace OpenApoc
