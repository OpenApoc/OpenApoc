
#include "strips.h"
#include "../framework/framework.h"

TransitionStrips::TransitionStrips( int FadeFrames, int NumberOfStrips )
{
	FadeToNewStage = false;
	Source = FRAMEWORK->ProgramStages->Current();	// This stage won't be on the stack when the constructor is called
	Target = FRAMEWORK->ProgramStages->Previous( Source );
	PrepareStrips( FadeFrames, NumberOfStrips );
}

TransitionStrips::TransitionStrips( Stage* FadeInTo, int FadeFrames, int NumberOfStrips )
{
	FadeToNewStage = true;
	Source = FRAMEWORK->ProgramStages->Current();	// This stage won't be on the stack when the constructor is called
	Target = FadeInTo;
	PrepareStrips( FadeFrames, NumberOfStrips );
}

void TransitionStrips::PrepareStrips( int FadeFrames, int NumberOfStrips )
{
	numStrips = NumberOfStrips;
	frameMax = FadeFrames;
	stripWidth = FRAMEWORK->Display_GetWidth() / NumberOfStrips;
	if( stripWidth * NumberOfStrips < FRAMEWORK->Display_GetWidth() )	// Correct any rounding issues
	{
		stripWidth++;
	}
	frameIndex = 0;
	slowestSpeed = FRAMEWORK->Display_GetHeight() / FadeFrames;
	while( (float)slowestSpeed < FRAMEWORK->Display_GetHeight() / (float)FadeFrames )
	{
		slowestSpeed++;
	}
	SourceScreen = al_create_bitmap( FRAMEWORK->Display_GetWidth(), FRAMEWORK->Display_GetHeight() );
	FRAMEWORK->Display_SetTarget( SourceScreen );
	Source->Render();
	TargetScreen = al_create_bitmap( FRAMEWORK->Display_GetWidth(), FRAMEWORK->Display_GetHeight() );
	FRAMEWORK->Display_SetTarget( TargetScreen );
	Target->Render();
	FRAMEWORK->Display_SetTarget();

	speedList = (int*)malloc( sizeof(int) * NumberOfStrips );
	for( int i = 0; i < NumberOfStrips; i++ )
	{
		speedList[i] = slowestSpeed + (rand() % slowestSpeed);
		if( speedList[i] == 0 )
		{
			speedList[i] = 1;
		}
	}
}

TransitionStrips::~TransitionStrips()
{
	free( (void*)speedList );
	if( SourceScreen != 0 )
	{
		al_destroy_bitmap( SourceScreen );
	}
	if( TargetScreen != 0 )
	{
		al_destroy_bitmap( TargetScreen );
	}
}

void TransitionStrips::Begin()
{
}

void TransitionStrips::Pause()
{
}

void TransitionStrips::Resume()
{
}

void TransitionStrips::Finish()
{
}

void TransitionStrips::EventOccurred(Event *e)
{
}

void TransitionStrips::Update()
{
	frameIndex++;
	if( frameIndex > frameMax )
	{
		Stage* t = Target;
		if( FadeToNewStage )
		{
			// Swap to new stage
			delete FRAMEWORK->ProgramStages->Pop();		// Remove transition stage (this)
			FRAMEWORK->ProgramStages->Push( t );
		} else {
			// Pop off back to stage
			while( FRAMEWORK->ProgramStages->Current() != t )
			{
				delete FRAMEWORK->ProgramStages->Pop();
			}
		}
	}
}

void TransitionStrips::Render()
{
	// Draw original source
	al_draw_bitmap( SourceScreen, 0, 0, 0 );

	al_hold_bitmap_drawing( true );

	// Draw strips
	for( int i = 0; i < numStrips; i++ )
	{
		al_draw_bitmap_region( TargetScreen, i * stripWidth, 0, stripWidth, speedList[i] * frameIndex, i * stripWidth, 0, 0 );
	}
	
	al_hold_bitmap_drawing( false );
}

bool TransitionStrips::IsTransition()
{
	return true;
}
