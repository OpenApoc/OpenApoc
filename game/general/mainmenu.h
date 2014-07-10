
#pragma once

#include "../../framework/stage.h"
#include "../../framework/includes.h"
#include "../../library/spritesheet.h"

#include "../apocresources/apocfont.h"
#include "../apocresources/language.h"
#include "../apocresources/rawsound.h"
#include "../apocresources/music.h"

class MainMenu : public Stage
{
	private:
		ALLEGRO_BITMAP* ufopediaimg;
		Palette* fontpalette;
		ApocalypseFont* largefont;
		ApocalypseFont* smallfont;
		Language* currentlanguage;
		RawSound* buttonclick;
		Music* musicplayer;


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
