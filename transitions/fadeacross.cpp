
#include "fadeacross.h"
#include "../framework/framework.h"

TransitionFadeAcross::TransitionFadeAcross( Stage* Target, int Frames )
{
	Stage* sourceStage = FRAMEWORK->ProgramStages->Current();
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
	if( e->Type == EVENT_KEY_DOWN || e->Type == EVENT_MOUSE_DOWN )
	{
		//currentFrame--;
		FinishTransition();
	}
}

void TransitionFadeAcross::Update()
{
	currentFrame++;
	if( currentFrame >= transitionFrames )
	{
		FinishTransition();
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

void TransitionFadeAcross::FinishTransition()
{
	Stage* t = targetStage;
	delete Framework::System->ProgramStages->Pop();
	Framework::System->ProgramStages->Push( t );
}

bool TransitionFadeAcross::IsTransition()
{
	return true;
}
