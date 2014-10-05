
#pragma once

#include "framework/stage.h"
#include "framework/includes.h"

#include "game/resources/gamecore.h"
#include "game/apocresources/apocresource.h"
#include "forms/forms.h"

namespace OpenApoc {

class MainMenu : public Stage
{
	private:
		std::unique_ptr<Music> musicplayer;
		Form* mainmenuform;
		StageCmd stageCmd;

	public:
		MainMenu(Framework &fw);
		~MainMenu();
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
