
#pragma once

#include "../framework/stage.h"
#include "../framework/includes.h"
#include "../library/spritesheet.h"

class TransitionFadeIn : public Stage
{
	private:
		int currentFrame;
		int transitionFrames;
		ALLEGRO_COLOR transitionFrom;
		Stage* targetStage;
		ALLEGRO_BITMAP* targetRender;

		void FinishTransition();

  public:
		TransitionFadeIn( Stage* Target, ALLEGRO_COLOR Source, int Frames );
		~TransitionFadeIn();

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