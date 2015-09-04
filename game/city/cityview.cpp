#include "game/city/city.h"
#include "game/city/cityview.h"
#include "framework/framework.h"
#include "game/resources/gamecore.h"
#include "game/ufopaedia/ufopaedia.h"
#include "game/city/infiltrationscreen.h"
#include "game/city/scorescreen.h"
#include "game/general/ingameoptions.h"

namespace OpenApoc
{

CityView::CityView(Framework &fw)
    : TileView(fw, *fw.state->city, Vec3<int>{CITY_TILE_X, CITY_TILE_Y, CITY_TILE_Z})
{
	static const std::vector<UString> tabFormNames = {
	    "FORM_CITY_UI_1", "FORM_CITY_UI_2", "FORM_CITY_UI_3", "FORM_CITY_UI_4",
	    "FORM_CITY_UI_5", "FORM_CITY_UI_6", "FORM_CITY_UI_7", "FORM_CITY_UI_8",
	};

	for (auto &formName : tabFormNames) {
		Form *f = fw.gamecore->GetForm(formName);
		if (!f) {
			LogError("Failed to load form \"%s\"", formName.str().c_str());
			return;
		}
		f->takesFocus = false;
		this->uiTabs.push_back(f);
	}
	this->activeTab = this->uiTabs[0];
}

CityView::~CityView() {}

void CityView::Render()
{
	TileView::Render();
	if (fw.state->showVehiclePath) {
		for (auto obj : this->map.activeObjects) {
			auto vTile = std::dynamic_pointer_cast<VehicleTileObject>(obj);
			if (!vTile)
				continue;
			auto &v = vTile->getVehicle();
			auto &path = v.mission->getCurrentPlannedPath();
			Vec3<float> prevPos = vTile->getPosition();
			for (auto *tile : path) {
				Vec3<float> pos = tile->position;
				Vec2<float> screenPosA = this->tileToScreenCoords(prevPos);
				screenPosA.x += this->offsetX;
				screenPosA.y += this->offsetX;
				Vec2<float> screenPosB = this->tileToScreenCoords(pos);
				screenPosB.x += this->offsetX;
				screenPosB.y += this->offsetX;

				fw.renderer->drawLine(screenPosA, screenPosB, Colour{255, 0, 0, 128});

				prevPos = pos;
			}
		}
	}
	activeTab->Render();
	fw.gamecore->MouseCursor->Render();
}

void CityView::Update(StageCmd *const cmd)
{
	TileView::Update(cmd);
	activeTab->Update();
}

void CityView::EventOccurred(Event *e)
{
	fw.gamecore->MouseCursor->EventOccured(e);
	activeTab->EventOccured(e);
	if (!e->Handled) {
		if (e->Type == EVENT_FORM_INTERACTION) {
			if (e->Data.Forms.EventFlag == FormEventType::ButtonClick) {
				auto &cname = e->Data.Forms.RaisedBy->Name;
				if (cname == "BUTTON_TAB_1")
					this->activeTab = uiTabs[0];
				else if (cname == "BUTTON_TAB_2")
					this->activeTab = uiTabs[1];
				else if (cname == "BUTTON_TAB_3")
					this->activeTab = uiTabs[2];
				else if (cname == "BUTTON_TAB_4")
					this->activeTab = uiTabs[3];
				else if (cname == "BUTTON_TAB_5")
					this->activeTab = uiTabs[4];
				else if (cname == "BUTTON_TAB_6")
					this->activeTab = uiTabs[5];
				else if (cname == "BUTTON_TAB_7")
					this->activeTab = uiTabs[6];
				else if (cname == "BUTTON_TAB_8")
					this->activeTab = uiTabs[7];
				else if (cname == "BUTTON_TOGGLE_STRATMAP")
					LogError("Toggle stratmap");
				else if (cname == "BUTTON_SHOW_ALIEN_INFILTRATION") {
					stageCmd.cmd = StageCmd::Command::PUSH;
					stageCmd.nextStage = std::make_shared<InfiltrationScreen>(fw);
					return;
				} else if (cname == "BUTTON_SHOW_SCORE") {
					stageCmd.cmd = StageCmd::Command::PUSH;
					stageCmd.nextStage = std::make_shared<ScoreScreen>(fw);
					return;
				} else if (cname == "BUTTON_SHOW_UFOPAEDIA") {
					stageCmd.cmd = StageCmd::Command::PUSH;
					stageCmd.nextStage = std::make_shared<Ufopaedia>(fw);
					return;
				} else if (cname == "BUTTON_SHOW_OPTIONS") {
					stageCmd.cmd = StageCmd::Command::PUSH;
					stageCmd.nextStage = std::make_shared<InGameOptions>(fw);
					return;
				} else if (cname == "BUTTON_SHOW_LOG")
					LogWarning("Show log");
				else if (cname == "BUTTON_ZOOM_EVENT")
					LogWarning("Zoom to event");
				else if (cname == "BUTTON_SPEED0") {
					LogWarning("Set speed 0");
					this->updateSpeed = UpdateSpeed::Pause;
				} else if (cname == "BUTTON_SPEED1") {
					LogWarning("Set speed 1");
					this->updateSpeed = UpdateSpeed::Speed1;
				} else if (cname == "BUTTON_SPEED2") {
					LogWarning("Set speed 2");
					this->updateSpeed = UpdateSpeed::Speed2;
				} else if (cname == "BUTTON_SPEED3") {
					LogWarning("Set speed 3");
					this->updateSpeed = UpdateSpeed::Speed3;
				} else if (cname == "BUTTON_SPEED4") {
					LogWarning("Set speed 4");
					this->updateSpeed = UpdateSpeed::Speed4;
				} else if (cname == "BUTTON_SPEED5") {
					LogWarning("Set speed 5");
					this->updateSpeed = UpdateSpeed::Speed5;
				}
			}

		} else {
			TileView::EventOccurred(e);
		}
	}
}

} // namespace OpenApoc
