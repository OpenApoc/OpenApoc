#include "game/ui/debugtools/debugmenu.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/ui/debugtools/formpreview.h"
#include "game/ui/debugtools/imagepreview.h"
#include "library/sp.h"

namespace OpenApoc
{

DebugMenu::DebugMenu() : Stage(), menuform(ui().getForm("FORM_DEBUG_MENU")) {}

DebugMenu::~DebugMenu() = default;

void DebugMenu::begin() {}

void DebugMenu::pause() {}

void DebugMenu::resume() {}

void DebugMenu::finish() {}

void DebugMenu::eventOccurred(Event *e)
{
	menuform->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE)
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
			return;
		}
	}

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().EventFlag == FormEventType::ButtonClick)
	{
		if (e->forms().RaisedBy->Name == "BUTTON_QUIT")
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
			return;
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_DUMPPCK")
		{
			bulkExportPcks();
			return;
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_FORMPREVIEW")
		{
			fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<FormPreview>()});
		}
		else if (e->forms().RaisedBy->Name == "BUTTON_IMAGEPREVIEW")
		{
			fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<ImagePreview>()});
		}
	}
}

void DebugMenu::update() { menuform->update(); }

void DebugMenu::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	fw().renderer->drawFilledRect(Vec2<float>(0, 0),
	                              Vec2<float>(fw().displayGetWidth(), fw().displayGetHeight()),
	                              Colour(0, 0, 0, 128));
	menuform->render();
}

bool DebugMenu::isTransition() { return false; }

void DebugMenu::bulkExportPcks()
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
		PaletteList.push_back(fw().data->loadPalette(palname));
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

		LogInfo("Processing %s", pckloadstr.cStr());

		sp<ImageSet> pckset = fw().data->loadImageSet(pckloadstr);

		if (pckset != nullptr)
		{
			// PckList.push_back(pckset);

			for (unsigned int idx = 0; idx < pckset->images.size(); idx++)
			{
				UString outputname = UString("Extracted/") + pckname + UString("/") +
				                     Strings::fromInteger(idx) + UString(".PNG");
				sp<Image> curimg = pckset->images.at(idx);

				if (RGBImage *bi = dynamic_cast<RGBImage *>(curimg.get()))
				{

					LogInfo("Saving %s", outputname.cStr());
					bi->saveBitmap(outputname);
				}
				else if (PaletteImage *pi = dynamic_cast<PaletteImage *>(curimg.get()))
				{

					for (unsigned int palidx = 0; palidx < PaletteList.size(); palidx++)
					{
						outputname = UString("Extracted/") + pckname + UString("/") +
						             Strings::fromInteger(idx) + UString(".#") +
						             Strings::fromInteger(palidx) + UString(".PNG");
						LogInfo("Saving %s", outputname.cStr());
						pi->toRGBImage(PaletteList.at(palidx))->saveBitmap(outputname);
					}
				}
			}
		}
	}
}

}; // namespace OpenApoc
