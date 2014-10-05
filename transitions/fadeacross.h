
#pragma once

#include "framework/stage.h"
#include "framework/includes.h"

namespace OpenApoc {

class TransitionFadeAcross : public Stage
{
	private:
		int currentFrame;
		int transitionFrames;
		ALLEGRO_BITMAP* sourceRender;
		std::shared_ptr<Stage> targetStage;
		ALLEGRO_BITMAP* targetRender;

  public:
		TransitionFadeAcross( std::shared_ptr<Stage> Target, int Frames );
		virtual ~TransitionFadeAcross();

    // Stage control
    virtual void Begin();
    virtual void Pause();
    virtual void Resume();
    virtual void Finish();
    virtual void EventOccurred(Event *e);
    virtual void Update(StageCmd * const cmd);
    virtual void Render();
		virtual bool IsTransition();
};

}; //namespace OpenApoc
