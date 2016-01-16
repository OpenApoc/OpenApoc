#include "library/sp.h"
#include "game/city/city.h"
#include "game/city/cityview.h"
#include "framework/framework.h"
#include "game/resources/gamecore.h"
#include "game/ufopaedia/ufopaedia.h"
#include "game/city/infiltrationscreen.h"
#include "game/city/scorescreen.h"
#include "game/general/ingameoptions.h"
#include "game/city/vehiclemission.h"
#include "game/city/vehicle.h"
#include "game/city/building.h"
#include "game/base/basescreen.h"
#include "game/base/vequipscreen.h"
#include "game/tileview/tileobject_vehicle.h"
#include "game/tileview/tileobject_scenery.h"
#include "game/tileview/voxel.h"
#include "game/city/scenery.h"
#include "game/city/buildingscreen.h"

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
} // anonymous namespace

CityView::CityView(sp<GameState> state)
    : TileView(state->city->map, Vec3<int>{CITY_TILE_X, CITY_TILE_Y, CITY_TILE_Z},
               Vec2<int>{CITY_STRAT_TILE_X, CITY_STRAT_TILE_Y}, TileViewMode::Isometric),
      baseForm(fw().gamecore->GetForm("FORM_CITY_UI")), updateSpeed(UpdateSpeed::Speed1),
      state(state)
{
	baseForm->FindControlTyped<RadioButton>("BUTTON_SPEED1")->SetChecked(true);
	for (auto &formName : TAB_FORM_NAMES)
	{
		sp<Form> f(fw().gamecore->GetForm(formName));
		if (!f)
		{
			LogError("Failed to load form \"%s\"", formName.c_str());
			return;
		}
		f->takesFocus = false;
		this->uiTabs.push_back(f);
	}
	this->activeTab = this->uiTabs[0];

	for (auto &base : state->playerBases)
	{
		auto bld = base->bld.lock();
		if (!bld)
		{
			LogError("Base with invalid bld");
		}
		auto bldBounds = bld->def.getBounds();

		Vec2<int> buildingCenter = (bldBounds.p0 + bldBounds.p1) / 2;
		this->setScreenCenterTile(buildingCenter);
		break;
	}

	for (auto &iconResource : CITY_ICON_RESOURCES)
	{
		auto &type = iconResource.first;
		auto &path = iconResource.second;
		auto image = fw().data->load_image(path);
		if (!image)
		{
			LogError("Failed to open city icon resource \"%s\"", path.c_str());
		}
		this->icons[type] = image;
	}

	auto img = std::make_shared<RGBImage>(Vec2<int>{1, 2});
	{
		RGBImageLock l(img);
		l.set({0, 0}, Colour{255, 255, 219});
		l.set({0, 1}, Colour{215, 0, 0});
	}
	this->healthImage = img;
	img = std::make_shared<RGBImage>(Vec2<int>{1, 2});
	{
		RGBImageLock l(img);
		l.set({0, 0}, Colour{160, 236, 252});
		l.set({0, 1}, Colour{4, 100, 252});
	}
	this->shieldImage = img;
}

CityView::~CityView() {}

void CityView::Render()
{
	TileView::Render();
	if (state->showVehiclePath)
	{
		for (auto v : state->city->vehicles)
		{
			auto vTile = v->tileObject;
			if (!vTile)
				continue;
			auto &path = v->missions.front()->getCurrentPlannedPath();
			Vec3<float> prevPos = vTile->getPosition();
			for (auto *tile : path)
			{
				auto screenOffset = this->getScreenOffset();
				Vec3<float> pos = tile->position;
				Vec2<float> screenPosA = this->tileToScreenCoords(prevPos);
				screenPosA += screenOffset;
				Vec2<float> screenPosB = this->tileToScreenCoords(pos);
				screenPosB += screenOffset;

				fw().renderer->drawLine(screenPosA, screenPosB, Colour{255, 0, 0, 128});

				prevPos = pos;
			}
		}
	}
	activeTab->Render();
	baseForm->Render();
	fw().gamecore->MouseCursor->Render();
}

