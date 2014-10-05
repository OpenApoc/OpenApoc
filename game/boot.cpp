
#include "boot.h"
#include "../framework/framework.h"
#include "general/mainmenu.h"
#include "../transitions/transitions.h"
#include "resources/gamecore.h"

namespace OpenApoc {

void BootUp::Begin()
{
	loadingimage = fw.data.load_image( "UI/LOADING.PNG" );
	loadtime = 0;
	fw.Display_SetTitle("OpenApocalypse");

	threadload = nullptr;
	threadload = al_create_thread( CreateGameCore, &fw );
	if( threadload != nullptr )
	{
		al_start_thread( threadload );
	}
}

void BootUp::Pause()
{
}

void BootUp::Resume()
{
}

void BootUp::Finish()
{
}

void BootUp::EventOccurred(Event *e)
{
}

void BootUp::Update(StageCmd * const cmd)
{
	loadtime++;
	loadingimageangle.Add( 5 );

	if( threadload == nullptr && fw.gamecore == nullptr )
	{
		CreateGameCore( nullptr, &fw );
	}

	if( fw.gamecore != nullptr && fw.gamecore->Loaded && loadtime > FRAMES_PER_SECOND * 2 )
	{
		StartGame();
		cmd->cmd = StageCmd::Command::REPLACE;
		cmd->nextStage = std::make_shared<TransitionFadeIn>(fw, std::make_shared<MainMenu>(fw), al_map_rgb(0,0,0), FRAMES_PER_SECOND);
	}
}

void BootUp::Render()
{
	al_clear_to_color( al_map_rgb( 0, 0, 0 ) );
	loadingimage->drawRotated( 24, 24, fw.Display_GetWidth() - 50, fw.Display_GetHeight() - 50, loadingimageangle.ToRadians() );
}

void BootUp::StartGame()
{
	if( threadload != nullptr )
	{
		al_destroy_thread( threadload );
	}
}

bool BootUp::IsTransition()
{
	return false;
}

void* BootUp::CreateGameCore(ALLEGRO_THREAD* thread, void* args)
{
	Framework *fw = (Framework*)args;
	std::string ruleset = fw->Settings->GetQuickStringValue( "GameRules", "XCOMAPOC.XML" );
	std::string language = fw->Settings->GetQuickStringValue( "Language", "en_gb" );

	fw->gamecore.reset(new GameCore(*fw));

	fw->gamecore->Load(ruleset, language);

	return nullptr;
}

}; //namespace OpenApoc
