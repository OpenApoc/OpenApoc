
#include "fadein.h"
#include "../framework/framework.h"

namespace OpenApoc {

TransitionFadeIn::TransitionFadeIn( Framework &fw, std::shared_ptr<Stage> Target, ALLEGRO_COLOR Source, int Frames )
	: Stage(fw)
{
	targetStage = Target;
	transitionFrom = Source;
	transitionFrames = Frames;
	currentFrame = transitionFrames;
	targetRender = 0;
}

TransitionFadeIn::~TransitionFadeIn()
{
	if( targetRender != 0 )
	{
		al_destroy_bitmap( targetRender );
	}
}

void TransitionFadeIn::Begin()
{
	targetRender = al_create_bitmap( fw.Display_GetWidth(), fw.Display_GetHeight() );
	fw.Display_SetTarget( targetRender );
	targetStage->Render();
	fw.Display_SetTarget();
}

void TransitionFadeIn::Pause()
{
}

void TransitionFadeIn::Resume()
{
}

void TransitionFadeIn::Finish()
{
}

void TransitionFadeIn::EventOccurred(Event *e)
{
}

void TransitionFadeIn::Update(StageCmd * const cmd)
{
	currentFrame++;
	if( currentFrame >= transitionFrames )
	{
		cmd->cmd = StageCmd::Command::REPLACE;
		cmd->nextStage = this->targetStage;
	}
}

void TransitionFadeIn::Render()
{
	//targetStage->Render();
	al_draw_bitmap( targetRender, 0, 0, 0 );
	transitionFrom.a = (float)currentFrame / (float)transitionFrames;
	if( transitionFrom.a < 0.0f )
	{
		transitionFrom.a = 0.0f;
	}
	al_draw_filled_rectangle( 0, 0, fw.Display_GetWidth(), fw.Display_GetHeight(), transitionFrom );
}

bool TransitionFadeIn::IsTransition()
{
	return true;
}

}; //namespace OpenApoc
