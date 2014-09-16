
#pragma once

#include "../../framework/stage.h"
#include "../../framework/includes.h"
#include "../../library/spritesheet.h"

#include "../resources/gamecore.h"
#include "../apocresources/apocresource.h"
#include "../../forms/forms.h"

class BaseScreen : public Stage
{
	private:
        Form* basescreenform;

  public:
        BaseScreen();
        ~BaseScreen();
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
