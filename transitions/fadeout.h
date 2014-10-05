
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
		std::shared_ptr<Stage> targetStage;
		std::shared_ptr<Stage> sourceStage;
		ALLEGRO_BITMAP* targetRender;

  public:
		TransitionFadeOut( std::shared_ptr<Stage> Target, std::shared_ptr<Stage> SourceStage, ALLEGRO_COLOR Source, int Frames );
		~TransitionFadeOut();

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
