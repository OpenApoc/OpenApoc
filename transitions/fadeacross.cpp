
#include "fadeacross.h"
#include "../framework/framework.h"

namespace OpenApoc {

TransitionFadeAcross::TransitionFadeAcross( std::shared_ptr<Stage> Target, int Frames )
{
	auto sourceStage = FRAMEWORK->getCurrentStage();
	targetStage = Target;

	transitionFrames = Frames;
	currentFrame = 0;

	sourceRender = al_create_bitmap( FRAMEWORK->Display_GetWidth(), FRAMEWORK->Display_GetHeight() );
	FRAMEWORK->Display_SetTarget( sourceRender );
	sourceStage->Render();
	targetRender = al_create_bitmap( FRAMEWORK->Display_GetWidth(), FRAMEWORK->Display_GetHeight() );
	FRAMEWORK->Display_SetTarget( targetRender );
	targetStage->Render();
	FRAMEWORK->Display_SetTarget();
}

TransitionFadeAcross::~TransitionFadeAcross()
{
	if( targetRender != 0 )
	{
		al_destroy_bitmap( targetRender );
	}
}

void TransitionFadeAcross::Begin()
{

}

void TransitionFadeAcross::Pause()
{
}

void TransitionFadeAcross::Resume()
{
}

void TransitionFadeAcross::Finish()
{
}

void TransitionFadeAcross::EventOccurred(Event *e)
{
}

void TransitionFadeAcross::Update(StageCmd * const cmd)
{
	currentFrame++;
	if( currentFrame >= transitionFrames )
	{
		cmd->cmd = StageCmd::Command::REPLACE;
		cmd->nextStage = this->targetStage;
	}
}

void TransitionFadeAcross::Render()
{

	//targetStage->Render();
	al_draw_bitmap( sourceRender, 0, 0, 0 );

	float alpha = (float)currentFrame / (float)transitionFrames;
	if( alpha > 1.0f )
	{
		alpha = 1.0f;
	}
	al_draw_tinted_bitmap( targetRender, al_map_rgba_f( 1.0f, 1.0f, 1.0f, alpha), 0, 0, 0 );
}

bool TransitionFadeAcross::IsTransition()
{
	return true;
}

}; //namespace OpenApoc
