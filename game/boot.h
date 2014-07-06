
#pragma once

#include "../framework/stage.h"
#include "../framework/includes.h"
#include "../library/spritesheet.h"

#include "apocresources/rawsound.h"

class BootUp : public Stage
{
	private:
		ALLEGRO_BITMAP* ufopediaimg;
		int loadtime;

		void StartGame();

  public:
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
