
#pragma once

#include "framework/stage.h"
#include "framework/includes.h"

namespace OpenApoc {

class TransitionFadeOut : public Stage
{
	private:
		int currentFrame;
		int transitionFrames;
		ALLEGRO_COLOR transitionFrom;
		Stage* targetStage;
		ALLEGRO_BITMAP* targetRender;

		void FinishTransition();

  public:
		TransitionFadeOut( Stage* Target, ALLEGRO_COLOR Source, int Frames );
		~TransitionFadeOut();

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

}; //namespace OpenApoc
