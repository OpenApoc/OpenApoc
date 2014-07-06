
#pragma once

#include "../framework/stage.h"
#include "../framework/includes.h"

class TransitionStrips : public Stage
{
	private:
		bool FadeToNewStage;
		Stage* Source;
		Stage* Target;
		int numStrips;
		int frameIndex;
		int frameMax;
		ALLEGRO_BITMAP* SourceScreen;
		ALLEGRO_BITMAP* TargetScreen;
		int stripWidth;
		int slowestSpeed;
		int* speedList;

		void PrepareStrips( int FadeFrames, int NumberOfStrips );

  public:
		TransitionStrips( int FadeFrames, int NumberOfStrips );	// Fade back to previous stack
		TransitionStrips( Stage* FadeInTo, int FadeFrames, int NumberOfStrips );		// Fade into new stage
		~TransitionStrips();

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
