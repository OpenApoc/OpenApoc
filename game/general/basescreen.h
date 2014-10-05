
#pragma once

#include "framework/stage.h"
#include "framework/includes.h"

#include "game/resources/gamecore.h"
#include "game/apocresources/apocresource.h"
#include "forms/forms.h"

namespace OpenApoc {

class BaseScreen : public Stage
{
	private:
        Form* basescreenform;
		StageCmd stageCmd;

  public:
        BaseScreen();
        ~BaseScreen();
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