void CityView::Update(StageCmd *const cmd)
{
	unsigned int ticks = 0;
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
			LogWarning("FIXME: Sort out higher speed to not just hammer update (GOD-SLOW) - "
			           "demoting to speed4");
			/* FIXME: This should be '600' according to the scale above? */
			ticks = 6;
			this->updateSpeed = UpdateSpeed::Speed4;
			break;
	}

	// As the city view is the only screen that actually progresses time, incremnt that here
	state->time += ticks;

	auto clockControl = baseForm->FindControlTyped<Label>("CLOCK");

	auto seconds = state->time / TICKS_PER_SECOND;
	auto minutes = seconds / 60;
	auto hours = minutes / 60;

	unsigned secondsClamped = seconds % 60;
	unsigned minutesClamped = minutes % 60;
	unsigned hoursClamped = hours % 24;

	auto timeString =
	    UString::format("%02u:%02u:%02u", hoursClamped, minutesClamped, secondsClamped);
	clockControl->SetText(timeString);

	*cmd = stageCmd;
	stageCmd = StageCmd();

	state->city->update(*state, ticks);

	// FIXME: Possibly more efficient ways than re-generating all controls every frame?

	// Setup owned vehicle list controls
	auto *ownedVehicleList = uiTabs[1]->FindControlTyped<ListBox>("OWNED_VEHICLE_LIST");
	if (!ownedVehicleList)
	{
		LogError("Failed to find \"OWNED_VEHICLE_LIST\" control on city tab \"%s\"",
		         TAB_FORM_NAMES[1].c_str());
	}

	// Clear the list
	this->playerVehicleListControls.clear();
	ownedVehicleList->Clear();
	ownedVehicleList->ItemSpacing = 0;

	auto selected = this->selectedVehicle.lock();

	for (auto &v : state->getPlayer()->vehicles)
	{
		auto vehicle = v.lock();
		if (!vehicle)
		{
			continue;
		}
		// FIXME: Add selection
		sp<Image> frame;
		if (vehicle == selected)
		{
			frame = this->icons[CityIcon::SelectedFrame];
		}
		else
		{
			frame = this->icons[CityIcon::UnselectedFrame];
		}
		auto baseControl = new GraphicButton(nullptr, frame, frame);
		baseControl->Size = frame->size;
		// FIXME: There's an extra 1 pixel here that's annoying
		baseControl->Size.x -= 1;
		baseControl->Name = "OWNED_VEHICLE_FRAME_" + vehicle->name;
		auto vehicleIcon = new Graphic(baseControl, vehicle->type.icon);
		vehicleIcon->AutoSize = true;
		vehicleIcon->Location = {1, 1};
		vehicleIcon->Name = "OWNED_VEHICLE_ICON_" + vehicle->name;
		ownedVehicleList->AddItem(baseControl);

		int currentHealth;
		int maxHealth;
		sp<Image> img;

		if (vehicle->getShield() != 0)
		{
			img = this->shieldImage;
			currentHealth = vehicle->getShield();
			maxHealth = vehicle->getMaxShield();
		}
		else
		{
			img = this->healthImage;
			currentHealth = vehicle->getHealth();
			maxHealth = vehicle->getMaxHealth();
		}

		auto healthGraphic = new Graphic(baseControl, img);

		// FIXME: Put these somewhere slightly less magic?
		Vec2<int> healthBarOffset = {27, 2};
		Vec2<int> healthBarSize = {3, 20};

		float healthProportion = (float)currentHealth / (float)maxHealth;

		// This is a bit annoying as the health bar starts at the bottom, but the coord origin is
		// top-left, so fix that up a bit
		int healthBarHeight = (float)healthBarSize.y * healthProportion;
		healthBarOffset.y = healthBarOffset.y + (healthBarSize.y - healthBarHeight);
		healthBarSize.y = healthBarHeight;
		healthGraphic->Location = healthBarOffset;
		healthGraphic->Size = healthBarSize;
		healthGraphic->ImagePosition = FillMethod::Stretch;

		// FIXME: Currently we might get events from 'any' of the controls here, maybe this could be
		// simplified by having a top-level 'full-size' transparent button or something equally
		// evil? (But hopefully less evil than this?)
		this->playerVehicleListControls[baseControl] = vehicle;
		this->playerVehicleListControls[vehicleIcon] = vehicle;
		this->playerVehicleListControls[healthGraphic] = vehicle;
	}

	activeTab->Update();
	baseForm->Update();
}

