#include "game/ui/boot.h"
#include "framework/framework.h"
#include "game/ui/general/mainmenu.h"

namespace OpenApoc
{

void BootUp::Begin()
{
	// FIXME: This is now useless, as it doesn't actually load anything interesting here
	loadingimage = fw().data->load_image("UI/LOADING.PNG");
	logoimage = fw().data->load_image("UI/LOGO.PNG");
	loadtime = 0;
	fw().Display_SetIcon();
	loadingimageangle = 0;
}

void BootUp::Pause() {}

void BootUp::Resume() {}

void BootUp::Finish() {}

void BootUp::EventOccurred(Event *e) { std::ignore = e; }

void BootUp::Update(StageCmd *const cmd)
{
	loadtime++;
	loadingimageangle += (M_PI + 0.05f);
	if (loadingimageangle >= M_PI * 2.0f)
		loadingimageangle -= M_PI * 2.0f;

	cmd->cmd = StageCmd::Command::REPLACE;
	cmd->nextStage = mksp<MainMenu>();
}

void BootUp::Render()
{
	int logow = fw().Display_GetWidth() / 3;
	float logosc = logow / static_cast<float>(logoimage->size.x);

	Vec2<float> logoPosition{fw().Display_GetWidth() / 2 - (logoimage->size.x * logosc / 2),
	                         fw().Display_GetHeight() / 2 - (logoimage->size.y * logosc / 2)};
	Vec2<float> logoSize{logoimage->size.x * logosc, logoimage->size.y * logosc};

	fw().renderer->drawScaled(logoimage, logoPosition, logoSize);

	fw().renderer->drawRotated(
	    loadingimage, Vec2<float>{24, 24},
	    Vec2<float>{fw().Display_GetWidth() - 50, fw().Display_GetHeight() - 50},
	    loadingimageangle);
}

bool BootUp::IsTransition() { return false; }

}; // namespace OpenApoc
