#include "library/sp.h"

#include "game/debugtools/debugmenu.h"
#include "framework/framework.h"
#include "game/debugtools/formpreview.h"

namespace OpenApoc
{

DebugMenu::DebugMenu(Framework &fw) : Stage(fw), menuform(fw.gamecore->GetForm("FORM_DEBUG_MENU"))
{
}

DebugMenu::~DebugMenu() {}

void DebugMenu::Begin() {}

void DebugMenu::Pause() {}

void DebugMenu::Resume() {}

void DebugMenu::Finish() {}

void DebugMenu::EventOccurred(Event *e)
{
	menuform->EventOccured(e);
	fw.gamecore->MouseCursor->EventOccured(e);

	if (e->Type == EVENT_KEY_DOWN)
	{
		if (e->Data.Keyboard.KeyCode == SDLK_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}

	if (e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.EventFlag == FormEventType::ButtonClick)
	{
		if (e->Data.Forms.RaisedBy->Name == "BUTTON_QUIT")
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
		else if (e->Data.Forms.RaisedBy->Name == "BUTTON_DUMPPCK")
		{
			BulkExportPCKs();
			return;
		}
		else if (e->Data.Forms.RaisedBy->Name == "BUTTON_FORMPREVIEW")
		{
			stageCmd.cmd = StageCmd::Command::PUSH;
			stageCmd.nextStage = std::make_shared<FormPreview>(fw);
		}
	}
}

void DebugMenu::Update(StageCmd *const cmd)
{
	menuform->Update();
	*cmd = this->stageCmd;
	// Reset the command to default
	this->stageCmd = StageCmd();
}

void DebugMenu::Render()
{
	fw.Stage_GetPrevious(this->shared_from_this())->Render();
	fw.renderer->drawFilledRect(Vec2<float>(0, 0),
	                            Vec2<float>(fw.Display_GetWidth(), fw.Display_GetHeight()),
	                            Colour(0, 0, 0, 128));
	menuform->Render();
	fw.gamecore->MouseCursor->Render();
}

bool DebugMenu::IsTransition() { return false; }

void DebugMenu::BulkExportPCKs()
{
	std::vector<UString> PaletteNames;
	std::vector<sp<Palette>> PaletteList;
	std::vector<UString> PckNames;

	// All the palettes
	PaletteNames.push_back("XCOM3/UFODATA/PAL_01.DAT");
	PaletteNames.push_back("XCOM3/UFODATA/PAL_02.DAT");
	PaletteNames.push_back("XCOM3/UFODATA/PAL_03.DAT");
	PaletteNames.push_back("XCOM3/UFODATA/PAL_04.DAT");
	PaletteNames.push_back("XCOM3/UFODATA/PAL_05.DAT");
	PaletteNames.push_back("XCOM3/UFODATA/PAL_06.DAT");
	PaletteNames.push_back("XCOM3/UFODATA/PAL_99.DAT");
	// PaletteNames.push_back("XCOM3/TACDATA/DEFAULT.PAL");
	// PaletteNames.push_back("XCOM3/TACDATA/EQUIP.PAL");
	// PaletteNames.push_back("XCOM3/TACDATA/TACTICAL.PAL");

	// Load them up
	for (auto i = PaletteNames.begin(); i != PaletteNames.end(); i++)
	{
		UString palname = (*i);
		PaletteList.push_back(fw.data->load_palette(palname));
	}

	// All the PCKs
	PckNames.push_back("XCOM3/UFODATA/AGNTICO.PCK");
	PckNames.push_back("XCOM3/UFODATA/ALIEN.PCK");
	PckNames.push_back("XCOM3/UFODATA/ALIEN_S.PCK");
	PckNames.push_back("XCOM3/UFODATA/ARMOUR.PCK");
	PckNames.push_back("XCOM3/UFODATA/BASE.PCK");
	PckNames.push_back("XCOM3/UFODATA/BIGVEH.PCK");
	PckNames.push_back("XCOM3/UFODATA/CITY.PCK");
	PckNames.push_back("XCOM3/UFODATA/CITYOVR.PCK");
	PckNames.push_back("XCOM3/UFODATA/CONTICO.PCK");
	PckNames.push_back("XCOM3/UFODATA/DESCURS.PCK");
	PckNames.push_back("XCOM3/UFODATA/ICONS.PCK");
	PckNames.push_back("XCOM3/UFODATA/ICON_M.PCK");
	PckNames.push_back("XCOM3/UFODATA/ICON_S.PCK");
	PckNames.push_back("XCOM3/UFODATA/NEWBUT.PCK");
	// PckNames.push_back("XCOM3/UFODATA/OVER-A.PCK");
	// PckNames.push_back("XCOM3/UFODATA/OVER-B.PCK");
	// PckNames.push_back("XCOM3/UFODATA/OVER-S.PCK");
	PckNames.push_back("XCOM3/UFODATA/PED-BUT.PCK");
	PckNames.push_back("XCOM3/UFODATA/PEQUIP.PCK");
	PckNames.push_back("XCOM3/UFODATA/PHOTO.PCK");
	PckNames.push_back("XCOM3/UFODATA/PTANG.PCK");
	PckNames.push_back("XCOM3/UFODATA/SAUCER.PCK");
	// PckNames.push_back("XCOM3/UFODATA/SHADOW.PCK");
	PckNames.push_back("XCOM3/UFODATA/SMALVEH.PCK");
	// PckNames.push_back("XCOM3/UFODATA/STRATMAP.PCK");
	PckNames.push_back("XCOM3/UFODATA/VEHEQUIP.PCK");
	PckNames.push_back("XCOM3/UFODATA/VEHICLE.PCK");
	PckNames.push_back("XCOM3/UFODATA/VS_ICON.PCK");
	PckNames.push_back("XCOM3/UFODATA/VS_OBS.PCK");

	// Load them up
	for (auto i = PckNames.begin(); i != PckNames.end(); i++)
	{
		UString pckname = (*i);
		UString pckloadstr = UString("PCK:") + pckname + UString(":") +
		                     pckname.substr(0, pckname.length() - 3) + UString("TAB");

		LogInfo(UString("Processing ") + pckloadstr);

		sp<ImageSet> pckset = fw.data->load_image_set(pckloadstr);

		if (pckset != nullptr)
		{
			// PckList.push_back(pckset);

			for (unsigned int idx = 0; idx < pckset->images.size(); idx++)
			{
				UString outputname = UString("Extracted/") + pckname + UString("/") +
				                     Strings::FromInteger(idx) + UString(".PNG");
				sp<Image> curimg = pckset->images.at(idx);

				if (RGBImage *bi = dynamic_cast<RGBImage *>(curimg.get()))
				{

					LogInfo(UString("Saving ") + outputname);
					bi->saveBitmap(outputname);
				}
				else if (PaletteImage *pi = dynamic_cast<PaletteImage *>(curimg.get()))
				{

					for (unsigned int palidx = 0; palidx < PaletteList.size(); palidx++)
					{
						outputname = UString("Extracted/") + pckname + UString("/") +
						             Strings::FromInteger(idx) + UString(".#") +
						             Strings::FromInteger(palidx) + UString(".PNG");
						LogInfo(UString("Saving ") + outputname);
						pi->toRGBImage(PaletteList.at(palidx))->saveBitmap(outputname);
					}
				}
			}
		}
	}
}

}; // namespace OpenApoc
