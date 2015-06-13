#include "game/city/city.h"
#include "game/city/cityview.h"
#include "framework/framework.h"
#include "game/resources/gamecore.h"
#include "game/ufopaedia/ufopaedia.h"
#include "game/city/infiltrationscreen.h"
#include "game/city/scorescreen.h"
#include "game/general/optionsmenu.h"

namespace OpenApoc {

CityView::CityView(Framework &fw)
	: TileView(fw, *fw.state.city, Vec3<int>{CITY_TILE_X, CITY_TILE_Y, CITY_TILE_Z})
{
	static const std::vector<UString> tabFormNames = 
	{
		"FORM_CITY_UI_1",
		"FORM_CITY_UI_2",
		"FORM_CITY_UI_3",
		"FORM_CITY_UI_4",
		"FORM_CITY_UI_5",
		"FORM_CITY_UI_6",
		"FORM_CITY_UI_7",
		"FORM_CITY_UI_8",
	};

	for (auto &formName : tabFormNames)
	{
		Form *f = fw.gamecore->GetForm(formName);
		if (!f)
		{
			LogError("Failed to load form \"%s\"", formName.str().c_str());
			return;
		}
		f->takesFocus = false;
		this->uiTabs.push_back(f);
	}
	this->activeTab = this->uiTabs[0];
}

CityView::~CityView()
{

}

void
CityView::Render()
{
	TileView::Render();
	activeTab->Render();
	fw.gamecore->MouseCursor->Render();
}

void
CityView::Update(StageCmd * const cmd)
{
	TileView::Update(cmd);
	activeTab->Update();
}

void
CityView::EventOccurred(Event *e)
{
	fw.gamecore->MouseCursor->EventOccured( e );
	activeTab->EventOccured(e);
	if (!e->Handled)
	{
		if (e->Type == EVENT_FORM_INTERACTION)
		{
			if (e->Data.Forms.EventFlag == FormEventType::ButtonClick)
			{
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
				else if (cname == "BUTTON_SHOW_ALIEN_INFILTRATION")
				{
					stageCmd.cmd = StageCmd::Command::PUSH;
					stageCmd.nextStage = std::make_shared<InfiltrationScreen>(fw);
					return;
				}
				else if (cname == "BUTTON_SHOW_SCORE")
				{
					stageCmd.cmd = StageCmd::Command::PUSH;
					stageCmd.nextStage = std::make_shared<ScoreScreen>(fw);
					return;
				}
				else if (cname == "BUTTON_SHOW_UFOPAEDIA")
				{
					stageCmd.cmd = StageCmd::Command::PUSH;
					stageCmd.nextStage = std::make_shared<Ufopaedia>(fw);
					return;
				}
				else if (cname == "BUTTON_SHOW_OPTIONS")
				{
					stageCmd.cmd = StageCmd::Command::PUSH;
					stageCmd.nextStage = std::make_shared<OptionsMenu>(fw);
					return;
				}
				else if (cname == "BUTTON_SHOW_LOG")
					LogError("Show log");
				else if (cname == "BUTTON_ZOOM_EVENT")
					LogError("Zoom to event");
				else if (cname == "BUTTON_SPEED0")
					LogError("Set speed 0");
				else if (cname == "BUTTON_SPEED1")
					LogError("Set speed 1");
				else if (cname == "BUTTON_SPEED2")
					LogError("Set speed 2");
				else if (cname == "BUTTON_SPEED3")
					LogError("Set speed 3");
				else if (cname == "BUTTON_SPEED4")
					LogError("Set speed 4");
				else if (cname == "BUTTON_SPEED5")
					LogError("Set speed 5");
					
			}

		}
		else
		{
			TileView::EventOccurred(e);
		}
	}
}

}; //namespace OpenApoc
