
#pragma once

#include "framework/stage.h"
#include "framework/includes.h"

#include "game/resources/gamecore.h"
#include "game/apocresources/apocresource.h"
#include "forms/forms.h"

#include "ufopaediacategory.h"

namespace OpenApoc {

class Ufopaedia : public Stage
{
	private:
		Form* menuform;
		StageCmd stageCmd;


	public:
		static std::vector<std::shared_ptr<UfopaediaCategory>> UfopaediaDB;

		Ufopaedia(Framework &fw);
		~Ufopaedia();
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
