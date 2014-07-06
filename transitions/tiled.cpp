
#include "tiled.h"
#include "../framework/framework.h"

TransitionTiled::TransitionTiled( TiledTransitions::Transition Style, int TilesWide, int TilesHigh )
{
	FadeToNewStage = false;
	Source = FRAMEWORK->ProgramStages->Current();	// This stage won't be on the stack when the constructor is called
	Target = FRAMEWORK->ProgramStages->Previous( Source );
	PrepareTiles( Style, TilesWide, TilesHigh );
}

TransitionTiled::TransitionTiled( Stage* FadeInTo, TiledTransitions::Transition Style, int TilesWide, int TilesHigh )
{
	FadeToNewStage = true;
	Source = FRAMEWORK->ProgramStages->Current();	// This stage won't be on the stack when the constructor is called
	Target = FadeInTo;
	Target->Begin();
	Target->Pause();
	PrepareTiles( Style, TilesWide, TilesHigh );
}

void TransitionTiled::PrepareTiles( TiledTransitions::Transition Style, int TilesWide, int TilesHigh )
{
	frameIndex = 0;
	tileWide = TilesWide;
	tileWidth = FRAMEWORK->Display_GetWidth() / TilesWide;
	if( (tileWidth * TilesWide) < FRAMEWORK->Display_GetWidth() )
	{
		tileWidth++;	// Fix rounding
	}
	tileHigh = TilesHigh;
	tileHeight = FRAMEWORK->Display_GetHeight() / TilesHigh;
	if( (tileHeight * TilesHigh) < FRAMEWORK->Display_GetHeight() )
	{
		tileHeight++;	// Fix rounding
	}

	fadeStyle = Style;
	switch( fadeStyle )
	{
		case TiledTransitions::SPIRAL_INWARDS:
			spiralLastX = -1;
			spiralLastY = 0;
			spiralLastDirection = 0;
			break;
	}

	// Cache screens
	SourceScreen = al_create_bitmap( FRAMEWORK->Display_GetWidth(), FRAMEWORK->Display_GetHeight() );
	FRAMEWORK->Display_SetTarget( SourceScreen );
	Source->Render();
	TargetScreen = al_create_bitmap( FRAMEWORK->Display_GetWidth(), FRAMEWORK->Display_GetHeight() );
	FRAMEWORK->Display_SetTarget( TargetScreen );
	Target->Render();
	FRAMEWORK->Display_SetTarget();

	tileMap = (char*)malloc( sizeof(char) * tileWide * tileHigh );
	for( int i = 0; i < tileWide * tileHigh; i++ )
	{
		tileMap[i] = 0;
	}

}

void TransitionTiled::LeaveTransition()
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

TransitionTiled::~TransitionTiled()
{
	free( (void*)tileMap );
	if( SourceScreen != 0 )
	{
		al_destroy_bitmap( SourceScreen );
	}
	if( TargetScreen != 0 )
	{
		al_destroy_bitmap( TargetScreen );
	}
}

void TransitionTiled::Begin()
{
}

void TransitionTiled::Pause()
{
}

void TransitionTiled::Resume()
{
}

void TransitionTiled::Finish()
{
}

void TransitionTiled::EventOccurred(Event *e)
{
}

void TransitionTiled::Update()
{
	frameIndex++;
	switch( fadeStyle )
	{
		case TiledTransitions::LEFT_TO_RIGHT:
			if( frameIndex >= tileWide )
			{
				LeaveTransition();
			} else {
				for( int y = 0; y < tileHigh; y++ )
				{
					tileMap[(y * tileWide) + (frameIndex - 1)] = 1;
				}
			}
			break;
		case TiledTransitions::TOP_TO_BOTTOM:
			if( frameIndex >= tileHigh )
			{
				LeaveTransition();
			} else {
				for( int x = 0; x < tileWide; x++ )
				{
					tileMap[((frameIndex - 1) * tileWide) + x] = 1;
				}
			}
			break;
		case TiledTransitions::NORTHWEST_TO_SOUTHEAST:
			if( frameIndex >= tileWide + tileHigh )
			{
				LeaveTransition();
			} else {
				for( int y = 0; y < tileHigh; y++ )
				{
					int x = frameIndex - 1 - y;
					if( x >= 0 && x < tileWide )
					{
						tileMap[(y * tileWide) + x] = 1;
					}
				}
			}
			break;
		case TiledTransitions::SPIRAL_INWARDS:
			switch( spiralLastDirection )
			{
				case 0:
					if( spiralLastX + 1 < tileWide && tileMap[(spiralLastY * tileWide) + (spiralLastX + 1)] == 0 )
					{
						spiralLastX++;
					} else {
						spiralLastY++;
						spiralLastDirection++;
					}
					break;
				case 1:
					if( spiralLastY + 1 < tileHigh && tileMap[((spiralLastY + 1) * tileWide) + spiralLastX] == 0 )
					{
						spiralLastY++;
					} else {
						spiralLastX--;
						spiralLastDirection++;
					}
					break;
				case 2:
					if( spiralLastX - 1 >= 0 && tileMap[(spiralLastY * tileWide) + (spiralLastX - 1)] == 0 )
					{
						spiralLastX--;
					} else {
						spiralLastY--;
						spiralLastDirection++;
					}
					break;
				case 3:
					if( spiralLastY - 1 >= 0 && tileMap[((spiralLastY - 1) * tileWide) + spiralLastX] == 0 )
					{
						spiralLastY--;
					} else {
						spiralLastX++;
						spiralLastDirection = 0;
					}
					break;
			}
			if( tileMap[(spiralLastY * tileWide) + spiralLastX] == 0 )
			{
				tileMap[(spiralLastY * tileWide) + spiralLastX] = 1;
			} else {
				LeaveTransition();
			}
			break;
	}
}

void TransitionTiled::Render()
{
	// Draw original source
	al_draw_bitmap( SourceScreen, 0, 0, 0 );

	al_hold_bitmap_drawing( true );

	// Draw tilemap
	for( int y = 0; y < tileHigh; y++ )
	{
		for( int x = 0; x < tileWide; x++ )
		{
			if( tileMap[(y * tileWide) + x] != 0 )
			{
				al_draw_bitmap_region( TargetScreen, x * tileWidth, y * tileHeight, tileWidth, tileHeight, x * tileWidth, y * tileHeight, 0 );
			}
		}
	}
	
	al_hold_bitmap_drawing( false );
}

bool TransitionTiled::IsTransition()
{
	return true;
}