void CityView::EventOccurred(Event *e)
{
	fw().gamecore->MouseCursor->EventOccured(e);
	baseForm->EventOccured(e);
	activeTab->EventOccured(e);

	if (!e->Handled)
	{
		if (e->Type == EVENT_FORM_INTERACTION)
		{
			if (e->Data.Forms.EventFlag == FormEventType::MouseDown)
			{
				auto it = this->playerVehicleListControls.find(e->Data.Forms.RaisedBy);

				if (it != this->playerVehicleListControls.end())
				{
					auto vehicle = it->second.lock();
					if (vehicle)
					{
						this->selectedVehicle = vehicle;
					}
					return;
				}
			}

			else if (e->Data.Forms.EventFlag == FormEventType::ButtonClick)
			{
				auto &cname = e->Data.Forms.RaisedBy->Name;

				if (cname == "BUTTON_TAB_1")
				{
					this->activeTab = uiTabs[0];
				}
				else if (cname == "BUTTON_TAB_2")
				{
					this->activeTab = uiTabs[1];
				}
				else if (cname == "BUTTON_TAB_3")
				{
					this->activeTab = uiTabs[2];
				}
				else if (cname == "BUTTON_TAB_4")
				{
					this->activeTab = uiTabs[3];
				}
				else if (cname == "BUTTON_TAB_5")
				{
					this->activeTab = uiTabs[4];
				}
				else if (cname == "BUTTON_TAB_6")
				{
					this->activeTab = uiTabs[5];
				}
				else if (cname == "BUTTON_TAB_7")
				{
					this->activeTab = uiTabs[6];
				}
				else if (cname == "BUTTON_TAB_8")
				{
					this->activeTab = uiTabs[7];
				}
				else if (cname == "BUTTON_SHOW_ALIEN_INFILTRATION")
				{
					stageCmd.cmd = StageCmd::Command::PUSH;
					stageCmd.nextStage = std::make_shared<InfiltrationScreen>();
					return;
				}
				else if (cname == "BUTTON_SHOW_SCORE")
				{
					stageCmd.cmd = StageCmd::Command::PUSH;
					stageCmd.nextStage = std::make_shared<ScoreScreen>();
					return;
				}
				else if (cname == "BUTTON_SHOW_UFOPAEDIA")
				{
					stageCmd.cmd = StageCmd::Command::PUSH;
					stageCmd.nextStage = std::make_shared<Ufopaedia>();
					return;
				}
				else if (cname == "BUTTON_SHOW_OPTIONS")
				{
					stageCmd.cmd = StageCmd::Command::PUSH;
					stageCmd.nextStage = std::make_shared<InGameOptions>(state);
					return;
				}
				else if (cname == "BUTTON_SHOW_LOG")
				{
					LogWarning("Show log");
				}
				else if (cname == "BUTTON_ZOOM_EVENT")
				{
					LogWarning("Zoom to event");
				}
				else if (cname == "BUTTON_SHOW_BASE")
				{
					stageCmd.cmd = StageCmd::Command::PUSH;
					stageCmd.nextStage = std::make_shared<BaseScreen>(state);
					return;
				}
				else if (cname == "BUTTON_EQUIP_VEHICLE")
				{
					stageCmd.cmd = StageCmd::Command::PUSH;
					auto equipScreen = std::make_shared<VEquipScreen>(state);
					// FIXME: Restrict this to player owned vehicles?
					auto selectedVehicle = this->selectedVehicle.lock();
					if (selectedVehicle)
					{
						equipScreen->setSelectedVehicle(selectedVehicle);
					}
					stageCmd.nextStage = equipScreen;

					return;
				}
			}
			else if (e->Data.Forms.EventFlag == FormEventType::CheckBoxChange)
			{
				auto &cname = e->Data.Forms.RaisedBy->Name;
				if (cname == "BUTTON_FOLLOW_VEHICLE")
				{
					LogWarning("Follow vehicle");
				}
				else if (cname == "BUTTON_TOGGLE_STRATMAP")
				{
					bool strategy = dynamic_cast<CheckBox *>(e->Data.Forms.RaisedBy)->IsChecked();
					this->setViewMode(strategy ? TileViewMode::Strategy : TileViewMode::Isometric);
				}
				else
				{
					if (baseForm->FindControlTyped<RadioButton>("BUTTON_SPEED0")->IsChecked())
					{
						this->updateSpeed = UpdateSpeed::Pause;
					}
					else if (baseForm->FindControlTyped<RadioButton>("BUTTON_SPEED1")->IsChecked())
					{
						this->updateSpeed = UpdateSpeed::Speed1;
					}
					else if (baseForm->FindControlTyped<RadioButton>("BUTTON_SPEED2")->IsChecked())
					{
						this->updateSpeed = UpdateSpeed::Speed2;
					}
					else if (baseForm->FindControlTyped<RadioButton>("BUTTON_SPEED3")->IsChecked())
					{
						this->updateSpeed = UpdateSpeed::Speed3;
					}
					else if (baseForm->FindControlTyped<RadioButton>("BUTTON_SPEED4")->IsChecked())
					{
						this->updateSpeed = UpdateSpeed::Speed4;
					}
					else if (baseForm->FindControlTyped<RadioButton>("BUTTON_SPEED5")->IsChecked())
					{
						this->updateSpeed = UpdateSpeed::Speed5;
					}
				}
			}
		}
		else if (e->Type == EVENT_KEY_DOWN && e->Data.Keyboard.KeyCode == SDLK_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
		// FIXME: Check if scancode is better/worse
		else if (e->Type == EVENT_KEY_DOWN &&
		         SDL_GetScancodeFromKey(e->Data.Keyboard.KeyCode) == SDL_SCANCODE_R)
		{
			LogInfo("Repairing...");
			std::set<sp<Scenery>> stuffToRepair;
			for (auto &s : state->city->scenery)
			{
				if (s->canRepair())
				{
					stuffToRepair.insert(s);
				}
			}
			LogInfo("Repairing %u tiles out of %u", static_cast<unsigned>(stuffToRepair.size()),
			        static_cast<unsigned>(state->city->scenery.size()));

			for (auto &s : stuffToRepair)
			{
				s->repair(*state);
				state->city->fallingScenery.erase(s);
			}
		}
		else if (this->getViewMode() == TileViewMode::Strategy && e->Type == EVENT_MOUSE_DOWN &&
		         e->Data.Mouse.Button == 2)
		{
			Vec2<float> screenOffset = {this->getScreenOffset().x, this->getScreenOffset().y};
			auto clickTile = this->screenToTileCoords(
			    Vec2<float>{e->Data.Mouse.X, e->Data.Mouse.Y} - screenOffset, 0.0f);
			this->setScreenCenterTile({clickTile.x, clickTile.y});
		}
		else if (e->Type == EVENT_MOUSE_DOWN && e->Data.Mouse.Button == 1)
		{
			// If a click has not been handled by a form it's in the map. See if we intersect with
			// anything
			Vec2<float> screenOffset = {this->getScreenOffset().x, this->getScreenOffset().y};
			auto clickTop = this->screenToTileCoords(
			    Vec2<float>{e->Data.Mouse.X, e->Data.Mouse.Y} - screenOffset, 9.99f);
			auto clickBottom = this->screenToTileCoords(
			    Vec2<float>{e->Data.Mouse.X, e->Data.Mouse.Y} - screenOffset, 0.0f);
			auto collision = state->city->map.findCollision(clickTop, clickBottom);
			if (collision)
			{
				if (collision.obj->getType() == TileObject::Type::Scenery)
				{
					auto scenery =
					    std::dynamic_pointer_cast<TileObjectScenery>(collision.obj)->getOwner();
					LogInfo("Clicked on scenery at {%d,%d,%d}", scenery->pos.x, scenery->pos.y,
					        scenery->pos.z);
					auto building = scenery->building;
					if (building)
					{
						LogInfo("Scenery owned by building \"%s\"",
						        building->def.getName().c_str());
						stageCmd.cmd = StageCmd::Command::PUSH;
						stageCmd.nextStage = std::make_shared<BuildingScreen>(building);

						return;
					}
				}
				else if (collision.obj->getType() == TileObject::Type::Vehicle)
				{
					// TODO: Click on vehicle
				}
			}
		}
		else
		{
			TileView::EventOccurred(e);
		}
	}
}
}; // namespace OpenApoc
