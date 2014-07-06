
#pragma once

#include "../framework/stage.h"
#include "../framework/includes.h"

namespace TiledTransitions
{
	enum Transition
	{
		LEFT_TO_RIGHT = 0,
		TOP_TO_BOTTOM,
		NORTHWEST_TO_SOUTHEAST,
		SPIRAL_INWARDS
	};
};

class TransitionTiled : public Stage
{
	private:
		bool FadeToNewStage;
		Stage* Source;
		Stage* Target;
		int frameIndex;
		int frameMax;
		ALLEGRO_BITMAP* SourceScreen;
		ALLEGRO_BITMAP* TargetScreen;

		TiledTransitions::Transition fadeStyle;
		int tileWide;
		int tileHigh;
		int tileWidth;
		int tileHeight;
		char* tileMap;

		int spiralLastX;
		int spiralLastY;
		int spiralLastDirection;

		void PrepareTiles( TiledTransitions::Transition Style, int TilesWide, int TilesHigh );
		void LeaveTransition();

  public:
		TransitionTiled( TiledTransitions::Transition Style, int TilesWide, int TilesHigh );	// Fade back to previous stack
		TransitionTiled( Stage* FadeInTo, TiledTransitions::Transition Style, int TilesWide, int TilesHigh );		// Fade into new stage
		~TransitionTiled();

    // Stage control
    virtual void Begin();
    virtual void Pause();
    virtual void Resume();
    virtual void Finish();
    virtual void EventOccurred(Event *e);
    virtual void Update();
    virtual void Render();
		virtual bool IsTransition();
};
