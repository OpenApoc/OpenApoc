
#include "fadeout.h"
#include "../framework/framework.h"

namespace OpenApoc {

TransitionFadeOut::TransitionFadeOut( Framework &fw, std::shared_ptr<Stage> Target, std::shared_ptr<Stage> SourceStage, ALLEGRO_COLOR Source, int Frames)
	: Stage(fw)
{
	sourceStage = SourceStage;
	targetStage = Target;
	transitionFrom = Source;
	transitionFrames = Frames;
	currentFrame = 0;
	targetRender = 0;
}

TransitionFadeOut::~TransitionFadeOut()
{
	if( targetRender != 0 )
	{
		al_destroy_bitmap( targetRender );
	}
}

void TransitionFadeOut::Begin()
{
	targetRender = al_create_bitmap( fw.Display_GetWidth(), fw.Display_GetHeight() );
	fw.Display_SetTarget( targetRender );
	sourceStage->Render();
	fw.Display_SetTarget();
}

void TransitionFadeOut::Pause()
{
}

void TransitionFadeOut::Resume()
{
}

void TransitionFadeOut::Finish()
{
}

void TransitionFadeOut::EventOccurred(Event *e)
{
}

void TransitionFadeOut::Update(StageCmd * const cmd)
{
	currentFrame++;
	if( currentFrame >= transitionFrames )
	{
		cmd->cmd = StageCmd::Command::REPLACE;
		cmd->nextStage = this->targetStage;
	}
}

void TransitionFadeOut::Render()
{
	//targetStage->Render();
	al_draw_bitmap( targetRender, 0, 0, 0 );
	transitionFrom.a = (float)currentFrame / (float)transitionFrames;
	if( transitionFrom.a > 1.0f )
	{
		transitionFrom.a = 1.0f;
	}
	al_draw_filled_rectangle( 0, 0, fw.Display_GetWidth(), fw.Display_GetHeight(), transitionFrom );
}

bool TransitionFadeOut::IsTransition()
{
	return true;
}

}; //namespace OpenApoc
