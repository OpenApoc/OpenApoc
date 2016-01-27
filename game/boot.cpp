
#include "game/boot.h"
#include "framework/framework.h"
#include "framework/trace.h"
#include "game/general/mainmenu.h"
#include "game/resources/gamecore.h"
#include <tuple>

namespace OpenApoc
{

static void CreateGameCore(std::atomic<bool> *isComplete)
{
	TRACE_FN;

	try
	{

		UString ruleset = fw().Settings->getString("GameRules");

		fw().gamecore.reset(new GameCore());

		fw().gamecore->Load(ruleset);
		*isComplete = true;
	}
	catch (std::exception &e)
	{
		LogError("Exception creating GameCore: %s", e.what());
	}
}

void BootUp::Begin()
{
	loadingimage = fw().data->load_image("UI/LOADING.PNG");
	logoimage = fw().data->load_image("UI/LOGO.PNG");
	loadtime = 0;
	fw().Display_SetIcon();

	this->gamecoreLoadComplete = false;
	this->asyncGamecoreLoad = fw().threadPool->enqueue(CreateGameCore, &this->gamecoreLoadComplete);
}

void BootUp::Pause() {}

void BootUp::Resume() {}

void BootUp::Finish() {}

void BootUp::EventOccurred(Event *e) { std::ignore = e; }

void BootUp::Update(StageCmd *const cmd)
{
	loadtime++;
	loadingimageangle.Add(5);

	if (gamecoreLoadComplete)
	{
		asyncGamecoreLoad.wait();
		cmd->cmd = StageCmd::Command::REPLACE;
		cmd->nextStage = mksp<MainMenu>();
	}
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
	    loadingimageangle.ToRadians());
}

bool BootUp::IsTransition() { return false; }

}; // namespace OpenApoc
