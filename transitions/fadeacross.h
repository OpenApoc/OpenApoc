
#pragma once

#include "../framework/stage.h"
#include "../framework/includes.h"

class TransitionFadeAcross : public Stage
{
	private:
		int currentFrame;
		int transitionFrames;
		ALLEGRO_BITMAP* sourceRender;
		Stage* targetStage;
		ALLEGRO_BITMAP* targetRender;

		void FinishTransition();

  public:
		TransitionFadeAcross( Stage* Target, int Frames );
		virtual ~TransitionFadeAcross();

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
