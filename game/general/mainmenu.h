
#pragma once

#include "../../framework/stage.h"
#include "../../framework/includes.h"
#include "../../library/spritesheet.h"

#include "../resources/gamecore.h"
#include "../apocresources/apocresource.h"
#include "../../forms/forms.h"

class MainMenu : public Stage
{
	private:
		ALLEGRO_BITMAP* emptybackground;
		ALLEGRO_BITMAP* buttonimage;
		Palette* fontpalette;
		ApocalypseFont* largefont;
		ApocalypseFont* smallfont;
		Music* musicplayer;

		Cursor* mousecursor;

		PCK* testpck;

		Form* testform;


  public:
		MainMenu();
		~MainMenu();
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
