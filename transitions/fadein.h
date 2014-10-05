
#pragma once

#include "framework/stage.h"
#include "framework/includes.h"

namespace OpenApoc {

class TransitionFadeIn : public Stage
{
	private:
		int currentFrame;
		int transitionFrames;
		ALLEGRO_COLOR transitionFrom;
		std::shared_ptr<Stage> targetStage;
		ALLEGRO_BITMAP* targetRender;

	public:
		TransitionFadeIn( Framework &fw, std::shared_ptr<Stage> Target, ALLEGRO_COLOR Source, int Frames );
		~TransitionFadeIn();

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
