#include "game/city/city.h"
#include "game/city/cityview.h"
#include "framework/framework.h"
#include "game/resources/gamecore.h"

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
			if (e->Data.Forms.EventFlag == FormEventType::MouseDown)
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
			}

		}
		else
		{
			TileView::EventOccurred(e);
		}
	}
}

}; //namespace OpenApoc
