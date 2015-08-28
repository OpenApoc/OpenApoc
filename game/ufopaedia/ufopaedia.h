
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
		virtual void Begin() override;
		virtual void Pause() override;
		virtual void Resume() override;
		virtual void Finish() override;
		virtual void EventOccurred(Event *e) override;
		virtual void Update(StageCmd * const cmd) override;
		virtual void Render() override;
		virtual bool IsTransition() override;
};
}; //namespace OpenApoc
