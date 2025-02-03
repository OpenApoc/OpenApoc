#include "game/ui/debugtools/debugmenu.h"
#include "forms/form.h"
#include "forms/ui.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "framework/keycodes.h"
#include "framework/logger.h"
#include "framework/renderer.h"
#include "game/ui/debugtools/formpreview.h"
#include "game/ui/debugtools/imagepreview.h"
#include "library/sp.h"

namespace OpenApoc
{

DebugMenu::DebugMenu() : Stage(), menuform(ui().getForm("debugmenu")) {}

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
	PaletteNames.push_back("xcom3/ufodata/pal_01.dat");
	PaletteNames.push_back("xcom3/ufodata/pal_02.dat");
	PaletteNames.push_back("xcom3/ufodata/pal_03.dat");
	PaletteNames.push_back("xcom3/ufodata/pal_04.dat");
	PaletteNames.push_back("xcom3/ufodata/pal_05.dat");
	PaletteNames.push_back("xcom3/ufodata/pal_06.dat");
	PaletteNames.push_back("xcom3/ufodata/pal_99.dat");
	// PaletteNames.push_back("xcom3/tacdata/default.pal");
	// PaletteNames.push_back("xcom3/tacdata/equip.pal");
	// PaletteNames.push_back("xcom3/tacdata/tactical.pal");

	// Load them up
	for (auto i = PaletteNames.begin(); i != PaletteNames.end(); i++)
	{
		UString palname = (*i);
		PaletteList.push_back(fw().data->loadPalette(palname));
	}

	// All the PCKs
	PckNames.push_back("xcom3/ufodata/agntico.pck");
	PckNames.push_back("xcom3/ufodata/alien.pck");
	PckNames.push_back("xcom3/ufodata/alien_s.pck");
	PckNames.push_back("xcom3/ufodata/armour.pck");
	PckNames.push_back("xcom3/ufodata/base.pck");
	PckNames.push_back("xcom3/ufodata/bigveh.pck");
	PckNames.push_back("xcom3/ufodata/city.pck");
	PckNames.push_back("xcom3/ufodata/cityovr.pck");
	PckNames.push_back("xcom3/ufodata/contico.pck");
	PckNames.push_back("xcom3/ufodata/descurs.pck");
	PckNames.push_back("xcom3/ufodata/icons.pck");
	PckNames.push_back("xcom3/ufodata/icon_m.pck");
	PckNames.push_back("xcom3/ufodata/icon_s.pck");
	PckNames.push_back("xcom3/ufodata/newbut.pck");
	// PckNames.push_back("xcom3/ufodata/over-a.pck");
	// PckNames.push_back("xcom3/ufodata/over-b.pck");
	// PckNames.push_back("xcom3/ufodata/over-s.pck");
	PckNames.push_back("xcom3/ufodata/ped-but.pck");
	PckNames.push_back("xcom3/ufodata/pequip.pck");
	PckNames.push_back("xcom3/ufodata/photo.pck");
	PckNames.push_back("xcom3/ufodata/ptang.pck");
	PckNames.push_back("xcom3/ufodata/saucer.pck");
	// PckNames.push_back("xcom3/ufodata/shadow.pck");
	PckNames.push_back("xcom3/ufodata/smalveh.pck");
	// PckNames.push_back("xcom3/ufodata/stratmap.pck");
	PckNames.push_back("xcom3/ufodata/vehequip.pck");
	PckNames.push_back("xcom3/ufodata/vehicle.pck");
	PckNames.push_back("xcom3/ufodata/vs_icon.pck");
	PckNames.push_back("xcom3/ufodata/vs_obs.pck");

	// Load them up
	for (auto i = PckNames.begin(); i != PckNames.end(); i++)
	{
		UString pckname = (*i);
		UString pckloadstr = UString("PCK:") + pckname + UString(":") +
		                     pckname.substr(0, pckname.length() - 3) + UString("tab");

		LogInfo("Processing {}", pckloadstr);

		sp<ImageSet> pckset = fw().data->loadImageSet(pckloadstr);

		if (pckset != nullptr)
		{
			// PckList.push_back(pckset);

			for (unsigned int idx = 0; idx < pckset->images.size(); idx++)
			{
				UString outputname = UString("Extracted/") + pckname + UString("/") +
				                     Strings::fromInteger(idx) + UString(".png");
				sp<Image> curimg = pckset->images.at(idx);

				if (sp<RGBImage> bi = std::dynamic_pointer_cast<RGBImage>(curimg))
				{

					LogInfo("Saving {}", outputname);
					fw().data->writeImage(outputname, bi);
				}
				else if (sp<PaletteImage> pi = std::dynamic_pointer_cast<PaletteImage>(curimg))
				{

					for (unsigned int palidx = 0; palidx < PaletteList.size(); palidx++)
					{
						outputname = UString("extracted/") + pckname + UString("/") +
						             Strings::fromInteger(idx) + UString(".#") +
						             Strings::fromInteger(palidx) + UString(".png");
						LogInfo("Saving {}", outputname);
						fw().data->writeImage(outputname, pi, PaletteList.at(palidx));
					}
				}
			}
		}
	}
}

}; // namespace OpenApoc
