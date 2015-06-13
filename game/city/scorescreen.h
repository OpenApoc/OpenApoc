
#pragma once

#include "framework/stage.h"
#include "framework/includes.h"

#include "game/resources/gamecore.h"
#include "game/apocresources/apocresource.h"
#include "forms/forms.h"

namespace OpenApoc {

class ScoreScreen : public Stage
{
	private:
		Form* menuform;
		StageCmd stageCmd;


	public:
		ScoreScreen(Framework &fw);
		~ScoreScreen();
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
